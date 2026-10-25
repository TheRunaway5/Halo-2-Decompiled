#!/usr/bin/env python3
"""children.py PREFIX BINARY WINDOW PATTERN: inclusive breakdown of direct callees under frames matching PATTERN."""
import collections, re, sys, runpy, io, contextlib
pattern = re.compile(sys.argv[4])
sys.argv = [sys.argv[0], sys.argv[1], sys.argv[2], '0', sys.argv[3]]
with contextlib.redirect_stdout(io.StringIO()):
    g = runpy.run_path(__file__.replace('children.py', 'analyze.py'), run_name='analyze')
samples, sym = g['samples'], g['sym']
child = collections.Counter(); total = 0
for t, pcs in samples:
    names = [sym(pc) for pc in pcs]
    idx = [i for i, n in enumerate(names) if pattern.search(n)]
    if not idx: continue
    total += 1
    i = max(idx)  # outermost match
    c = names[i-1] if i > 0 else '<self>'
    child[c[:150]] += 1
n = len(samples)
print(f'{100*total/n:.2f}% under {pattern.pattern}')
for k, c in child.most_common(20): print(f'{100*c/n:6.2f}% {k}')
