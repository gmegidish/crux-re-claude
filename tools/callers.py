#!/usr/bin/env python3
"""Count call sites for each named CRUX.EXE function.

Answers "is this stub even called?". A function with 0 callers is dead weight; one
with 40 is load-bearing and its absence from src/ matters. Single pass over .text:
collect every direct `call rel32`, follow the import-table jmp thunks, histogram the
targets, and name them from FUNCTIONS.md.

Usage: python3 tools/callers.py [name-substring ...]      e.g. Adv_ Anim_
"""
import re, struct, sys
from collections import Counter

IMAGE_BASE = 0x400000


def sections(d):
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    off, out = pe + 24 + struct.unpack_from('<H', d, pe + 20)[0], []
    for _ in range(struct.unpack_from('<H', d, pe + 6)[0]):
        vs, va = struct.unpack_from('<II', d, off + 8)
        rs, ra = struct.unpack_from('<II', d, off + 16)
        out.append((IMAGE_BASE + va, vs, ra, rs))
        off += 40
    return out


d = open('CRUX.EXE', 'rb').read()
tva, tvs, tra, trs = sections(d)[0]
v2f = lambda va: tra + (va - tva) if tva <= va < tva + tvs else None
f2v = lambda o: tva + (o - tra)

names, row = {}, re.compile(r'^\|\s*`0x([0-9a-fA-F]{8})`\s*\|\s*`?([^`|]*)`?\s*\|')
for line in open('FUNCTIONS.md', encoding='utf-8'):
    m = row.match(line)
    if m:
        names[int(m.group(1), 16)] = m.group(2).strip()


def resolve(va):
    o = v2f(va)
    return va + 5 + struct.unpack_from('<i', d, o + 1)[0] if o is not None and d[o] == 0xe9 else va


direct, via_thunk = Counter(), Counter()
for off in range(tra, tra + trs - 5):
    if d[off] != 0xe8:
        continue
    target = f2v(off) + 5 + struct.unpack_from('<i', d, off + 1)[0]
    real = resolve(target)
    (via_thunk if real != target else direct)[real] += 1

wanted = sys.argv[1:] or ['Adv_', 'Anim_']
rows = []
for va, name in names.items():
    if name.startswith('thunk_') or not any(w in name for w in wanted):
        continue
    n = direct[va] + via_thunk[va]
    rows.append((n, name, va))
rows.sort(reverse=True)

print('%-38s %8s  %s' % ('function', 'callers', 'address'))
for n, name, va in rows:
    print('%-38s %8d  0x%08x%s' % (name, n, va, '   <-- never called' if n == 0 else ''))
print('\n%d functions, %d never called' % (len(rows), sum(1 for n, _, _ in rows if n == 0)))
