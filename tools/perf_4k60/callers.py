#!/usr/bin/env python3
"""callers.py PREFIX BINARY WINDOW PATTERN [DEPTH]: who calls leaf functions matching PATTERN."""
import collections, re, sys, runpy
sys.argv = [sys.argv[0], sys.argv[1], sys.argv[2], '0', sys.argv[3]] + sys.argv[4:]
pattern, depth = re.compile(sys.argv[5]), int(sys.argv[6]) if len(sys.argv) > 6 else 3
import io, contextlib
buf = io.StringIO()
with contextlib.redirect_stdout(buf):
    g = runpy.run_path(__file__.replace('callers.py', 'analyze.py'), run_name='analyze')
samples, sym = g['samples'], g['sym']
hits = collections.Counter(); total = 0
for t, pcs in samples:
    names = [sym(pc) for pc in pcs]
    if not pattern.search(names[0]): continue
    total += 1
    chain = []
    for n in names[1:]:
        if n.startswith('[') and not 'halo2' in n: continue
        chain.append(n[:110]);
        if len(chain) >= depth: break
    hits[' <- '.join(chain)] += 1
print(f'{total} samples ({100*total/len(samples):.2f}%) match {pattern.pattern}')
for k, c in hits.most_common(25): print(f'{100*c/len(samples):6.2f}% {c:5d} {k}')
