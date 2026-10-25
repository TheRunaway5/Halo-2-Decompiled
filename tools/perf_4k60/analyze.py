#!/usr/bin/env python3
"""Summarize sampler.c output: self and inclusive function profiles."""
import bisect, collections, struct, subprocess, sys
from pathlib import Path

prefix, binary = sys.argv[1], Path(sys.argv[2]).resolve()
skip_roots = 6
top = int(sys.argv[3]) if len(sys.argv) > 3 else 45
window = float(sys.argv[4]) if len(sys.argv) > 4 else 0
data = Path(prefix + '.samples').read_bytes()
(count,) = struct.unpack_from('<I', data, 0)
off, samples = 4, []
for _ in range(count):
    t, n = struct.unpack_from('<QI', data, off); off += 12
    pcs = struct.unpack_from(f'<{n}Q', data, off); off += 8 * n
    samples.append((t, pcs))
if window and samples:
    last = samples[-1][0]
    samples = [x for x in samples if x[0] >= last - window * 1e9]

maps = []
for line in Path(prefix + '.maps').read_text().splitlines():
    parts = line.split()
    if len(parts) >= 6:
        lo, hi = (int(x, 16) for x in parts[0].split('-'))
        maps.append((lo, hi, int(parts[2], 16), parts[5]))
base = min(lo - o for lo, hi, o, p in maps if Path(p).name == binary.name)

nm = subprocess.run(['nm', '-C', '--defined-only', '-n', str(binary)], capture_output=True, text=True).stdout
addrs, names = [], []
for line in nm.splitlines():
    parts = line.split(' ', 2)
    if len(parts) == 3 and parts[1] in 'tTwWiI':
        addrs.append(int(parts[0], 16)); names.append(parts[2])

modsyms = {}
def module_symbols(path):
    if path not in modsyms:
        out = subprocess.run(['nm', '-D', '-C', '--defined-only', '-n', path], capture_output=True, text=True).stdout
        out += subprocess.run(['nm', '-C', '--defined-only', '-n', path], capture_output=True, text=True).stdout
        rows = sorted({(int(l.split(' ',2)[0],16), l.split(' ',2)[2]) for l in out.splitlines()
                       if len(l.split(' ',2)) == 3 and l.split(' ',2)[1] in 'tTwWiI'})
        modsyms[path] = ([a for a, _ in rows], [n for _, n in rows])
    return modsyms[path]
def module_base(path):
    return min(lo - o for lo, hi, o, p in maps if p == path)
def module(pc):
    for lo, hi, o, p in maps:
        if lo <= pc < hi:
            return p
    return '?'

cache = {}
def sym(pc):
    if pc in cache: return cache[pc]
    rel = pc - base
    i = bisect.bisect_right(addrs, rel) - 1
    m = module(pc)
    if Path(m).name == binary.name and i >= 0:
        name = names[i]
    else:
        name = '[' + Path(m).name + ']'
        if m.startswith('/'):
            a, n = module_symbols(m)
            j = bisect.bisect_right(a, pc - module_base(m)) - 1
            if j >= 0: name = '[' + Path(m).name + '] ' + n[j]
    if len(name) > 150: name = name[:150] + '…'
    cache[pc] = name
    return name

self_c, incl_c = collections.Counter(), collections.Counter()
for t, pcs in samples:
    stack = [sym(pc) for pc in pcs]
    self_c[stack[0]] += 1
    for name in set(stack):
        incl_c[name] += 1
total = len(samples)
span = (samples[-1][0] - samples[0][0]) / 1e9 if samples else 0
print(f'samples={total} wall_span={span:.1f}s')
print('\n== self ==')
for name, c in self_c.most_common(top):
    print(f'{100*c/total:6.2f}% {c:6d} {name}')
print('\n== inclusive ==')
for name, c in incl_c.most_common(top):
    print(f'{100*c/total:6.2f}% {c:6d} {name}')
