#!/usr/bin/env python3
"""Diff the original engine's trace against the SDL port's log.

CRUX_DEBUG.EXE (see tools/patch_trace.py) writes CRUXTRC.LOG from the engine's own
177 Debug_Trace sites. The port logs the same landmarks in its own words. Both are
reduced here to one event vocabulary — SCREEN / ANIM_ADD / ANIM_FREE / SCM — and
aligned, so "the port diverges from the real engine HERE" becomes a one-line answer.

Capture a port log with the landmarks enabled:
    ANIM_LOG=1 ./port/crux . 2>&1 | tee /tmp/port.log

Usage: python3 tools/tracediff.py ../granny/CRUXTRC.LOG /tmp/port.log
"""
import re, sys
from difflib import SequenceMatcher

# (regex, event kind, capture group holding the name)
ENGINE_RULES = [
    (re.compile(r'^Entered screen (\S+)'),                       'SCREEN'),
    (re.compile(r'Adding animation: (\S+?)/\d+'),                'ANIM_ADD'),
    (re.compile(r'dumping: (\S+) at'),                           'ANIM_FREE'),
    (re.compile(r'Playing sca/m: (\S+)'),                        'SCM'),
]

PORT_RULES = [
    (re.compile(r"^\[INFO \] Scene '([^']+)': version="),        'SCREEN'),
    (re.compile(r"^\[INFO \] ANIM add\s+slot \d+ '([^']+)'"),    'ANIM_ADD'),
    (re.compile(r"^\[INFO \] ANIM free\s+slot \d+ '([^']+)'"),   'ANIM_FREE'),
    (re.compile(r'^\[INFO \] play SCM (\S+):'),                  'SCM'),
    (re.compile(r"^\[INFO \] SKIP_VIDEOS: skipping SCM '([^']+)'"), 'SCM'),
]


def extract(path, rules):
    events = []
    for lineno, line in enumerate(open(path, encoding='latin1'), 1):
        line = line.rstrip('\n')
        for rx, kind in rules:
            m = rx.search(line)
            if m:
                events.append((kind, m.group(1).upper(), lineno))
                break
    return events


def key(events):
    """The comparable part of an event — kind and name, not line number."""
    return [(k, n) for k, n, _ in events]


def render(events, i):
    if i >= len(events):
        return '<end of log>'
    k, n, lineno = events[i]
    return 'line %-5d %s %s' % (lineno, k, n)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    engine = extract(sys.argv[1], ENGINE_RULES)
    port = extract(sys.argv[2], PORT_RULES)
    print('engine: %d events   port: %d events\n' % (len(engine), len(port)))

    matcher = SequenceMatcher(None, key(engine), key(port), autojunk=False)
    divergences = 0
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == 'equal':
            continue
        divergences += 1
        print('--- divergence %d (%s) ---' % (divergences, tag))
        print('  engine @ %s' % render(engine, i1))
        print('  port   @ %s' % render(port, j1))
        for k, n, _ in engine[i1:i2][:8]:
            print('    engine only: %s %s' % (k, n))
        for k, n, _ in port[j1:j2][:8]:
            print('    port   only: %s %s' % (k, n))
        if divergences == 1:
            print('  ^^ FIRST DIVERGENCE — start here')
        print()
        if divergences >= 10:
            print('(stopping after 10)')
            break
    if not divergences:
        print('no divergence — the two runs match event for event')


if __name__ == '__main__':
    main()
