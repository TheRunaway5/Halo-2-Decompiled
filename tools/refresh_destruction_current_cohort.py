#!/usr/bin/env python3
"""Refresh the declared destruction fixture inside its still-owned source/build.

Run from a tools/run_test.py-owned finite command session. Production stays at
the authenticated cohort snapshot; only the named fixture and finite CTest
registrations change. CMake validation always uses tools/run_tests.py.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from test_destruction_current_cohort import CASES
from mcc_native_map_owner_publication import derive_owner_index, verify_owner_index

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cohort', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--previous-refresh', type=Path)
    parser.add_argument('--production-refresh', type=Path)
    parser.add_argument('--case', action='append', choices=tuple(CASES), required=True)
    parser.add_argument('--continuation', action='store_true')
    parser.add_argument('--fixed-clock', action='store_true')
    parser.add_argument('--pending-continuation', action='store_true',
                        help='Also qualify the module chain at its actual original pending countdown')
    parser.add_argument('--native-pending', action='store_true',
                        help='Require actual target-native pending steps and their committed stores before the original blast')
    parser.add_argument('--native-region-eligibility', action='store_true',
                        help='Observe actual native response-region decisions, including pending due/eligibility/blast order')
    parser.add_argument('--native-model-state', action='store_true',
                        help='Require actual full-generation target calls through the native model-region state owner')
    parser.add_argument('--sound-requests', action='store_true',
                        help='Retain actual original sound caller requests during firing and destruction, excluding verification replays')
    parser.add_argument('--initial-pose-read-only', action='store_true',
                        help='Compare complete native memory and resource owners across two cold body-pose publications before the first state query')
    parser.add_argument('--jobs', type=int, default=8)
    parser.add_argument('--test-jobs', type=int, default=1)
    parser.add_argument('--build-runtime', action='store_true',
                        help='Also rebuild/qualify the same-source native and SDL executables')
    args = parser.parse_args()
    if args.pending_continuation and not (args.continuation and args.fixed_clock):
        parser.error('Pending continuation requires --continuation --fixed-clock')
    if args.native_pending and not args.pending_continuation:
        parser.error('Native pending validation requires --pending-continuation')
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use a tools/run_test.py-owned finite command session')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source = scratch / 'source'
    prior = json.loads(args.cohort.read_text())
    prepared=prior.get('status')=='prepared' and prior.get('validation_not_run') is True and not prior.get('passed')
    if (Path(prior['scratch']).resolve() != scratch or not source.is_dir() or
            (not (scratch / 'build').is_dir() and not prepared)):
        raise ValueError('Cohort does not belong to this active build owner')
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = args.output / 'refresh.json'
    report = {'passed': False, 'scratch': str(scratch), 'prior_cohort': str(args.cohort.resolve()),
              'tool_sha256': sha(__file__), 'retained_large_artifacts': []}

    def save():
        receipt.write_text(json.dumps(report, indent=2) + '\n')

    save()
    manifest = json.loads((args.cohort.parent / 'source/source-manifest.json').read_text())
    expected = dict(manifest['source_files_sha256'])
    expected.update(prior['additional_frozen_inputs'])
    expected['CMakeLists.txt'] = prior['registered_cmake_sha256']
    fixture = 'tests/destruction_runtime_tu15_tests.cpp'
    registered_cases = set(prior['cases'])
    previous = {}
    if args.previous_refresh:
        previous = json.loads(args.previous_refresh.read_text())
        if Path(previous['scratch']).resolve() != scratch or previous['prior_cohort'] != str(args.cohort.resolve()):
            raise ValueError('Previous refresh has a different source/build owner')
        expected['CMakeLists.txt'] = previous['registered_cmake_sha256']
        expected[fixture] = previous['fixture_refresh']['after']
        registered_cases.update(previous.get('registered_cases', previous['cases']))
        report['previous_refresh'] = str(args.previous_refresh.resolve())
    if args.production_refresh:
        production = json.loads(args.production_refresh.read_text())
        if Path(production['scratch']).resolve() != scratch:
            raise ValueError('Production refresh has a different source/build owner')
        for name, change in production['patches'].items():
            expected[name] = change['after']
        # A later fixture refresh already includes this production admission.
        # Keep its newer fixture instead of restoring the admission's earlier
        # diagnostic hash. A new production admission still owns its new hash.
        if previous.get('production_refresh') == str(args.production_refresh.resolve()):
            expected[fixture] = previous['fixture_refresh']['after']
            expected['CMakeLists.txt'] = previous['registered_cmake_sha256']
        if previous.get('production_refresh') != str(args.production_refresh.resolve()):
            expected['CMakeLists.txt'] = production['cmake_sha256']
        report['production_refresh'] = str(args.production_refresh.resolve())
        report['patches'] = dict(production['patches'])
        if 'qualification' in production:
            report['qualification'] = production['qualification']
            report['original_qualified_dependency_closure'] = production['original_qualified_dependency_closure']
    if prior.get('private_build_partition'):
        part = prior['private_build_partition']
        # A declared later production delta can append qualified sources to
        # this same CMake file while retaining the private partition. Do not
        # replace that authenticated newer hash with the initial partition.
        if part['path'] not in report.get('patches', {}):
            expected[part['path']] = part['after']
    frozen_drift = [name for name, digest in expected.items() if sha(source / name) != digest]
    report['frozen_source_drift_before_refresh'] = frozen_drift
    report['workspace_drift_from_frozen_source'] = [name for name, digest in expected.items()
        if (ROOT / name).is_file() and sha(ROOT / name) != digest and
        name not in ('CMakeLists.txt', 'cmake/PhysicsLiveWorldTu15.cmake')]
    save()
    if frozen_drift:
        raise ValueError('Frozen source changed before the declared fixture refresh')
    maps = Path(next(iter(prior['runtime_inputs']['destruction_maps']))).parent
    cache = Path(prior['runtime_inputs']['mcc_halo2_directory']).parent
    provenance = json.loads((cache / 'provenance.json').read_text())
    verify_owner_index(cache, provenance)
    owner_index, _ = derive_owner_index(cache, provenance)
    owners = {row['map']: row for row in owner_index['maps']}
    report['runtime_inputs'] = {'destruction_maps': {}, 'prepared_native_owners': {}}
    for name in sorted({CASES[case] for case in args.case}):
        relative = f'halo2/h2_maps_win64_dx11/{name}.map'
        if relative not in owners:
            raise ValueError(f'Refreshed case lacks a verified native scenario owner: {relative}')
        path = str(maps / (name + '.map'))
        digest = sha(path)
        original_digest = prior['runtime_inputs']['destruction_maps'].get(path)
        if original_digest is not None and digest != original_digest:
            raise ValueError(f'Runtime map changed since the frozen cohort: {path}')
        report['runtime_inputs']['destruction_maps'][path] = digest
        report['runtime_inputs']['prepared_native_owners'][relative] = owners[relative]
    save()
    report['fixture_refresh'] = {'path': fixture, 'before': sha(source / fixture),
                                 'after': sha(ROOT / fixture)}
    report['fixture_refresh']['copied'] = report['fixture_refresh']['before'] != report['fixture_refresh']['after']
    if report['fixture_refresh']['copied']:
        shutil.copyfile(ROOT / fixture, source / fixture)
    if (sha(source / fixture) != report['fixture_refresh']['after'] or
            sha(ROOT / fixture) != report['fixture_refresh']['after']):
        raise ValueError('Destruction fixture changed while freezing its declared refresh')
    if 'patches' in report:
        report['patches'][fixture] = dict(before=report['fixture_refresh']['before'],after=report['fixture_refresh']['after'])
    cmake = source / 'CMakeLists.txt'
    text = cmake.read_text()
    text = text.replace('target_link_libraries(destruction_runtime_tu15_tests PRIVATE halo2_systems)',
        'target_link_libraries(destruction_runtime_tu15_tests PRIVATE halo2_systems halo2_authority_codec OpenSSL::Crypto)')
    for case in args.case:
        if case not in registered_cases:
            text += (f'\nadd_test(NAME destruction_mcc_{case} COMMAND ${{Python3_EXECUTABLE}} '
                '"${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" -- '
                '$<TARGET_FILE:destruction_runtime_tu15_tests> '
                f'"{maps / (CASES[case] + ".map")}" "{maps}" "{case}" '
                f'"${{CMAKE_CURRENT_BINARY_DIR}}/destruction-{case}.json")\n')
        text += f'set_tests_properties(destruction_mcc_{case} PROPERTIES TIMEOUT 600)\n'
    cmake.write_text(text)
    report['registered_cmake_sha256'] = sha(cmake)
    report['cmake_sha256'] = sha(cmake)
    report['registered_cases'] = sorted(registered_cases | set(args.case))
    report['removed_previous_case_outputs'] = {}
    for case in args.case:
        path = scratch / 'build' / f'destruction-{case}.json'
        if path.exists():
            report['removed_previous_case_outputs'][path.name] = sha(path)
            path.unlink()
    command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--build-worker',
        '--source', str(source), '--target', 'destruction_runtime_tu15_tests',
        '--filter', '^destruction_mcc_(' + '|'.join(args.case) + ')$',
        '--jobs', str(args.jobs), '--test-jobs', str(args.test_jobs), '--output', str(args.output / 'validation')]
    if args.build_runtime:
        command += ['--target','halo2_native','--target','physics_multiplayer_acceptance']
    for case in args.case:
        command += ['--retain-report', f'destruction-{case}.json']
    environment = os.environ.copy()
    if args.continuation:
        environment['HALO2_DESTRUCTION_CONTINUATION'] = '1'
    if args.fixed_clock:
        environment['HALO2_DESTRUCTION_FIXED_CLOCK'] = '1'
    if args.pending_continuation:
        environment['HALO2_DESTRUCTION_PENDING_CONTINUATION'] = '1'
    if args.native_pending:
        environment['HALO2_DESTRUCTION_PENDING_NATIVE'] = '1'
    if args.native_region_eligibility:
        environment['HALO2_DESTRUCTION_REGION_ELIGIBILITY_NATIVE'] = '1'
    if args.native_model_state:
        environment['HALO2_DESTRUCTION_MODEL_REGION_STATE_NATIVE'] = '1'
    if args.sound_requests:
        environment['HALO2_DESTRUCTION_SOUND_REQUESTS'] = '1'
    if args.initial_pose_read_only:
        environment['HALO2_DESTRUCTION_INITIAL_POSE_READ_ONLY'] = '1'
    report.update(command=command, continuation=args.continuation, fixed_clock=args.fixed_clock,
                  pending_continuation=args.pending_continuation, cases=args.case,
                  native_pending=args.native_pending,
                  native_region_eligibility=args.native_region_eligibility,
                  native_model_state=args.native_model_state,sound_requests=args.sound_requests,
                  initial_pose_read_only=args.initial_pose_read_only,
                  runtime_targets_built=args.build_runtime,test_jobs=args.test_jobs)
    save()
    print(json.dumps({'status': 'validating', 'report': str(receipt)}), flush=True)
    code = subprocess.run(command, env=environment, stdout=subprocess.DEVNULL,
                          stderr=subprocess.STDOUT).returncode
    report.update(passed=code == 0, returncode=code)
    save()
    print(json.dumps({'passed': report['passed'], 'returncode': code, 'report': str(receipt)}), flush=True)
    return code


if __name__ == '__main__':
    raise SystemExit(main())
