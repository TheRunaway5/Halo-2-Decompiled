#!/usr/bin/env python3
# selflines.py PREFIX BINARY PATTERN [TOP]: source lines of the LEAF samples that symbolize
# to a function matching PATTERN (its self time), through addr2line with inlining, so the
# inlined loops of a large function get line-level attribution. Needs debug line info.
import bisect, collections, re, struct, subprocess, sys
from pathlib import Path
prefix, binary, pattern = sys.argv[1], Path(sys.argv[2]).resolve(), re.compile(sys.argv[3])
top = int(sys.argv[4]) if len(sys.argv) > 4 else 40
data = Path(prefix + '.samples').read_bytes()
(count,) = struct.unpack_from('<I', data, 0); off, leaves = 4, []
for _ in range(count):
    t, n = struct.unpack_from('<QI', data, off); off += 12
    pcs = struct.unpack_from(f'<{n}Q', data, off); off += 8 * n
    if n: leaves.append(pcs[0])
maps = []
for line in Path(prefix + '.maps').read_text().splitlines():
    parts = line.split()
    if len(parts) >= 6:
        lo, hi = (int(x, 16) for x in parts[0].split('-')); maps.append((lo, hi, int(parts[2], 16), parts[5]))
base = min(lo - o for lo, hi, o, p in maps if Path(p).name == binary.name)
nm = subprocess.run(['nm', '-C', '--defined-only', '-n', str(binary)], capture_output=True, text=True).stdout
addrs, names = [], []
for line in nm.splitlines():
    parts = line.split(' ', 2)
    if len(parts) == 3 and parts[1] in 'tTwWiI': addrs.append(int(parts[0], 16)); names.append(parts[2])
in_binary = lambda pc: any(lo <= pc < hi and Path(p).name == binary.name for lo, hi, o, p in maps)
hits = collections.Counter()
for pc in leaves:
    if not in_binary(pc): continue
    rel = pc - base; i = bisect.bisect_right(addrs, rel) - 1
    if i >= 0 and pattern.search(names[i]): hits[rel] += 1
total = sum(hits.values())
print(f'{total} self samples of {len(leaves)} match {pattern.pattern}')
if not total: sys.exit(0)
lines = collections.Counter()
for a, n in hits.items():
    out = subprocess.run(['addr2line', '-e', str(binary), '-i', '-f', '-C', '-s', hex(a)], capture_output=True, text=True).stdout.splitlines()
    chain = ' <- '.join(f'{out[i+1]} {out[i][:40]}' for i in range(0, min(len(out), 6), 2))
    lines[chain] += n
for loc, n in lines.most_common(top): print(f'{100*n/total:6.2f}%  {n:5d}  {loc}')
