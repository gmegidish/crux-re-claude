#!/usr/bin/env python3
"""Re-enable CRUX.EXE's compiled-out Debug_Trace.

The release build stubbed Debug_Trace(line, srcFile, msg) at 0x0041a770 to an
empty body (it moves each argument onto itself and returns), so all 177 call
sites still pass their arguments but nothing is recorded. This rewrites the
stub as a jump into a code cave that does:

    char buf[1024];
    vsprintf(buf, fmt, (va_list)&fmt + 1);
    FILE* f = fopen("CRUXTRC.LOG", "a");
    if (f) { fprintf(f, "%s(%d): %s\n", srcFile, line, buf); fclose(f); }

Debug_Trace is varargs — Debug_Trace(line, srcFile, fmt, ...) — so the message
has to be formatted with the call site's own arguments before it is written.

using the game's own statically-linked MSVC CRT. Writes a patched copy; the
original is never modified.

Usage: python3 tools/patch_trace.py [CRUX.EXE] [CRUX_DEBUG.EXE]
"""
import struct, sys

IMAGE_BASE = 0x400000
STUB       = 0x0041a770   # Debug_Trace no-op stub (thunk 0x004016ea, 177 callers)
CAVE       = 0x00469c00   # inside a 7604-byte int3 run at 0x00469bfc
FOPEN      = 0x0048a340
FPRINTF    = 0x0048a840
FCLOSE     = 0x0048a0d0
VSPRINTF   = 0x0048a6a0
BUFSIZE    = 0x400

LOGNAME = b"CRUXTRC.LOG\0"
MODE    = b"a\0"
FMT     = b"%s(%d): %s\n\0"


def sections(d):
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    nsec = struct.unpack_from('<H', d, pe + 6)[0]
    optsz = struct.unpack_from('<H', d, pe + 20)[0]
    off = pe + 24 + optsz
    out = []
    for _ in range(nsec):
        va, ra = struct.unpack_from('<I', d, off + 12)[0], struct.unpack_from('<I', d, off + 20)[0]
        vs, rs = struct.unpack_from('<I', d, off + 8)[0], struct.unpack_from('<I', d, off + 16)[0]
        out.append((IMAGE_BASE + va, vs, ra, rs))
        off += 40
    return out


def make_va_to_file(secs):
    def v2f(va):
        for vstart, vsize, ra, rs in secs:
            if vstart <= va < vstart + vsize:
                return ra + (va - vstart)
        raise ValueError("VA %08x not mapped" % va)
    return v2f


def rel32(frm_end, to):
    """rel32 displacement for an instruction ENDING at frm_end."""
    return struct.pack('<i', to - frm_end)


def build_cave(str_base):
    """Assemble the trace body. str_base = VA where the literals live."""
    log_va = str_base
    mode_va = log_va + len(LOGNAME)
    fmt_va = mode_va + len(MODE)

    code = bytearray()
    code += bytes.fromhex('55')            # push ebp
    code += bytes.fromhex('8bec')          # mov  ebp, esp
    code += b'\x81\xec' + struct.pack('<I', BUFSIZE)   # sub esp, BUFSIZE   (message buffer)
    code += bytes.fromhex('56')            # push esi   (callee-saved; holds FILE*)
    # vsprintf(buf, fmt, &fmt + 1) — format with the call site's own varargs.
    code += bytes.fromhex('8d4514')        # lea  eax, [ebp+0x14]   first vararg
    code += bytes.fromhex('50')            # push eax
    code += bytes.fromhex('ff7510')        # push [ebp+0x10]        fmt
    code += b'\x8d\x85' + struct.pack('<i', -BUFSIZE)  # lea eax, [ebp-BUFSIZE]
    code += bytes.fromhex('50')            # push eax               buf
    code += b'\xe8' + b'\0\0\0\0'             # call vsprintf
    vsprintf_at = len(code)
    code += bytes.fromhex('83c40c')        # add  esp, 0xc
    code += b'\x68' + struct.pack('<I', mode_va)   # push "a"
    code += b'\x68' + struct.pack('<I', log_va)    # push "CRUXTRC.LOG"
    code += b'\xe8' + b'\0\0\0\0'                  # call fopen        (patched below)
    fopen_at = len(code)
    code += bytes.fromhex('83c408')        # add  esp, 8
    code += bytes.fromhex('85c0')          # test eax, eax
    code += bytes.fromhex('7400')          # je   done          (patched below)
    je_at = len(code)
    code += bytes.fromhex('8bf0')          # mov  esi, eax      (FILE*)
    code += b'\x8d\x85' + struct.pack('<i', -BUFSIZE)  # lea eax, [ebp-BUFSIZE]
    code += bytes.fromhex('50')            # push eax           formatted msg
    code += bytes.fromhex('ff7508')        # push [ebp+8]       line
    code += bytes.fromhex('ff750c')        # push [ebp+0xc]     srcFile
    code += b'\x68' + struct.pack('<I', fmt_va)    # push "%s(%d): %s\n"
    code += bytes.fromhex('56')            # push esi           FILE*
    code += b'\xe8' + b'\0\0\0\0'                  # call fprintf
    fprintf_at = len(code)
    code += bytes.fromhex('83c414')        # add  esp, 0x14
    code += bytes.fromhex('56')            # push esi
    code += b'\xe8' + b'\0\0\0\0'                  # call fclose
    fclose_at = len(code)
    code += bytes.fromhex('83c404')        # add  esp, 4
    done_at = len(code)
    code += bytes.fromhex('5e')            # pop  esi
    code += bytes.fromhex('8be5')          # mov  esp, ebp      (drop the buffer)
    code += bytes.fromhex('5d')            # pop  ebp
    code += bytes.fromhex('c3')            # ret                (cdecl: caller cleans)

    # short-jump displacement for `je done`
    code[je_at - 1] = done_at - je_at
    return code, fopen_at, fprintf_at, fclose_at, vsprintf_at, len(code)


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else 'CRUX.EXE'
    dst = sys.argv[2] if len(sys.argv) > 2 else 'CRUX_DEBUG.EXE'
    d = bytearray(open(src, 'rb').read())
    v2f = make_va_to_file(sections(d))

    # The stub must still be the untouched no-op, or we are patching the wrong build.
    expected = bytes.fromhex('558bec8b45088945088b4d0c894d0c8b5510895510 5dc3'.replace(' ', ''))
    at = v2f(STUB)
    if bytes(d[at:at + len(expected)]) != expected:
        sys.exit("stub at %08x is not the expected no-op — wrong binary?" % STUB)

    # Lay the code out first with a placeholder string base, then place the
    # literals immediately after it (the size does not depend on the base).
    code, fopen_at, fprintf_at, fclose_at, vsprintf_at, size = build_cave(CAVE)
    str_base = CAVE + size
    code, fopen_at, fprintf_at, fclose_at, vsprintf_at, size = build_cave(str_base)

    code[vsprintf_at - 4:vsprintf_at] = rel32(CAVE + vsprintf_at, VSPRINTF)
    code[fopen_at - 4:fopen_at] = rel32(CAVE + fopen_at, FOPEN)
    code[fprintf_at - 4:fprintf_at] = rel32(CAVE + fprintf_at, FPRINTF)
    code[fclose_at - 4:fclose_at] = rel32(CAVE + fclose_at, FCLOSE)

    blob = bytes(code) + LOGNAME + MODE + FMT
    cave_off = v2f(CAVE)
    if any(b != 0xcc for b in d[cave_off:cave_off + len(blob)]):
        sys.exit("cave at %08x is not free padding" % CAVE)
    d[cave_off:cave_off + len(blob)] = blob

    # Stub -> jmp cave.
    d[at:at + 5] = b'\xe9' + rel32(STUB + 5, CAVE)

    open(dst, 'wb').write(d)
    print("patched %s -> %s" % (src, dst))
    print("  stub  %08x -> jmp %08x  (%d bytes of code + %d of literals)"
          % (STUB, CAVE, size, len(blob) - size))
    print("  traces from all 177 call sites now append to CRUXTRC.LOG")


if __name__ == '__main__':
    main()
