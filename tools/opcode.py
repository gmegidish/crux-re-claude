#!/usr/bin/env python3
"""Disassemble one RunProg_Exec opcode handler, with call targets named.

RunProg_Exec's main dispatch is a jump table at 0x00468f27 indexed by (opcode - 1)
over 0..0x2c4. This resolves an opcode to its handler and disassembles it, naming
any called function from FUNCTIONS.md.

Argument slots (calibrated against SET_VAR, whose handler is
`var[[ebp-0x13c]] = [ebp-0x138]` writing the variable file at 0x0070fa38):
    [ebp-0x140] opcode   [ebp-0x13c] arg0   [ebp-0x138] arg1   [ebp-0x134] arg2

Usage: python3 tools/opcode.py 0x26f [0x207 ...]
"""
import re, struct, subprocess, sys

IMAGE_BASE = 0x400000

# RunProg_Exec dispatches through several range tables, each preceded by `sub $base` +
# `cmp $count` + `ja default`. (base, count_inclusive, table_va) — found by scanning for
# `jmp *tbl(,reg,4)` inside 0x462560..0x46bc40 and reading the guard before each.
# Opcodes outside all of these are handled by direct `cmpl $0x<op>` compares in
# 0x462800..0x462960 (e.g. 0x9c4) — grep for the constant there.
TABLES = [
    (0x001,  0x2c4, 0x00468f27),   # the main one
    (0x84d,  0x00b, 0x00469a3b),
    (0x8fe,  0x01e, 0x00469a6b),
    (0x961,  0x014, 0x00469ae7),
    (0x9c5,  0x004, 0x00469b3b),
    (0x1b59, 0x018, 0x00469b77),
    (0x906,  0x005, 0x00469be4),
]
ARGS = {0x13c: 'arg0', 0x138: 'arg1', 0x134: 'arg2', 0x140: 'opcode'}


def sections(d):
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    nsec = struct.unpack_from('<H', d, pe + 6)[0]
    off, out = pe + 24 + struct.unpack_from('<H', d, pe + 20)[0], []
    for _ in range(nsec):
        vs, va = struct.unpack_from('<II', d, off + 8)
        rs, ra = struct.unpack_from('<II', d, off + 16)
        out.append((IMAGE_BASE + va, vs, ra, rs))
        off += 40
    return out


def load_names():
    names, row = {}, re.compile(r'^\|\s*`0x([0-9a-fA-F]{8})`\s*\|\s*`?([^`|]*)`?\s*\|')
    for line in open('FUNCTIONS.md', encoding='utf-8'):
        m = row.match(line)
        if m:
            names[int(m.group(1), 16)] = m.group(2).strip()
    return names


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    d = open('CRUX.EXE', 'rb').read()
    secs = sections(d)
    tva, tvs, tra, _ = secs[0]
    v2f = lambda va: tra + (va - tva)
    names = load_names()

    def thunk(va):
        o = v2f(va)
        return va + 5 + struct.unpack_from('<i', d, o + 1)[0] if d[o] == 0xe9 else va

    def name_of(va):
        n = names.get(va, '')
        if n.startswith('thunk_') or not n:
            n = names.get(thunk(va), n)
        return n

    for spec in sys.argv[1:]:
        op = int(spec, 16)
        entry = next(((b, n, t) for b, n, t in TABLES if b <= op <= b + n), None)
        if entry is None:
            print('0x%x: in no dispatch table — likely a direct compare; try\n'
                  '  objdump -d --start-address=0x462800 --stop-address=0x462960 CRUX.EXE'
                  ' | grep -i \'$0x%x\'\n' % (op, op))
            continue
        base_op, _n, table = entry
        handler = struct.unpack_from('<I', d, v2f(table + (op - base_op) * 4))[0]
        print('=== opcode 0x%x -> handler 0x%08x %s' % (op, handler, '=' * 30))

        out = subprocess.run(
            ['objdump', '-d', '--start-address=0x%x' % handler,
             '--stop-address=0x%x' % (handler + 0x60), 'CRUX.EXE'],
            capture_output=True, text=True).stdout
        for line in out.splitlines():
            m = re.match(r'\s+([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s*(.*)', line)
            if not m:
                continue
            addr, text = int(m.group(1), 16), m.group(2)
            if addr < handler:
                continue
            # annotate argument slots and call targets
            for off, label in ARGS.items():
                text = text.replace('-0x%x(%%ebp)' % off, '-0x%x(%%ebp){%s}' % (off, label))
            c = re.search(r'call.*?0x([0-9a-f]+)', text)
            if c:
                t = int(c.group(1), 16)
                n = name_of(t) or name_of(thunk(t))
                if n:
                    text += '   ; %s' % n
            print('  %08x  %s' % (addr, text))
            if text.startswith('jmp') and 'ebp' not in text:
                break          # handlers end by jumping back to the dispatch tail
        print()


if __name__ == '__main__':
    main()
