#!/usr/bin/env python3
"""Inventory MCC weapon identities against the retained Xbox TU1.5 scope.

Run through tools/run_test.py. This reads MCC directories, not gameplay state,
and never certifies behavior, tag conversion, pixels or an entire weapon.
"""
import argparse
import json
import os
from pathlib import Path

from mcc_content import MccCache, directory
from weapon_test_cache import require_mcc_map, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mcc-root', type=Path, required=True)
    parser.add_argument('--reference-scope', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    maps = directory(args.mcc_root) / 'h2_maps_win64_dx11'
    reference = json.loads(args.reference_scope.read_text())
    expected = {weapon['name'] for weapon in reference['weapons']}
    occurrences = {}
    inputs = []
    failures = []
    for candidate in sorted(maps.glob('*.map')):
        try:
            path = require_mcc_map(candidate)
            cache = MccCache(path)
            try:
                rows = [tag for tag in cache.tags.values() if tag['group'] == 'weap']
                for tag in rows:
                    occurrences.setdefault(tag['name'], []).append(dict(
                        map=path.name, datum=tag['datum'], address=tag['address'],
                        metadata_bytes=tag['size'], resident=tag['offset'] is not None))
                inputs.append(dict(path=str(path), sha256=sha(path),
                    bytes=path.stat().st_size, authored_weapon_rows=len(rows)))
            finally:
                cache.close()
        except (ValueError, OSError) as error:
            failures.append(dict(path=str(candidate), error=str(error)))
    present = set(occurrences)
    missing = sorted(expected - present)
    report = dict(
        passed=bool(inputs and not failures and not missing),
        runtime_cache_source='MCC', maps_directory=str(maps),
        reference_scope=str(args.reference_scope.resolve()),
        reference_scope_sha256=sha(args.reference_scope),
        reference_weapon_paths=len(expected), mcc_weapon_paths=len(present),
        reference_weapon_paths_present=len(expected & present),
        reference_weapon_paths_missing=missing,
        mcc_additional_weapon_paths=sorted(present - expected),
        maps_read=len(inputs), authored_rows=sum(row['authored_weapon_rows'] for row in inputs),
        inputs=inputs, failures=failures,
        weapons=[dict(name=name, occurrences=occurrences[name]) for name in sorted(present)],
        cache_fallback_allowed=False, tag_conversion_performed_by_test=False,
        gameplay_verified=False, visible_pixels_verified=False,
        full_weapon_parity=False, fully_certified_weapons=0,
        tool_sha256=sha(Path(__file__)), scratch=str(scratch), large_artifacts_retained=[])
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in (
        'passed', 'maps_read', 'authored_rows', 'mcc_weapon_paths',
        'reference_weapon_paths_present', 'reference_weapon_paths_missing', 'failures')}), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
