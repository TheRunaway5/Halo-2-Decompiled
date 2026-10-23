#!/usr/bin/env python3
"""Check resident turret reference conversion against pinned original maps.

Build mcc_gameplay_reference_tests with tools/run_tests.py, then run this
through tools/run_test.py. This qualifies reference identities, not playback.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    if not args.executable.resolve().is_relative_to(owner):
        parser.error('Executable must belong to the active test owner')
    audit = json.loads(args.contracts.read_text())
    gauss = 'objects\\vehicles\\warthog\\turrets\\gauss\\weapon\\gauss_bullet'
    maps = {row['map'] for row in audit['contexts']
            if row['identity'] == ['proj', gauss] and row['platform'] == 'xbox'}
    if len(maps) != 28:
        raise ValueError('Expected all 28 inventoried original Gauss contexts')
    rows = []
    for item in audit['source_files']:
        original, donor = Path(item['xbox']), Path(item['mcc'])
        if original.name not in maps:
            continue
        if sha(original) != item['xbox_sha256'] or sha(donor) != item['mcc_sha256']:
            raise ValueError('Inventoried map changed: ' + original.name)
        result = subprocess.run([str(args.executable.resolve()), str(original), str(donor)],
                                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                timeout=120)
        output = result.stdout
        passed = result.returncode == 0 and 'PASS 1 original map-specific Gauss flyby references' in output
        rows.append({'map': original.name, 'passed': passed, 'returncode': result.returncode,
                     'diagnostic': output[:4096], 'inputs': item})
        print(json.dumps({'map': original.name, 'passed': passed}), flush=True)
    report = {'schema': 'H2_TURRET_CACHE_DEPENDENCIES_V1',
              'passed': len(rows) == 28 and all(row['passed'] for row in rows),
              'contracts_sha256': sha(args.contracts), 'executable_sha256': sha(args.executable),
              'tool_sha256': sha(Path(__file__)), 'maps': rows,
              'scope': 'Actual Cache publication of named dependency and per-map Gauss flyby references',
              'playback_or_full_tu15_parity': False, 'large_artifacts_retained': []}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
