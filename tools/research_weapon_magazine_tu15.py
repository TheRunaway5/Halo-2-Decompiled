#!/usr/bin/env python3
"""Compare complete original magazine/reload owners and their jump tables.

Static Xbox TU1.1/TU1.5 byte and branch evidence only. External owners remain
explicit; this report does not execute or certify a complete weapon cycle.
"""
import argparse
import json
from pathlib import Path
import struct

import research_weapon_effect_u5 as correspondence
from weapon_reference_versions import images

OWNERS = (0x101e30, 0x101d50, 0x106c30, 0x106d40, 0x106e10,
          0x106e70, 0x109c30, 0x109d50, 0x105640, 0x106580,
          0x104aa0, 0x106030)
AMMUNITION = (0x104aa0,0x104d30,0x10a1d0,0xb8250)
TIMING_LOOKUP = (0x169f3b,0x11cf00,0xceab0,0x1de810,0x1680b3)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--ammunition', action='store_true', help='Compare shared reserve owners')
    parser.add_argument('--timing-lookup', action='store_true', help='Compare representation-based timing lookup')
    args = parser.parse_args()
    original = {label: image for label, image, _ in images()}
    assert not (args.ammunition and args.timing_lookup)
    correspondence.OWNERS = TIMING_LOOKUP if args.timing_lookup else AMMUNITION if args.ammunition else OWNERS
    correspondence.BOUNDARIES = ()
    correspondence.run(args.output / 'complete-bodies')
    path = args.output / 'report.json'
    report = json.loads((args.output / 'complete-bodies/report.json').read_text())
    family='timing-lookup' if args.timing_lookup else 'ammunition' if args.ammunition else 'magazine'
    report.update(schema=f'weapon-{family}-tu15-correspondence-v1', scope=__doc__,
        owner_family=family,
        correspondence_tool_sha256=report['tool_sha256'],
        tool_sha256=correspondence.sha(__file__),
        instructions_directory='complete-bodies/instructions')
    for row in report['owners']:
        old = correspondence._Code(original['TU1.1'], '.text')
        new = correspondence._Code(original['TU1.5'], '.text')
        before = correspondence._trace(old, int(row['old'], 16), set())
        after = correspondence._trace(new, int(row['u5'], 16), set())
        assert len(before.code) == len(after.code) == row['rows']
        assert len(before.tables) == len(after.tables)
        mapping = dict(zip(sorted(before.code), sorted(after.code)))
        tables = []
        for (oa, os, oc), (na, ns, nc) in zip(before.tables, after.tables):
            assert os == ns == 4 and oc == nc
            ov = struct.unpack('<' + 'I' * oc, original['TU1.1'].read(oa, oc * 4))
            nv = struct.unpack('<' + 'I' * nc, original['TU1.5'].read(na, nc * 4))
            assert [mapping[value] for value in ov] == list(nv)
            tables.append(dict(old_address=f'{oa:08x}', tu15_address=f'{na:08x}',
                old_targets=[f'{value:08x}' for value in ov],
                tu15_targets=[f'{value:08x}' for value in nv]))
        row['jump_tables'] = tables
    path.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
