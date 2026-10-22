#!/usr/bin/env python3
"""Build and exercise a complete coherent current-source campaign client.

Copy current source and selected original fixtures into owned disposable
storage. Use exact committed dependency archives, not dirty dependency trees.
The production CMake graph builds the actual native executable. Added CTests
only invoke the existing MCC-only runtime probe; no engine code, input state,
callbacks or results are substituted. No installed runtime is changed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIRECTORIES = ('src', 'include', 'cmake', 'shaders', 'tools', 'third_party', 'external', 'multiplayer', 'tests')
SQUADS = ('1st_hum_floor_mar', '1st_hum_johnson', '1st_cov_wv1_elt', '1st_cov_wv1_grt')


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--mcc-root', type=Path, required=True)
    ap.add_argument('--dependency-receipt', type=Path, required=True)
    ap.add_argument('--compiler', default='clang++')
    ap.add_argument('--jobs', type=int, default=2)
    ap.add_argument('--ticks', type=int, default=300)
    ap.add_argument('--timeout', type=int, default=300)
    ap.add_argument('--actor-components', action='store_true',
                    help='Also freeze/run original actor components and complete cold-world ownership checks')
    args = ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        ap.error('Run through tools/run_test.py')
    if args.jobs < 1 or not 1 <= args.ticks <= 1800 or not 1 <= args.timeout <= 600:
        ap.error('Require positive jobs,1..1800 ticks and1..600 seconds per runtime')
    compiler = shutil.which(args.compiler)
    if compiler is None:
        ap.error('Compiler unavailable')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    source = scratch / 'current-source'
    source.mkdir()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    args.mcc_root = args.mcc_root.resolve(strict=True)
    report = dict(scope=__doc__, tool_sha256=sha(Path(__file__)), scratch=str(scratch),
                  phase='copying current sources', passed=False, installed_runtime_changed=False,
                  retained_large_artifacts=[], engine_source_substitutions=[], runtime_results_supplied=False)
    report_path = args.output / 'current-source-cohort.json'

    def save():
        report_path.write_text(json.dumps(report, indent=2) + '\n')

    save()
    for name in ('CMakeLists.txt', 'AGENTS.md', '.gitignore'):
        shutil.copy2(ROOT / name, source / name)
    for name in SOURCE_DIRECTORIES:
        excluded = {'__pycache__', '.git'}
        if name == 'tools':
            excluded.add('reference')
        if name == 'tests':
            excluded.add('fixtures')
        shutil.copytree(ROOT / name, source / name, symlinks=True,
                        ignore=shutil.ignore_patterns(*excluded))
    for name in ('assets', 'resources', 'data', 'docs', '.tools'):
        (source / name).symlink_to(ROOT / name, target_is_directory=True)
    (source / 'tools/reference').symlink_to(ROOT / 'tools/reference', target_is_directory=True)
    fixture_root = source / 'tests/fixtures'
    fixture_root.mkdir()
    selected = 'campaign_activation_tu15'
    selected_fixtures = {selected, 'campaign_session_seed_order_tu15.json'}
    if args.actor_components:
        selected_fixtures.update(re.findall(r'\$\{CMAKE_CURRENT_SOURCE_DIR\}/tests/fixtures/([^"/]+)',
                                           (source / 'cmake/CampaignAiObjectDecisions.cmake').read_text()))
        selected_fixtures.update(re.findall(r'#include "fixtures/([^"]+)"',
                                           (source / 'tests/native_tu15_world_memory_tests.cpp').read_text()))
    for fixture in (ROOT / 'tests/fixtures').iterdir():
        if fixture.name == selected:
            shutil.copytree(fixture, fixture_root / selected)
        elif fixture.name in selected_fixtures:
            shutil.copy2(fixture, fixture_root / fixture.name)
        else:
            (fixture_root / fixture.name).symlink_to(fixture, target_is_directory=fixture.is_dir())
    if not (fixture_root / selected).is_dir():
        raise ValueError('Required original activation fixtures are absent')
    inputs = {}
    for name in ('CMakeLists.txt', 'AGENTS.md', '.gitignore'):
        inputs[name] = sha(source / name)
    for directory in SOURCE_DIRECTORIES:
        for path in (source / directory).rglob('*'):
            relative = path.relative_to(source)
            if relative.parts[:2] == ('tools', 'reference'):
                continue
            if relative.parts[:2] == ('tests', 'fixtures') and (len(relative.parts) < 3 or relative.parts[2] not in selected_fixtures):
                continue
            if path.is_file():
                inputs[str(relative)] = sha(path)
    changed = [name for name, digest in inputs.items() if sha(ROOT / name) != digest]
    if changed:
        report['phase'] = 'capture rejected before build'
        report['capture_changed'] = changed
        save()
        raise ValueError('Current source changed while capturing: ' + ','.join(changed[:10]))
    (args.output / 'source-inputs.json').write_text(json.dumps(inputs, indent=2, sort_keys=True) + '\n')
    report['source_inputs_sha256'] = sha(args.output / 'source-inputs.json')
    report['captured_source_files'] = len(inputs)
    report['nonselected_fixtures_are_shared_and_not_qualified'] = True
    report['phase'] = 'preparing pinned dependency archives'
    save()
    dependency = json.loads(args.dependency_receipt.read_text())
    datachannel = scratch / 'datachannel'
    datachannel.mkdir()
    for ordinal, row in enumerate(dependency['inputs']):
        archive = scratch / f'dependency-{ordinal}.tar'
        with archive.open('wb') as stream:
            subprocess.run(['git', '-C', row['repository'], 'archive', row['commit']], stdout=stream, check=True)
        if sha(archive) != row['archive_sha256']:
            raise ValueError('Committed dependency archive differs')
        destination = datachannel if ordinal == 0 else datachannel / 'deps' / Path(row['repository']).name
        destination.mkdir(parents=True, exist_ok=True)
        with tarfile.open(archive) as stream:
            stream.extractall(destination, filter='data')
    report['dependency_inputs'] = dependency['inputs']
    report['dependency_working_trees_modified'] = False
    additions = '\n# Disposable current-source campaign acceptance cases.\n'
    report['runtime_cases'] = []
    for ordinal, squad in enumerate(SQUADS, 1):
        result = args.output / f'runtime-{ordinal}.json'
        name = f'current_campaign_ai_{ordinal}'
        # Paths are CMake bracket arguments; values containing closing brackets
        # cannot be silently reinterpreted as extra CMake commands.
        values = [str(source / 'tools/test_campaign_ai_runtime.py'), str(args.mcc_root), squad, str(result)]
        if any(']]' in value for value in values):
            raise ValueError('Unsupported CMake path delimiter')
        additions += f'''add_test(NAME {name} COMMAND "${{Python3_EXECUTABLE}}"
  [[{values[0]}]] --runtime $<TARGET_FILE:halo2_native>
  --mcc-root [[{values[1]}]] --squad [[{squad}]] --ticks {args.ticks}
  --timeout {args.timeout} --report-output [[{result}]])
set_tests_properties({name} PROPERTIES TIMEOUT {args.timeout + 60})
'''
        report['runtime_cases'].append(dict(name=name, squad=squad, report=str(result)))
    cmake = source / 'CMakeLists.txt'
    original_cmake = cmake.read_text()
    cmake.write_text(original_cmake + additions)
    if cmake.read_text().removesuffix(additions) != original_cmake:
        raise ValueError('Acceptance registration is not an exact append')
    staged_inputs = dict(inputs)
    staged_inputs['CMakeLists.txt'] = sha(cmake)
    report['test_registration_delta'] = additions
    report['staged_cmake_sha256'] = sha(cmake)
    command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--source', str(source),
               '--scratch-root', str(scratch / 'builds'), '--output', str(args.output / 'validation'),
               '--target', 'halo2_native', '--target', 'campaign_completion_tu15_tests',
               '--target', 'campaign_session_seed_order_tu15_tests',
               '--filter', r'^original_campaign_activation_tu15$|^current_campaign_ai_[1-4]$|^native_campaign_session_seed_order_',
               '--jobs', str(args.jobs), '--test-jobs', '1', '--cmake-arg=-DCMAKE_CXX_COMPILER=' + compiler,
               '--cmake-arg=-DFETCHCONTENT_SOURCE_DIR_HALO2_DATACHANNEL=' + str(datachannel),
               '--cmake-arg=-DHALO2_BUNDLE_MOVIES=OFF', '--cmake-arg=-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld',
               '--cmake-arg=-DHALO2_CAMPAIGN_SEED_ORDER_MCC_MAP=' + str(args.mcc_root / 'halo2/h2_maps_win64_dx11/01b_spacestation.map')]
    if args.actor_components:
        command += ['--target', 'campaign_ai_object_decisions_tu15_tests']
        command += ['--target', 'native_tu15_world_memory_tests']
        filter_index = command.index('--filter') + 1
        command[filter_index] += '|^original_campaign_ai_object_decisions_tu15$|^native_tu15_world_memory_ownership$'
    for seed in (305419896, 2271560481):
        command += ['--retain-report', f'campaign-session-seed-{seed}.json']
    report['command'] = command
    report['phase'] = 'building and validating current native client'
    save()
    report['returncode'] = subprocess.call(command, cwd=source)
    report['snapshot_changed'] = [name for name, digest in staged_inputs.items() if sha(source / name) != digest]
    report['checkout_drift'] = [name for name, digest in inputs.items() if not (ROOT / name).is_file() or sha(ROOT / name) != digest]
    runtime_passed = []
    for case in report['runtime_cases']:
        path = Path(case['report'])
        case['executed'] = path.is_file()
        if path.is_file():
            result = json.loads(path.read_text())
            case['passed'] = result['passed']
            case['report_sha256'] = sha(path)
        runtime_passed.append(case.get('passed', False))
    test_log = args.output / 'validation/tests.log'
    text = test_log.read_text(errors='replace') if test_log.exists() else ''
    report['original_activation_passed'] = bool(re.search(r'original_campaign_activation_tu15\s+\.+\s+Passed', text))
    report['session_seed_tests_passed'] = all(re.search(r'native_campaign_session_seed_order_' + str(seed) + r'\s+\.+\s+Passed', text)
        for seed in (305419896, 2271560481))
    report['actor_components_requested'] = args.actor_components
    report['actor_components_passed'] = (bool(re.search(r'original_campaign_ai_object_decisions_tu15\s+\.+\s+Passed', text))
                                         if args.actor_components else None)
    report['cold_world_ownership_passed'] = (bool(re.search(r'native_tu15_world_memory_ownership\s+\.+\s+Passed', text))
                                           if args.actor_components else None)
    report['passed'] = (report['returncode'] == 0 and not report['snapshot_changed'] and all(runtime_passed)
                        and report['original_activation_passed'] and report['session_seed_tests_passed']
                        and (not args.actor_components or
                             (report['actor_components_passed'] and report['cold_world_ownership_passed'])))
    report['phase'] = 'completed'
    report['full_campaign_or_coop_qualified'] = False
    save()
    print(json.dumps(dict(passed=report['passed'], returncode=report['returncode'], report=str(report_path))))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
