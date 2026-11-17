#!/usr/bin/env python3
"""Exercise the real 1704C0 producer and BCB70 consumer with both Cache leases."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

REFERENCE = 'HALO2_REFERENCE_OBJECT_FUNCTION_SCALAR'
PRIMARY = 'src/physics/physics_live_support_tu15_generated.cpp'
PRODUCER = 'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def verify_counters(path, reference):
    source = json.loads(path.read_bytes())
    assert not any(source[key] for key in ('active_scopes', 'omitted_records', 'registration_failures', 'orphan_literal_events'))
    rows = {}
    for values in source['rows']:
        row = dict(zip(source['row_fields'], values, strict=True))
        unit = source['units'][row['unit']]
        if (unit[0], row['symbol']) not in ((PRIMARY, 'bcb70'), (PRODUCER, '1704c0')):
            continue
        assert source['image_names'][row['image']] == 'tu15' and row['symbol'] not in rows
        rows[row['symbol']] = {**row, 'source': unit[0], 'canonical_sha256': unit[1], 'input_sha256': unit[2]}
    parent = rows['bcb70']
    producer = rows['1704c0']
    assert (parent['literal_sites'], parent['entries'], parent['returns'], parent['unwinds']) == (138, 4, 4, 0)
    assert (producer['entries'], producer['returns'], producer['unwinds']) == (2, 0, 2)
    if reference:
        assert parent['body_entries'] == 4 and parent['literal_statements'] == 144
    else:
        assert parent['returns_without_body'] == 4 and parent['body_entries'] == parent['literal_statements'] == 0
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--accounting', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    if not args.map.is_file() or not args.map.with_name('shared.map').is_file():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    report = {'passed': False, 'owned_scratch': str(scratch), 'retained_large_artifacts': [], 'routes': {}}
    try:
        report.update(map_sha256=sha(args.map), shared_map_sha256=sha(args.map.with_name('shared.map')),
                      binary_sha256=sha(args.binary), configured_accounting=args.accounting)
        env = {key: value for key, value in os.environ.items() if not key.startswith('HALO2_REFERENCE_')}
        env.pop('HALO2_NATIVE_EXECUTION_REPORT', None)
        results = {}
        for reference in (False, True):
            mode = 'reference' if reference else 'default'
            effective = dict(env)
            if reference:
                effective[REFERENCE] = '1'
            counters = scratch / (mode + '-accounting.json')
            if args.accounting:
                effective['HALO2_NATIVE_EXECUTION_REPORT'] = str(counters)
            command = [str(args.binary), '--bcb70-producer', str(args.map)]
            run = subprocess.run(command, env=effective, text=True, capture_output=True, timeout=90)
            if run.returncode == 77:
                return 77
            if run.returncode:
                raise RuntimeError(f'{mode} producer/consumer exited {run.returncode}: {run.stderr[-6000:]}')
            result = json.loads(run.stdout)
            assert result['passed'] and result.pop('native_bcb70_entries') == (0 if reference else 4)
            for lease, key in (('original', 0x030005a6), ('mcc', 0x030005b2)):
                route = result[lease]
                assert route['producer_entry'] == 0x1704c0 and route['producer_instruction'] == 0x170676
                assert route['producer_visits'] == route['primary_stores'] == route['secondary_stores'] == 1
                assert route['produced_function'] == key and route['actual_consumer_calls'] == 2
            results[mode] = result
            report['routes'][mode] = {'result': result, 'native_bcb70_entries': 0 if reference else 4}
            if args.accounting:
                report['routes'][mode]['accounting'] = verify_counters(counters, reference)
        assert results['default'] == results['reference']
        report['passed'] = True
    except Exception as error:
        report['error'] = repr(error)
        raise
    finally:
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
