#!/usr/bin/env python3
"""Estimate static entry-tree shares from two instruction census run lengths.

Roots and shares are static reachability estimates, not measured native entry
points or dynamic caller costs. Use a call trace/profiler for those claims.
"""
import argparse
from collections import Counter, defaultdict
from pathlib import Path
from census_common import CensusIndex, attribution_note, describe, load_census


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('short', type=Path)
    parser.add_argument('long', type=Path)
    parser.add_argument('frame_delta', type=float)
    parser.add_argument('--source-root', type=Path, default=Path('src'))
    parser.add_argument('--image', choices=('tu15', 'map-pack11'), default='tu15')
    args = parser.parse_args()
    if args.frame_delta <= 0:
        parser.error('frame_delta must be positive')
    short, long = load_census(args.short), load_census(args.long)
    census = {pc: (long[pc] - short[pc]) / args.frame_delta for pc in set(short) | set(long)
              if long[pc] > short[pc]}
    index = CensusIndex.scan(args.source_root, args.image)
    per = index.attribute(census)
    executed = set(per) - {None}
    called = {callee for caller in executed for callee in index.calls.get(caller, ())
              if callee in executed and callee != caller}
    roots = executed - called
    trees = {root: index.tree(root, executed) for root in roots}
    reach = defaultdict(set)
    for root, tree in trees.items():
        for entry in tree:
            reach[entry].add(root)
    share, exclusive = Counter(), Counter()
    for entry, count in per.items():
        if entry is None:
            continue
        owners = reach.get(entry) or {entry}
        for root in owners:
            share[root] += count / len(owners)
        if len(owners) == 1:
            exclusive[next(iter(owners))] += count
    total = sum(per.values())
    print(f'steady-state translated instructions per frame: {total:,.0f}; '
          f'executed routines {len(executed)}; static root candidates {len(roots)}')
    print(attribution_note(index, census))
    print('Shares and roots are static reachability estimates; indirect calls and dynamic ownership require a call trace.')
    print('rank  root      share/frame   exclusive   routines  unit')
    for rank, (root, count) in enumerate(sorted(share.items(), key=lambda item: (-item[1], item[0]))[:30], 1):
        units = ','.join(sorted(index.units.get(root, ()))) or '?'
        print(f'{rank:3d}  {describe(root):>7}  {count:12,.0f} ({100*count/total if total else 0:4.1f}%)  '
              f'{exclusive[root]:10,.0f}  {len(trees.get(root, {root})):5d}  {units}')


if __name__ == '__main__':
    main()
