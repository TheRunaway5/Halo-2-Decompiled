"""Qualify actual prediction resource consumers in one immutable current cohort.

Freeze current production and vendor sources using the existing launcher build
workflow, then build only the Cache-linked resource tests through run_tests.py.
No old archives or foreign Cache headers enter this cohort. The active feedback
build and all four Root publication identities remain unchanged. Run this helper
through run_test.py so its complete source and compiler trees are removed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from build_launcher_engine import freeze_source, publication_identity, source_hashes
from prepare_workshop_mcc_command_snapshot import freeze_vendor_inputs


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--recovery', type=Path, required=True)
    parser.add_argument('--base-cache', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.output.resolve().is_relative_to(owner)
    args.output.mkdir(parents=True, exist_ok=False)
    recovery = json.loads(args.recovery.read_text())
    feedback = Path(recovery['source']).resolve(strict=True)
    feedback_inventory = source_hashes(feedback)
    assert hashlib.sha256(json.dumps(feedback_inventory, sort_keys=True).encode()).hexdigest() == recovery['production_inventory_sha256']
    identities = {name: publication_identity(root / name) for name in
                  ('Halo2-Launcher', 'Halo2-Title-Launcher', 'gameplay-runtime-selection.json',
                   'frontend-runtime-selection.json')}
    source = owner / 'source'
    before = source_hashes(root)
    root_changed_during_copy = {}
    try:
        inventory = freeze_source(root, source)
    except RuntimeError as error:
        # A concurrently edited checkout must not turn into an unauthenticated
        # mixed cohort. Keep the exact differing paths before scratch cleanup.
        current, copied = source_hashes(root), source_hashes(source)
        names = sorted(set(before) | set(current) | set(copied))
        changed = {name: dict(before=before.get(name), current=current.get(name), copied=copied.get(name))
                   for name in names if len({before.get(name), current.get(name), copied.get(name)}) != 1}
        if copied != before:
            failure = dict(scope=__doc__, passed=False, status='source_freeze_refused',
                error=str(error), differing_inputs=changed, native_tests_run=False,
                owned_scratch=str(owner), retained_large_artifacts=[])
            (args.output / 'report.json').write_text(json.dumps(failure, indent=2) + '\n')
            print(json.dumps(dict(passed=False, status=failure['status'], differing_inputs=list(changed))))
            return 1
        # The copy is the EXACT complete inventory captured before copying.
        # Root moving afterward does not alter that authenticated earlier
        # cohort. This accepts no partial source match or mixed revision.
        inventory = before
        root_changed_during_copy = changed
    vendors = freeze_vendor_inputs(root, source)
    explicit = ['tests/native_workshop_prediction_delivery_tests.cpp',
                'tests/workshop_prediction_raw_bitmap_tests.cpp',
                'tests/source_bitmap_pending_header_tests.cpp',
                'tests/fixtures/retail_source_bitmap_pending_header.tsv']
    tests = {name: digest(source / name) for name in explicit}
    command = [sys.executable, '-u', '-B', str(root / 'tools/run_tests.py'),
        '--source', str(source), '--scratch-root', str(owner / 'compiler'),
        '--output', str(args.output / 'validation'), '--jobs', '2', '--test-jobs', '1',
        '--target', 'workshop_prediction_delivery_tests',
        '--target', 'workshop_prediction_raw_bitmap_tests',
        '--target', 'source_bitmap_pending_header_tests',
        '--filter', '^(workshop_prediction_sound_resources|workshop_prediction_raw_bitmap_storage|retail_source_bitmap_pending_header)$',
        '--cmake-arg=-DHALO2_STAGE_BUILD_OUTPUT=OFF',
        '--cmake-arg=-DHALO2_TEST_ASSET_SOURCE_ROOT=' + str(root),
        '--cmake-arg=-DHALO2_WORKSHOP_TEST_CACHE=' + str(args.base_cache.resolve(strict=True)),
        '--retain-report', 'workshop-prediction-sound-resources.json',
        '--retain-report', 'Testing/Temporary/LastTest.log']
    inventory_hash = hashlib.sha256(json.dumps(inventory, sort_keys=True).encode()).hexdigest()
    report = dict(scope=__doc__, passed=False, status='running', command=command,
        source=str(source), source_files=len(inventory), source_inventory_sha256=inventory_hash,
        copied_source_matches_complete_pre_copy_inventory=True,
        Root_changes_after_inventory_capture=root_changed_during_copy,
        source_tests=tests, vendor_files=len(vendors), feedback_source=str(feedback),
        feedback_source_inventory_sha256=recovery['production_inventory_sha256'],
        borrowed_frozen_archives=False, script_declaration_admitted=False,
        actual_World_qualified=False, complete_mod_support=False,
        owned_scratch=str(owner), retained_large_artifacts=[])
    path = args.output / 'report.json'
    path.write_text(json.dumps(report, indent=2) + '\n')
    result = subprocess.run(command)
    unchanged = source_hashes(source) == inventory
    unchanged &= source_hashes(feedback) == feedback_inventory
    unchanged &= all(digest(source / name) == pin for name, pin in tests.items())
    unchanged &= all(digest(source / name) == pin for name, pin in vendors.items())
    unchanged &= all(publication_identity(root / name) == pin for name, pin in identities.items())
    evidence = args.output / 'validation/evidence/workshop-prediction-sound-resources.json'
    consumer = json.loads(evidence.read_text()) if evidence.is_file() else None
    report.update(passed=result.returncode == 0 and unchanged and consumer is not None and consumer['passed'],
                  status='finished', exit_code=result.returncode,
                  all_frozen_sources_and_publication_identities_unchanged=unchanged,
                  resource_consumer=consumer)
    path.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], exit_code=result.returncode, inputs_unchanged=unchanged)))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
