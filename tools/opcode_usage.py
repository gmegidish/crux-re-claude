#!/usr/bin/env python3
"""Which opcodes does the game actually use, and which does the port implement?

Runs dumpprog over every scene, histograms the opcodes, and marks each as
implemented or not by scanning RunProg.cpp's `case 0x...:` labels. The unimplemented
rows, sorted by frequency, are the work queue — an opcode used 100 times across 20
scenes matters more than one used once.

Usage: python3 tools/opcode_usage.py [--all]     (--all also lists implemented ones)
"""
import re, subprocess, sys
from collections import Counter, defaultdict

scenes = subprocess.run(['./port/dumpprog', '.', '--scenes', 'x'],
                        capture_output=True, text=True).stdout.split()

implemented = set()
for m in re.finditer(r'case (0x[0-9a-fA-F]+):', open('port/RunProg.cpp').read()):
    implemented.add(int(m.group(1), 16))

counts, scenes_using = Counter(), defaultdict(set)
op_re = re.compile(r'op=(0x[0-9a-f]+)')
for s in scenes:
    out = subprocess.run(['./port/dumpprog', '.', s, 'all'],
                         capture_output=True, text=True).stdout
    for m in op_re.finditer(out):
        op = int(m.group(1), 16)
        counts[op] += 1
        scenes_using[op].add(s)

show_all = '--all' in sys.argv
print('%-8s %6s %7s  %s' % ('opcode', 'uses', 'scenes', 'status'))
missing = 0
for op, n in counts.most_common():
    done = op in implemented
    if done and not show_all:
        continue
    if not done:
        missing += n
    print('%-8s %6d %7d  %s' % (hex(op), n, len(scenes_using[op]),
                                'ok' if done else 'MISSING  ' + ' '.join(sorted(scenes_using[op])[:6])))
print('\n%d scenes, %d distinct opcodes, %d unimplemented uses'
      % (len(scenes), len(counts), missing))
