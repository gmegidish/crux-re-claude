#!/usr/bin/env python3
"""Show the call structure of one CRUX.EXE function, in order.

Reading 5 KB of linear disassembly by hand is hopeless, but the *sequence of calls*
is what you need to reconcile a ported loop against the original. This sweeps a
function's byte range for direct calls, follows the import-table thunks
(0x00401000-0x00403954 are `jmp` stubs), and names each target from FUNCTIONS.md.

Usage: python3 tools/callgraph.py 0x004101f0 [end]
"""
import re, struct, sys

IMAGE_BASE = 0x400000


def sections(d):
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    nsec = struct.unpack_from('<H', d, pe + 6)[0]
    optsz = struct.unpack_from('<H', d, pe + 20)[0]
    off, out = pe + 24 + optsz, []
    for _ in range(nsec):
        vs, va = struct.unpack_from('<II', d, off + 8)
        rs, ra = struct.unpack_from('<II', d, off + 16)
        out.append((IMAGE_BASE + va, vs, ra, rs))
        off += 40
    return out


def names_from_functions_md(path='FUNCTIONS.md'):
    names = {}
    row = re.compile(r'^\|\s*`0x([0-9a-fA-F]{8})`\s*\|\s*`?([^`|]*)`?\s*\|')
    for line in open(path, encoding='utf-8'):
        m = row.match(line)
        if m:
            names[int(m.group(1), 16)] = m.group(2).strip()
    return names


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    start = int(sys.argv[1], 16)

    d = open('CRUX.EXE', 'rb').read()
    secs = sections(d)
    text_va, text_vs, text_ra, _ = secs[0]

    def v2f(va):
        return text_ra + (va - text_va) if text_va <= va < text_va + text_vs else None

    names = names_from_functions_md()

    # End of the function: the next named function after `start`, unless given.
    if len(sys.argv) > 2:
        end = int(sys.argv[2], 16)
    else:
        later = [a for a in names if a > start]
        end = min(later) if later else start + 0x2000

    def resolve(va):
        """Follow a jmp-thunk to its real target."""
        o = v2f(va)
        if o is not None and d[o] == 0xe9:
            return va + 5 + struct.unpack_from('<i', d, o + 1)[0]
        return va

    print('%s  0x%08x .. 0x%08x  (%d bytes)\n'
          % (names.get(start, '?'), start, end, end - start))

    lo, hi = v2f(start), v2f(end)
    for off in range(lo, hi - 4):
        if d[off] != 0xe8:                     # direct call rel32
            continue
        site = text_va + (off - text_ra)
        target = resolve(site + 5 + struct.unpack_from('<i', d, off + 1)[0])
        name = names.get(target, '')
        if name.startswith('thunk_'):
            name = names.get(resolve(target), name)
        if not name or name.startswith('FUN_'):
            continue                           # unnamed / CRT noise
        print('  %08x  call %-34s (0x%08x)' % (site, name, target))


if __name__ == '__main__':
    main()
