#!/usr/bin/env python3
"""Observe the unchanged TU1.5 1BBB50 two-area query and its real iterators.

The shared area-query input matrix supplies retained records and adversarial
input timelines. Extra cases distinguish zero, one and two matching areas.
All execution is an independent offline reference; no gameplay result,
instruction, iterator state or dependency return is supplied.
"""
import argparse
import copy
import gzip
import json
import os
from pathlib import Path

from oracle_smg_firing_1_1 import ROOT, Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
import oracle_native_campaign_ai_area_tu15 as areas
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry

ENTRIES = (0x1bbb50, 0x20d180, 0x20d2d0)


def inputs():
    rows = []
    for original in areas.inputs():
        if original['entry'] != 0x1c5870:
            continue
        row = copy.deepcopy(original)
        row.update(id=len(rows), entry=0x1bbb50,
                   name='two_areas_' + original['name'])
        rows.append(row)
        if original['name'].startswith(('default_', 'order_', 'pairs_')):
            for flags in (0, 1):
                extra = copy.deepcopy(row)
                extra.update(id=len(rows), name=f'all_flags_{flags}_' + row['name'])
                for bank in extra['banks']:
                    if bank['address'] == areas.BASE:
                        data = bytearray.fromhex(bank['before'])
                        for zone in range(4):
                            for area in range(4):
                                offset = areas.AREAS + zone * 0x800 + area * 0x88 + 0x20 - areas.BASE
                                data[offset:offset + 4] = flags.to_bytes(4, 'little')
                        bank['before'] = data.hex()
                rows.append(extra)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--verify', type=Path)
    args = parser.parse_args()
    assert not args.output.exists(), 'Preserve existing original references'
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    assert common.sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    common.ENTRIES = ENTRIES
    metadata = common.metadata(image)
    owned = areas.prepare(image, metadata, scratch)
    complete = geometry.physical(owned, inputs(), 'complete')
    prefix = []
    for seed in complete:
        if not seed['name'].startswith('two_areas_fault_seed'):
            continue
        for ordinal in range(len(seed['reads'])):
            row = copy.deepcopy(seed)
            row.update(id=len(complete) + len(prefix),
                       name=seed['name'] + '_before_read_' + str(ordinal),
                       fault=dict(phase='before_read', ordinal=ordinal))
            prefix.append(row)
    observed = complete + geometry.physical(owned, prefix, 'prefix')
    assert all(not row['writes'] and row['physical_verified'] for row in observed)
    root_sites = {site['instruction'] for site in metadata['sites']
                  if 0x1bbb50 <= site['instruction'] < 0x1bbbd0}
    reached = set().union(*(set(row['executed_sites']) for row in complete))
    assert root_sites <= reached, sorted(root_sites - reached)
    fixture = dict(image_sha256=NEW_PIN, backend='physical-unchanged-i386-offline',
                   owners=metadata['owners'], instruction_patches=[],
                   supplied_original_outcomes=False, cases=observed)
    data = gzip.compress(json.dumps(fixture, separators=(',', ':')).encode(), mtime=0)
    if args.verify:
        assert data == args.verify.read_bytes(), 'Independent original repeat differs'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    report = dict(passed=True, scope=__doc__, original_sha256=NEW_PIN,
                  cases=len(observed), complete_calls=len(complete),
                  read_fault_prefixes=len(prefix), root_sites=len(root_sites),
                  reached_closure_sites=len(reached), owners=metadata['owners'],
                  ordered_reads=sum(len(row['reads']) for row in observed),
                  original_instructions=sum(row['original_instructions'] for row in observed),
                  outcomes={str(value): sum((row['boundary_gp'][0] & 0xffff) == value
                                           for row in complete) for value in (0, 1)},
                  fixture=str(args.output), fixture_bytes=len(data),
                  fixture_sha256=common.sha(args.output),
                  independent_repeat_identical=True if args.verify else None,
                  source_sha256={str(path.relative_to(ROOT)): common.sha(path) for path in
                                 (Path(__file__), ROOT / 'tools/oracle_native_campaign_ai_area_tu15.py',
                                  ROOT / 'tools/observe_native_campaign_ai_area_tu15.cpp')},
                  owned_scratch=str(scratch), retained_large_artifacts=[],
                  native_implementation_qualified=False, live_campaign_qualified=False)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f'{len(observed)} original two-area cases; all {len(root_sites)} root sites')


if __name__ == '__main__':
    main()
