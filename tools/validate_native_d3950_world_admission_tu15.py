#!/usr/bin/env python3
"""Exercise the two genuine D3950 required-owner calls through public World."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from validate_native_node_function_progress_source_tu15 import PIN, Xbe, corpus

COAGULATION = '1de37727dbb9c51aa08e03bfea86c0641ad67250dba28c5dcb3ba3dadaaf93fa'
REFERENCE = 'HALO2_REFERENCE_NODE_PROGRESS_QUERY'
CALLS = {0xd3998: 0xd399d, 0xd39b3: 0xd39b8}


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def verify_calls(image):
    for site, returned in CALLS.items():
        code = image.read(site, 5)
        if code[0] != 0xe8 or returned + int.from_bytes(code[1:], 'little', signed=True) != 0x1cf0e0:
            raise ValueError(f'Wrong original D3950 CALL at {site:x}')


def verify_observed(rows):
    calls = {site: 0 for site in CALLS}
    stores = 0
    for row in rows:
        caller_stores = [event for event in row['stores'] if event[0] in CALLS]
        child_stores = [event for event in row['stores'] if event[0] == 0x1cf0e9]
        if len(caller_stores) != len(child_stores) or len(caller_stores) > 1:
            raise ValueError('D3950 did not reach the genuine AIFrame scratch publication exactly once')
        if caller_stores:
            call = caller_stores[0]
            if int.from_bytes(bytes.fromhex(call[3]), 'little') != CALLS[call[0]]:
                raise ValueError('D3950 did not push the original CALL return')
            calls[call[0]] += 1
        stores += len(row['stores'])
    if set(calls.values()) != {14}:
        raise ValueError('D3950 did not take both authentic CALLs on their admitted rows')
    return {f"{site:x}": count for site, count in calls.items()}, stores


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    if not args.xbe.is_file() or not args.map.is_file():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    result = {'passed': False, 'owned_scratch': str(scratch), 'retained_large_artifacts': []}
    try:
        if digest(args.xbe) != PIN or digest(args.map) != COAGULATION:
            raise ValueError('D3950 requires the authenticated TU1.5 XBE and Coagulation map')
        result.update(xbe_sha256=PIN, map_sha256=COAGULATION, binary_sha256=digest(args.binary))
        image = Xbe(args.xbe)
        verify_calls(image)
        rows = corpus(image)
        fixture = scratch / 'd3950-inputs.json'
        fixture.write_text(json.dumps(rows, separators=(',', ':')))
        result['fixture_sha256'] = digest(fixture)
        base = {key: value for key, value in os.environ.items() if not key.startswith('HALO2_REFERENCE_')}
        outputs = {}
        for mode in ('observed', 'quiet'):
            for variant, env in (('default', base), ('reference', {**base, REFERENCE: '1'})):
                identity = mode + '-' + variant
                command = [str(args.binary.resolve()), str(args.map.resolve()), str(args.xbe.resolve()),
                           str(scratch / identity), str(fixture), mode]
                run = subprocess.run(command, env=env, capture_output=True, text=True, timeout=90)
                if run.returncode:
                    raise RuntimeError(identity + ' failed: ' + run.stderr[-5000:])
                reply = json.loads(run.stdout)
                if len(reply['rows']) != len(rows) or reply['direct_refusals'] != 2:
                    raise ValueError(identity + ' did not exercise the full public World fixture')
                outputs[identity] = reply['rows']
        for mode in ('observed', 'quiet'):
            if outputs[mode + '-default'] != outputs[mode + '-reference']:
                raise ValueError(mode + ' World default and forced-reference results differ')
        calls, stores = verify_observed(outputs['observed-default'])
        for observed, quiet in zip(outputs['observed-default'], outputs['quiet-default']):
            if quiet['stores'] or any(quiet[key] != observed[key] for key in ('name', 'abi', 'changes')):
                raise ValueError('World observer altered the escaped D3950 state for ' + quiet['name'])
        result.update(passed=True, complete_returns_per_route=len(rows), actual_child_returns_by_site=calls,
                      ordered_store_packets=stores, compared_routes=list(outputs), direct_refusals_per_route=2)
    except Exception as error:
        result['error'] = str(error)
        raise
    finally:
        args.report.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
