#!/usr/bin/env python3
# callees.py PREFIX BINARY PARENT_PATTERN [CHILD_PATTERN] [TOP]
# Full-depth callee breakdown: for every sample whose stack contains a frame matching
# PARENT_PATTERN (nearest the leaf), count the frame directly below it (toward the leaf)
# by full symbol name. With CHILD_PATTERN, count the frames directly below that child
# instead. Unlike children.py this does not limit the parent's distance from the leaf,
# so inclusive shares are exact: "N samples (p%)" is the parent's inclusive share.
import collections, re, sys, runpy, io, contextlib, os
if len(sys.argv) < 4:
    sys.exit(__doc__ or 'callees.py PREFIX BINARY PARENT_PATTERN [CHILD_PATTERN] [TOP]')
prefix, binary, parent = sys.argv[1], sys.argv[2], re.compile(sys.argv[3])
child = re.compile(sys.argv[4]) if len(sys.argv) > 4 and sys.argv[4] else None
top = int(sys.argv[5]) if len(sys.argv) > 5 else 14
sys.argv = [sys.argv[0], prefix, binary, '0', '12']
with contextlib.redirect_stdout(io.StringIO()):
    g = runpy.run_path(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'analyze.py'), run_name='analyze')
samples, sym = g['samples'], g['sym']
total = len(samples); counts = collections.Counter(); hit = 0
def tidy(n): return re.sub(r'std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >', 'string', n)
for t, pcs in samples:
    names = [sym(p) for p in pcs]
    if names and ('_start' in names[0] or 'libc_start_main' in names[0]): names = names[::-1]   # leaf-first
    idx = next((i for i, n in enumerate(names) if parent.search(n)), None)
    if idx is None: continue
    if child is not None:
        if idx == 0 or not child.search(names[idx - 1]): continue
        hit += 1
        counts['<self>' if idx - 1 == 0 else tidy(names[idx - 2])] += 1
    else:
        hit += 1
        counts['<self>' if idx == 0 else tidy(names[idx - 1])] += 1
print(f'{hit} samples ({100*hit/total:.2f}%) include {parent.pattern}' + (f' -> {child.pattern}' if child else ''))
for n, c in counts.most_common(top): print(f'{100*c/total:6.2f}% {c:5d}  {n[:420]}')
