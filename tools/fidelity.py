#!/usr/bin/env python3
"""Report how faithful the reverse-engineered sources and the port actually are.

FUNCTIONS.md counts a function as "reversed" once someone has looked at it, which is
not the same as the code being present. Advanim.cpp scores 97/97 while
Anim_HandleFrameTick is an empty body with `// (See decompiled body at 0x004059c0)`.
Every bug this project has chased into the disassembly (the gamma curve, the anim
dump queue) sat behind exactly that kind of entry.

This scores two things a checkbox cannot:

  src/   — per module, how many functions have a REAL body vs a stub (empty, or a
           bare placeholder return, or an explicit "see decompiled body" pointer).
           A stub means the port's model of it was written from prose.

  port/  — self-declared approximations. The port comments where it knowingly
           diverges ("the port has no...", "approximates", "isn't modeled"). Each
           is a place the engine does something we chose not to reproduce.

Usage: python3 tools/fidelity.py [--verbose]
"""
import os, re, sys

SRC, PORT = 'src', 'port'

FUNC_BODY = re.compile(r'\n(\w[\w \*&:]*?)\(([^;{]*)\)\s*\{(.*?)\n\}', re.S)
PLACEHOLDER_RETURN = re.compile(r'^return\s+(-?\d+|NULL|nullptr|FALSE|TRUE)\s*;$')
ADDR_POINTER = re.compile(r'[Ss]ee decompiled body at\s*(0x[0-9a-fA-F]+)')

# Phrases the port uses when it knowingly does not reproduce the engine.
# Decompiler residue: a body can be present but untrustworthy. SetPal_ApplyGamma had a
# full body whose arithmetic was wrong, because Ghidra rendered an inline FPU sequence as
# two opaque FUN_ calls. Any of these names in a body means it was not fully understood.
ARTIFACTS = re.compile(r'\b(FUN_[0-9a-f]{6,}|DAT_[0-9a-f]{6,}|unaff_|extraout_|in_EAX|undefined[1248]?\b)')

HEDGES = re.compile(
    r"port (?:has no|approximates|does not|doesn't|omits)"
    r"|isn't (?:ported|modeled|modelled)"
    r"|not (?:ported|modeled|modelled)"
    r"|approximat"
    r"|NOT IN THE ORIGINAL"
    r"|faithful no-op"
    r"|aren't ported",
    re.I)


def strip_comments(text):
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def scan_src(path):
    """-> (total, stubbed [(name, addr)], suspect [(name, artifact)])."""
    text = open(path, encoding='latin1').read()
    total, stubs, suspect = 0, [], []
    for name, _args, body in FUNC_BODY.findall(text):
        name = name.split()[-1].lstrip('*')
        if name in ('if', 'for', 'while', 'switch', 'else', 'do'):
            continue
        total += 1
        code = strip_comments(body).strip()
        if code == '' or PLACEHOLDER_RETURN.match(code):
            m = ADDR_POINTER.search(body)
            stubs.append((name, m.group(1) if m else None))
            continue
        m = ARTIFACTS.search(code)
        if m:
            suspect.append((name, m.group(1)))
    return total, stubs, suspect


def scan_port(path):
    out = []
    for n, line in enumerate(open(path, encoding='latin1'), 1):
        if HEDGES.search(line):
            out.append((n, line.strip().lstrip('/ ').strip()))
    return out


def main():
    verbose = '--verbose' in sys.argv

    print('=== src/ — reversed bodies vs stubs ' + '=' * 36)
    print('%-22s %6s %8s %8s %7s' % ('module', 'funcs', 'stubbed', 'suspect', 'solid'))
    rows = []
    for f in sorted(os.listdir(SRC)):
        if not f.endswith('.cpp'):
            continue
        total, stubs, suspect = scan_src(os.path.join(SRC, f))
        if total:
            rows.append(((len(stubs) + len(suspect)) / total, f, total, stubs, suspect))
    rows.sort(reverse=True)
    for frac, f, total, stubs, suspect in rows:
        solid = total - len(stubs) - len(suspect)
        flag = '  <-- treat as prose, read the binary' if frac >= 0.25 else ''
        print('%-22s %6d %8d %8d %6d%%%s'
              % (f, total, len(stubs), len(suspect), solid * 100 / total, flag))
        if verbose:
            for name, addr in stubs:
                print('        STUB    %-38s %s' % (name, addr or ''))
            for name, art in suspect:
                print('        SUSPECT %-38s contains %s' % (name, art))

    print('\n=== port/ — self-declared approximations ' + '=' * 31)
    total = 0
    for f in sorted(os.listdir(PORT)):
        if not f.endswith(('.cpp', '.h')):
            continue
        hits = scan_port(os.path.join(PORT, f))
        if not hits:
            continue
        total += len(hits)
        print('%-22s %d' % (f, len(hits)))
        if verbose:
            for n, txt in hits:
                print('        %s:%d  %s' % (f, n, txt[:110]))
    print('%-22s %d total' % ('', total))
    print('\nRun with --verbose for the individual functions and comments.')


if __name__ == '__main__':
    main()
