#!/usr/bin/env python3
"""Per-routine counts and static call-tree instruction totals from a census."""
import argparse
from collections import Counter
from pathlib import Path
from census_common import CensusIndex, attribution_note, describe, load_census


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('census', type=Path)
    parser.add_argument('roots', nargs='*', help='observed-image routine entry in hex')
    parser.add_argument('--source-root', type=Path, default=Path('src'))
    parser.add_argument('--image', choices=('tu15', 'map-pack11'), default='tu15')
    args = parser.parse_args()
    census = load_census(args.census)
    index = CensusIndex.scan(args.source_root, args.image)
    per = index.attribute(census)
    pcs = Counter(index.owner(pc) for pc in census)
    total = sum(census.values())
    percent = lambda value: 100 * value / total if total else 0
    print(f'total observed instructions {total:,}; routines executed {len(set(per) - {None})}')
    print(attribution_note(index, census))
    print('Call trees use static direct and literal dispatch/required edges; shared work is not dynamic caller attribution.')
    for name in args.roots:
        root = int(name, 16)
        tree = index.tree(root)
        dynamic = sum(per[entry] for entry in tree)
        executed = [entry for entry in tree if per[entry]]
        print(f'\n== {name}: static tree {len(tree)} routines; executed {len(executed)} routines, '
              f'{sum(pcs[entry] for entry in tree)} distinct PCs, {dynamic:,} observed instructions ({percent(dynamic):.1f}% of all)')
        for entry in sorted(executed, key=lambda value: (-per[value], value))[:15]:
            print(f'    {entry:x}: {per[entry]:>12,} instr  {pcs[entry]:5d} PCs')
    print('\n== top routines overall')
    for entry, count in per.most_common(25):
        print(f'    {describe(entry)}: {count:>12,} ({percent(count):.1f}%)')


if __name__ == '__main__':
    main()
