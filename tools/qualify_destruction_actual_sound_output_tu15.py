#!/usr/bin/env python3
"""Replay authenticated original destruction sound callers in an owned build.

CMake validation uses tools/run_tests.py. The native PCM/manual-output exercise
runs inside the enclosing tools/run_test.py owner. No raw audio is retained.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from refresh_destruction_current_cohort import sha

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cohort', type=Path, required=True)
    parser.add_argument('--refresh', type=Path, required=True)
    parser.add_argument('--summary', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--previous-output', type=Path,
                        help='Authenticate a prior declared audio-fixture refresh in this same active owner')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use a tools/run_test.py-owned finite command session')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    cohort, refresh, summary = (json.loads(path.read_text()) for path in
        (args.cohort, args.refresh, args.summary))
    if (Path(cohort['scratch']).resolve() != scratch or
            Path(refresh['scratch']).resolve() != scratch or
            refresh.get('prior_cohort') != str(args.cohort.resolve()) or
            not refresh.get('passed') or not refresh.get('sound_requests') or
            not summary.get('passed') or summary['refresh_sha256'] != sha(args.refresh)):
        raise ValueError('Require a passing actual caller journal in this active build owner')
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    source = scratch / 'source'
    report = dict(passed=False, scratch=str(scratch), tool_sha256=sha(__file__),
        cohort=str(args.cohort.resolve()), cohort_sha256=sha(args.cohort),
        refresh=str(args.refresh.resolve()), refresh_sha256=sha(args.refresh),
        summary=str(args.summary.resolve()), summary_sha256=sha(args.summary),
        retained_large_artifacts=[])
    receipt = args.output / 'qualification.json'

    def save():
        receipt.write_text(json.dumps(report, indent=2) + '\n')

    save()
    manifest = json.loads((args.cohort.parent / 'source/source-manifest.json').read_text())
    expected = dict(manifest['source_files_sha256'])
    expected.update(cohort['additional_frozen_inputs'])
    expected['CMakeLists.txt'] = refresh['registered_cmake_sha256']
    expected['tests/destruction_runtime_tu15_tests.cpp'] = refresh['fixture_refresh']['after']
    # The actual caller run can include declared, qualified production deltas
    # after the initial freeze. Authenticate its effective source, rather than
    # silently importing current workspace production into the audio build.
    for name, change in refresh.get('patches', {}).items():
        expected[name] = change['after']
    report['qualified_refresh_patches'] = refresh.get('patches', {})
    fixture = 'tests/vehicle_destruction_audio_playback_tu15_tests.cpp'
    if args.previous_output:
        previous = json.loads(args.previous_output.read_text())
        if (Path(previous['scratch']).resolve() != scratch or
                previous['refresh_sha256'] != sha(args.refresh) or
                set(previous['patches']) != {fixture}):
            raise ValueError('Previous audio refresh has a different source/build owner')
        expected[fixture] = previous['patches'][fixture]['after']
        report['previous_output'] = str(args.previous_output.resolve())
        report['previous_output_sha256'] = sha(args.previous_output)
    if cohort.get('private_build_partition'):
        part = cohort['private_build_partition']
        if part['path'] not in refresh.get('patches', {}):
            expected[part['path']] = part['after']
    drift = [name for name, digest in expected.items() if sha(source / name) != digest]
    report['frozen_source_drift'] = drift
    save()
    if drift:
        raise ValueError('Frozen production changed before the declared audio fixture refresh')
    report['patches'] = {fixture: dict(before=sha(source / fixture), after=sha(ROOT / fixture))}
    shutil.copyfile(ROOT / fixture, source / fixture)
    if (sha(source / fixture) != report['patches'][fixture]['after'] or
            sha(ROOT / fixture) != report['patches'][fixture]['after']):
        raise ValueError('Audio fixture changed while freezing its declared refresh')
    report['cmake_sha256'] = sha(source / 'CMakeLists.txt')
    cases, evidence, maps = [], {}, {}
    for name, row in summary['cases'].items():
        path = Path(row['evidence'])
        if sha(path) != row['evidence_sha256']:
            raise ValueError('Actual source caller evidence changed after its qualification')
        raw = json.loads(path.read_text())
        map_path = str(Path(raw['map']).resolve(strict=True))
        digest = refresh.get('runtime_inputs', {}).get('destruction_maps', {}).get(map_path)
        if digest is None:
            digest = cohort['runtime_inputs']['destruction_maps'].get(map_path)
        if not raw.get('passed') or not digest or sha(map_path) != digest:
            raise ValueError('Actual caller evidence lost its authenticated scenario owner')
        evidence[str(path.resolve())] = sha(path)
        maps[map_path] = digest
        cases.append(dict(case=name, map=map_path, requests=raw['original_sound_requests']))
    report['source_evidence_sha256'] = evidence
    report['runtime_maps_sha256'] = maps
    plan = args.output / 'actual-caller-plan.json'
    plan.write_text(json.dumps(dict(passed=True, mode='actual-original-sound-requests', cases=cases), indent=2) + '\n')
    if plan.stat().st_size > 1024 * 1024:
        raise ValueError('Actual caller plan exceeds its bounded report budget')
    report['plan'] = str(plan)
    report['plan_sha256'] = sha(plan)
    command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--build-worker',
        '--source', str(source), '--target', 'vehicle_destruction_pcm_resources_tu15_tests',
        '--build-only', '--jobs', '2', '--output', str(args.output / 'validation')]
    report['build_command'] = command
    save()
    code = subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT).returncode
    report['build_returncode'] = code
    save()
    if code:
        return code
    output = scratch / 'actual-sound-output.json'
    command = [str(scratch / 'build/vehicle_destruction_pcm_resources_tu15_tests'),
        '--actual-requests', str(plan), str(output)]
    report['output_command'] = command
    save()
    result = subprocess.run(command, env=dict(os.environ, SDL_AUDIODRIVER='dummy'),
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=300)
    (args.output / 'output.log').write_text(result.stdout[-65536:])
    report['output_returncode'] = result.returncode
    if output.is_file():
        if output.stat().st_size > 1024 * 1024:
            raise ValueError('Native actual caller output exceeds its bounded report budget')
        retained = args.output / output.name
        shutil.copyfile(output, retained)
        report['native_output_report'] = str(retained)
        report['native_output_sha256'] = sha(retained)
        data = json.loads(output.read_text())
        report['passed'] = result.returncode == 0 and data.get('passed') is True
    save()
    print(json.dumps(dict(passed=report['passed'], report=str(receipt))))
    return 0 if report['passed'] else (result.returncode or 1)


if __name__ == '__main__':
    raise SystemExit(main())
