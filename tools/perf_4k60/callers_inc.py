#!/usr/bin/env python3
# callers_inc.py PREFIX BINARY PATTERN [DEPTH]: inclusive callers (toward the root) of the
# first frame matching PATTERN, aggregated over all samples that include it (callers.py
# matches leaf frames only).
import collections, re, sys, runpy, io, contextlib, os
prefix, binary, pattern = sys.argv[1], sys.argv[2], re.compile(sys.argv[3]); depth = int(sys.argv[4]) if len(sys.argv) > 4 else 4
sys.argv = [sys.argv[0], prefix, binary, '0', '12']
with contextlib.redirect_stdout(io.StringIO()):
    g = runpy.run_path(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'analyze.py'), run_name='analyze')
samples, sym = g['samples'], g['sym']
total = len(samples); hit = 0; chains = collections.Counter()
def short(n): 
    n = re.sub(r'halo2::(native_split_physics_live_support_tu15_generated::|\(anonymous namespace\)::)?', '', n); return n.split('(')[0][:60]
for t, pcs in samples:
    names = [sym(p) for p in pcs]
    if names and ('_start' in names[0] or 'libc_start_main' in names[0]): names = names[::-1]   # make leaf-first
    idx = next((i for i, n in enumerate(names) if pattern.search(n)), None)
    if idx is None: continue
    hit += 1
    # skip recursive repeats of the same owner, then take the callers toward the root
    j = idx
    while j + 1 < len(names) and pattern.search(names[j + 1]): j += 1
    chains[' <- '.join(short(n) for n in names[j + 1:j + 1 + depth])] += 1
print(f'{hit} samples ({100 * hit / total:.2f}%) include {pattern.pattern}')
for chain, n in chains.most_common(12): print(f'{100 * n / total:6.2f}% {n:5d}  {chain}')
