#!/usr/bin/env python3
"""Run ordinary skin cohorts in fresh processes under one owned test scratch.

Each native child still performs its complete CPU/GPU pair and exact gates.
Only bounded case evidence survives its temporary raw-output directory.
"""
import argparse
import csv
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
REPORT_LIMIT = 1024 * 1024
TAIL_LIMIT = 16 * 1024
SOURCES = (
    'tools/run_model_skin_runtime.py', 'cmake/ModelSkinRuntime.cmake',
    'tests/model_skin_runtime_tests.cpp',
    'tests/test_model_skin_runtime_extent_driver.py',
    'src/gameplay/multiplayer/multiplayer_runtime.cpp',
    'src/gameplay/multiplayer/multiplayer_world.cpp',
    'include/halo2/model_skin_submission.hpp',
    'include/halo2/model_skin_gpu_admission.hpp', 'include/halo2/renderer_timing.hpp',
    'src/rendering/models/model_skin_gpu.hpp',
    'src/rendering/models/model_skin_gpu.cpp',
    'src/rendering/models/model_skin_cpu_materialization.hpp',
    'src/rendering/models/model_skin_vertices.hpp',
    'src/rendering/renderer/frame_model_skin_plan.hpp',
    'src/rendering/renderer/renderer.cpp', 'shaders/model_skin.comp',
)
FALLBACK_REASONS = ('quota', 'packing', 'create', 'allocation', 'device-allocation', 'map', 'map-range')
GPU_COUNTERS = ('gpu_frames', 'gpu_jobs', 'gpu_vertices', 'gpu_weighted_vertices', 'gpu_verified_vertices', 'gpu_post_identity_vertices')
FALLBACK_COUNTERS = ('frames', 'jobs', 'vertices', 'weighted_vertices', 'post_identity_vertices', 'quota_jobs', 'allocation_jobs', 'mapping_jobs')


def sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        while block := stream.read(1024 * 1024):
            digest.update(block)
    return digest.hexdigest()


def source_hashes():
    return {name: sha256(ROOT / name) for name in SOURCES}


def binary_identity(path):
    value = path.stat()
    return value.st_dev, value.st_ino, value.st_size, value.st_mtime_ns, value.st_ctime_ns


def save(path, report):
    data = (json.dumps(report, indent=2) + '\n').encode()
    if len(data) > REPORT_LIMIT:
        raise RuntimeError('Aggregate skin runtime report exceeds 1 MiB')
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def cohorts():
    return [species + ('-fp-' if first_person else '-') + str(players) + 'p-' + action
            for first_person in (False, True) for players in (1, 2, 4)
            for species in ('spartan', 'elite') for action in ('neutral', 'fire')]


def parse_extent_tsv(path, submitted, width, height, present_width, present_height):
    if not path.is_file() or not 0 < path.stat().st_size <= 64 * 1024 * 1024:
        raise RuntimeError('Missing or oversized per-frame renderer extent evidence')
    with path.open(newline='') as stream:
        reader = csv.reader(stream, delimiter='\t')
        header = next(reader, [])
        names = ('frame', 'render_width', 'render_height', 'present_width', 'present_height')
        if any(header.count(name) != 1 for name in names):
            raise RuntimeError('Per-frame extent evidence omits or duplicates required integer columns')
        positions = [header.index(name) for name in names]
        count = 0
        for row in reader:
            if len(row) != len(header) or any(not re.fullmatch(r'[0-9]+', row[index]) for index in positions):
                raise RuntimeError('Per-frame extent evidence has an incomplete or noninteger row')
            values = [int(row[index]) for index in positions]
            if values != [count, width, height, present_width, present_height]:
                raise RuntimeError('Per-frame extent evidence has a gap or mismatched internal/presented dimensions')
            count += 1
    if type(submitted) is not int or count != submitted or count < 60:
        raise RuntimeError('Per-frame extent evidence does not cover every submitted frame')
    return dict(frames=count, first_frame=0, last_frame=count - 1, render_width=width, render_height=height,
                present_width=present_width, present_height=present_height, consecutive_frames=True,
                all_extents_match=True, tsv_bytes=path.stat().st_size, tsv_sha256=sha256(path))


def presented_ppm_evidence(path, width, height):
    header = f'P6\n{width} {height}\n255\n'.encode()
    payload = width * height * 3
    if not path.is_file() or path.stat().st_size != len(header) + payload:
        raise RuntimeError('Presented PPM has a missing, truncated or mismatched complete payload')
    with path.open('rb') as stream:
        if stream.read(len(header)) != header:
            raise RuntimeError('Presented PPM header differs from the requested presentation extent')
    return dict(ppm_header=header.decode(), complete_ppm_bytes=len(header) + payload,
                rgb_payload_bytes=payload, frame_sha256=sha256(path))


def strict_dual_extents(report, name, width, height, presentation_size, roles, raw_root=None):
    if presentation_size is None:
        return None  # Legacy same-extent reports keep their existing contract.
    present_width, present_height = presentation_size
    if report.get('dual_extent_mode') is not True or report.get('requested_width') != width or \
            report.get('requested_height') != height or report.get('requested_present_width') != present_width or \
            report.get('requested_present_height') != present_height or report.get('framebuffer_extent_kind') != 'presented':
        return 'Child omitted the explicit separate internal/presented extent contract'
    cases = report.get('cases', [])
    if len(cases) != 1 or cases[0].get('name') != name:
        return 'Dual-extent child does not contain its requested case'
    header = f'P6\n{present_width} {present_height}\n255\n'
    expected = dict(render_width=width, render_height=height, present_width=present_width, present_height=present_height)
    frame_hash = None
    for role, suffix in roles:
        result = cases[0].get(role, {})
        if result.get('dual_extent_mode') is not True or result.get('framebuffer_extent_kind') != 'presented' or \
                any(result.get(key) != value for key, value in expected.items()) or \
                result.get('ppm_header') != header or result.get('rgb_payload_bytes') != present_width * present_height * 3 or \
                result.get('complete_ppm_bytes') != len(header.encode()) + present_width * present_height * 3:
            return 'Child confused internal render size with complete presented PPM evidence'
        receipt = result.get('frame_extent_evidence', {})
        frames = result.get('submitted_frames')
        current_hash = result.get('frame_sha256', '')
        if not re.fullmatch(r'[0-9a-f]{64}', str(current_hash)) or (frame_hash is not None and current_hash != frame_hash):
            return 'Dual-extent complete presented frame hashes differ or are missing'
        frame_hash = current_hash
        if type(frames) is not int or frames < 60 or type(receipt.get('frames')) is not int or receipt['frames'] != frames or \
                type(receipt.get('first_frame')) is not int or receipt['first_frame'] != 0 or \
                type(receipt.get('last_frame')) is not int or receipt['last_frame'] != frames - 1 or \
                receipt.get('consecutive_frames') is not True or receipt.get('all_extents_match') is not True or \
                any(receipt.get(key) != value for key, value in expected.items()) or \
                type(receipt.get('tsv_bytes')) is not int or receipt['tsv_bytes'] <= 0 or \
                not re.fullmatch(r'[0-9a-f]{64}', str(receipt.get('tsv_sha256', ''))):
            return 'Child omitted complete consecutive per-frame internal/presented extent evidence'
        if raw_root is not None:
            directory = raw_root / (name + '-' + suffix)
            try:
                actual = parse_extent_tsv(directory / 'render-timings.tsv', frames, width, height, present_width, present_height)
                ppm = presented_ppm_evidence(directory / 'final.ppm', present_width, present_height)
                if actual != receipt or any(result.get(key) != value for key, value in ppm.items()):
                    return 'Child extent or PPM receipt differs from its actual owned evidence'
            except (OSError, ValueError, RuntimeError) as error:
                return str(error)
    return None


def strict_case(report, name, width, height, presentation_size=None, raw_root=None):
    if report.get('passed') is not True or report.get('control_only'):
        return 'Child did not grant ordinary GPU acceptance'
    cases = report.get('cases')
    if not isinstance(cases, list) or len(cases) != 1 or cases[0].get('name') != name:
        return 'Child report does not contain exactly its requested cohort'
    case = cases[0]
    if case.get('passed') is not True or case.get('control_only') or \
            case.get('full_frame_identical') is not True or case.get('ordered_trace_identical') is not True:
        return 'Child omitted an exact full-frame/ordered-trace acceptance gate'
    for role in ('cpu', 'gpu'):
        result = case.get(role, {})
        if result.get('input_callbacks') != 60 or result.get('submitted_frames', 0) < 60 or \
                result.get('render_width') != width or result.get('render_height') != height or \
                result.get('validation_messages') != 0:
            return 'Child omitted actual finite inputs, submitted extent, or validation evidence'
    if any(case['cpu'].get(key) != 0 for key in ('gpu_frames', 'gpu_jobs', 'gpu_vertices', 'gpu_weighted_vertices',
                                              'gpu_verified_vertices', 'gpu_post_identity_vertices')):
        return 'CPU counterfactual dispatched or verified GPU skin work'
    gpu = case['gpu']
    if not gpu.get('gpu_frames') or not gpu.get('gpu_jobs') or not gpu.get('gpu_weighted_vertices') or \
            not gpu.get('gpu_vertices') or gpu.get('gpu_vertices') != gpu.get('gpu_verified_vertices'):
        return 'Child omitted actual weighted GPU work or complete vertex verification'
    if '-fp-' in name and not gpu.get('gpu_post_identity_vertices'):
        return 'First-person child omitted actual GPU post-identity vertices'
    return strict_dual_extents(report, name, width, height, presentation_size, (('cpu', 'cpu'), ('gpu', 'gpu')), raw_root)


def strict_fallback_case(report, name, width, height, reason, presentation_size=None, raw_root=None):
    if report.get('qualification_kind') != 'forced-cpu-materialization' or report.get('passed') is not False or \
            report.get('control_only') is not True or report.get('ordinary_gpu_acceptance') is not False or \
            report.get('fallback_qualification_passed') is not True or report.get('forced_prepare_failure') != reason:
        return 'Child did not grant the explicitly requested separate fallback qualification'
    cases = report.get('cases')
    if not isinstance(cases, list) or len(cases) != 1 or cases[0].get('name') != name:
        return 'Fallback child report does not contain exactly its requested cohort'
    case = cases[0]
    if case.get('passed') is not False or case.get('control_only') is not True or \
            case.get('fallback_qualification_passed') is not True or case.get('forced_prepare_failure') != reason or \
            case.get('full_frame_identical') is not True or case.get('ordered_trace_identical') is not True:
        return 'Fallback child omitted its separate exact frame/trace qualification gates'
    cpu, candidate = case.get('cpu', {}), case.get('forced_fallback', {})
    present_width, present_height = presentation_size or (width, height)
    extent = len(f'P6\n{present_width} {present_height}\n255\n'.encode()) + present_width * present_height * 3
    for result in (cpu, candidate):
        if result.get('input_callbacks') != 60 or result.get('actual_delivered_commands_per_view') != 60 or \
                result.get('submitted_frames', 0) < 60 or result.get('render_width') != width or \
                result.get('render_height') != height or result.get('validation_messages') != 0 or \
                result.get('complete_ppm_bytes') != extent or any(result.get(key) != 0 for key in GPU_COUNTERS):
            return 'Fallback child omitted real finite inputs/frame extent or dispatched/verified GPU work'
        rng = result.get('source_rng_frames', {})
        if rng.get('callbacks') != result['submitted_frames'] or result.get('source_rng_construction', {}).get('auxiliary') != 1:
            return 'Fallback child omitted actual pinned construction/per-frame source RNG evidence'
    if not cpu.get('frame_sha256') or cpu.get('frame_sha256') != candidate.get('frame_sha256') or \
            not cpu.get('ordered_callback_sha256') or cpu.get('ordered_callback_sha256') != candidate.get('ordered_callback_sha256'):
        return 'Fallback complete frame/source-RNG callback hashes differ'
    for stage in ('first', 'last'):
        for key in ('primary', 'auxiliary'):
            if cpu['source_rng_frames'].get(stage, {}).get(key) is None or \
                    cpu['source_rng_frames'].get(stage, {}).get(key) != candidate['source_rng_frames'].get(stage, {}).get(key):
                return 'Fallback actual source RNG readback differs'
    baseline, actual = cpu.get('cpu_fallback', {}), candidate.get('cpu_fallback', {})
    if any(baseline.get(key) != 0 for key in FALLBACK_COUNTERS) or baseline.get('vulkan_results') != {}:
        return 'CPU reference unexpectedly used late materialization'
    if not actual.get('frames') or actual.get('jobs') != actual['frames'] or not actual.get('vertices') or \
            not actual.get('weighted_vertices') or actual['weighted_vertices'] > actual['vertices'] or \
            actual.get('post_identity_vertices', -1) < 0 or actual['post_identity_vertices'] > actual['vertices'] or \
            ('-fp-' in name and not actual['post_identity_vertices']):
        return 'Fallback child performed no actual weighted materialization/post-placement'
    group = 'quota_jobs' if reason == 'quota' else 'mapping_jobs' if reason in ('map', 'map-range') else 'allocation_jobs'
    if any(actual.get(key) != (actual['jobs'] if key == group else 0) for key in ('quota_jobs', 'allocation_jobs', 'mapping_jobs')):
        return 'Fallback child used another actual resource-refusal reason'
    result = 0 if reason in ('quota', 'packing') else -2 if reason == 'device-allocation' else -5 if reason == 'map-range' else -1
    if actual.get('vulkan_results') != {str(result): actual['frames']}:
        return 'Fallback child reported another actual Vulkan refusal result'
    return strict_dual_extents(report, name, width, height, presentation_size,
                               (('cpu', 'cpu'), ('forced_fallback', 'forced-fallback')), raw_root)


def retain_references(case, temporary, report_path, name):
    references = case.pop('retained_frame_references', None)
    if not references:
        return
    if os.environ.get('HALO2_MODEL_SKIN_RUNTIME_KEEP_MISMATCH_FRAMES') != '1':
        raise RuntimeError('Child retained frame references without explicit retention')
    retained = {'scope': 'Explicit required failed-pair references beside the aggregate report; all other child raw outputs are removed.'}
    case['retained_frame_references'] = retained
    for role in ('cpu', 'gpu'):
        receipt = references[role]
        source = Path(receipt['path']).resolve()
        if not source.is_relative_to(temporary.resolve()) or not source.is_file():
            raise RuntimeError('Child frame reference escaped its owned temporary directory')
        size = source.stat().st_size
        if size > 64 * 1024 * 1024 or size != receipt['bytes'] or sha256(source) != receipt['sha256']:
            raise RuntimeError('Child frame reference extent or hash does not match its receipt')
        for index in range(1024):
            target = report_path.with_name(report_path.name + f'.{name}.{role}.{index}.reference.ppm')
            try:
                output = target.open('xb')
            except FileExistsError:
                continue
            try:
                with output, source.open('rb') as original:
                    shutil.copyfileobj(original, output, 1024 * 1024)
                if target.stat().st_size != size or sha256(target) != receipt['sha256']:
                    raise RuntimeError('Required frame-reference copy changed bytes')
                break
            except Exception:
                target.unlink(missing_ok=True)
                raise
        else:
            raise RuntimeError('Allocate unique required frame-reference destination')
        retained[role] = {'path': str(target), 'bytes': size, 'sha256': sha256(target)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path)
    parser.add_argument('map', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('report', type=Path)
    parser.add_argument('--cohort', default='')
    parser.add_argument('--width', type=int, default=640)
    parser.add_argument('--height', type=int, default=480)
    parser.add_argument('--presentation-size', type=int, nargs=2, metavar=('W', 'H'),
                        help='Explicit presented framebuffer size, separate from internal --width/--height; validates every submitted frame')
    parser.add_argument('--fallback', default=os.environ.get('HALO2_MODEL_SKIN_RUNTIME_FALLBACK_CONTROL', ''),
                        help='Explicit separate forced CPU materialization qualification; ordinary GPU acceptance remains false')
    parser.add_argument('--child-timeout-seconds', type=float, default=180)
    parser.add_argument('--suite-timeout-seconds', type=float, default=1740)
    args = parser.parse_args()
    report = {'passed': False, 'status': 'initializing', 'cases': [], 'children': [],
              'control_only': bool(args.fallback) or os.environ.get('HALO2_MODEL_SKIN_RUNTIME_REFERENCE_ONLY') == '1',
              'scope': 'Fresh process per ordinary cohort, same native binary; each child retains its own complete CPU/GPU pair and exact acceptance gates. Temporary raw frames, source storage and logs are removed after each child.'}
    try:
        if args.fallback and (args.fallback not in FALLBACK_REASONS or os.environ.get('HALO2_MODEL_SKIN_RUNTIME_REFERENCE_ONLY') == '1'):
            raise RuntimeError('Fallback mode requires one of the seven recoverable reasons and excludes reference-only control')
        if not 64 <= args.width <= 4096 or not 64 <= args.height <= 4096:
            raise RuntimeError('Requested dimensions must each be in 64..4096')
        if args.presentation_size is not None and any(not 64 <= value <= 4096 for value in args.presentation_size):
            raise RuntimeError('Presentation dimensions must each be in 64..4096')
        if not 0 < args.child_timeout_seconds <= args.suite_timeout_seconds < 1800:
            raise RuntimeError('Require 0 < child timeout <= suite timeout < CTest 1800 seconds')
        scratch_value = os.environ.get('HALO2_TEST_SCRATCH')
        if not scratch_value:
            raise RuntimeError('Skin runtime suite requires owned HALO2_TEST_SCRATCH')
        scratch = Path(scratch_value).resolve(strict=True)
        args.output = args.output.resolve()
        args.report = args.report.resolve()
        if not args.output.is_relative_to(scratch):
            raise RuntimeError('Child raw outputs must remain inside owned scratch')
        args.output.mkdir(parents=True, exist_ok=True)
        args.binary = args.binary.resolve(strict=True)
        args.map = args.map.resolve(strict=True)
        binary_stat = binary_identity(args.binary)
        binary_hash = sha256(args.binary)
        hashes = source_hashes()
        selected = [name for name in cohorts() if args.cohort in name]
        if not selected:
            raise RuntimeError('Ordinary runtime cohort filter selected no cases')
        report.update(status='running', requested_width=args.width, requested_height=args.height,
                      requested_present_width=(args.presentation_size or (args.width, args.height))[0],
                      requested_present_height=(args.presentation_size or (args.width, args.height))[1],
                      dual_extent_mode=args.presentation_size is not None, framebuffer_extent_kind='presented',
                      extent_scope='Complete presented framebuffer equivalence at the separately requested internal render extent; no physical display mode, latency or FPS qualification.',
                      requested_cohorts=selected, cohort_filter=args.cohort,
                      binary={'path': str(args.binary), 'bytes': args.binary.stat().st_size, 'sha256': binary_hash},
                      source_hashes=hashes, coordinator_argv=sys.argv,
                      child_timeout_seconds=args.child_timeout_seconds, suite_timeout_seconds=args.suite_timeout_seconds)
        report['qualification_kind'] = 'forced-cpu-materialization' if args.fallback else 'ordinary-gpu'
        report['ordinary_gpu_acceptance'] = False
        if args.fallback:
            report.update(forced_prepare_failure=args.fallback, fallback_qualification_passed=False,
                          reason='Separate forced CPU materialization qualification; normal passed and ordinary GPU/pin acceptance remain false')
        if report['control_only']:
            report.update(actual_gpu_acceptance_run=False, reason='Reference-only child controls cannot grant GPU quality acceptance')
        save(args.report, report)
        deadline = time.monotonic() + args.suite_timeout_seconds
        for name in selected:
            if binary_identity(args.binary) != binary_stat:
                raise RuntimeError('Native binary changed during the suite')
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise RuntimeError('Skin runtime suite deadline expired before the next cohort')
            child = {'name': name, 'status': 'running', 'binary_sha256': binary_hash}
            report['children'].append(child)
            failure = None
            with tempfile.TemporaryDirectory(prefix=name + '-', dir=args.output) as directory:
                temporary = Path(directory)
                child_report = temporary / 'case.json'
                log = temporary / 'stdout.log'
                command = [str(args.binary), str(args.map), str(temporary / 'raw'), str(child_report), name,
                           str(args.width), str(args.height)]
                if args.presentation_size is not None:
                    command.extend(map(str, args.presentation_size))
                child.update(command=command, temporary_directory=directory, temporary_directory_removed=False)
                save(args.report, report)
                started = time.monotonic()
                timed_out = False
                environment = os.environ.copy()
                environment.update(HALO2_TEST_SCRATCH=directory, TMPDIR=directory, TMP=directory, TEMP=directory)
                if args.fallback:
                    environment['HALO2_MODEL_SKIN_RUNTIME_FALLBACK_CONTROL'] = args.fallback
                else:
                    environment.pop('HALO2_MODEL_SKIN_RUNTIME_FALLBACK_CONTROL', None)
                # Inherit tools/run_test.py's supervised process group.
                with log.open('wb') as stream:
                    process = subprocess.Popen(command, stdout=stream, stderr=subprocess.STDOUT, env=environment)
                    try:
                        code = process.wait(timeout=min(args.child_timeout_seconds, remaining))
                    except subprocess.TimeoutExpired:
                        timed_out = True
                        process.terminate()
                        try:
                            code = process.wait(timeout=5)
                        except subprocess.TimeoutExpired:
                            process.kill()
                            code = process.wait()
                    finally:
                        if process.poll() is None:
                            process.kill()
                            process.wait()
                child.update(exit_code=code, timed_out=timed_out, elapsed_seconds=time.monotonic() - started,
                             stdout_bytes=log.stat().st_size, stdout_sha256=sha256(log))
                # Preserve this known loader failure as a failure even if a
                # driver ignores its manifest and the native process returns.
                marker = b'cannot allocate memory in static TLS block'
                with log.open('rb') as stream:
                    overlap = b''
                    while block := stream.read(65536):
                        if marker in overlap + block:
                            failure = 'Vulkan loader: cannot allocate memory in static TLS block'
                            break
                        overlap = block[-len(marker):]
                payload = None
                if child_report.is_file():
                    if child_report.stat().st_size > REPORT_LIMIT:
                        failure = 'Child report exceeds 1 MiB'
                    else:
                        try:
                            payload = json.loads(child_report.read_text())
                        except (ValueError, OSError) as error:
                            failure = 'Cannot read child report: ' + str(error)
                else:
                    failure = 'Child omitted its bounded report'
                if payload is not None:
                    child['reported_passed'] = payload.get('passed')
                    child['control_only'] = bool(payload.get('control_only'))
                    if payload.get('control_only'):
                        report['control_only'] = True
                    for case in payload.get('cases', []):
                        report['cases'].append(case)
                        retain_references(case, temporary, args.report, name)
                    failure = failure or (strict_fallback_case(payload, name, args.width, args.height, args.fallback,
                                                             args.presentation_size, temporary / 'raw')
                                          if args.fallback else strict_case(payload, name, args.width, args.height,
                                                                          args.presentation_size, temporary / 'raw'))
                    if payload.get('error'):
                        child['error'] = payload['error']
                if timed_out:
                    failure = 'Native cohort exceeded its declared timeout'
                elif code:
                    failure = failure or 'Native cohort exited unsuccessfully'
                if failure:
                    child.update(status='timed_out' if timed_out else 'failed', failure=failure)
                    with log.open('rb') as stream:
                        stream.seek(max(0, log.stat().st_size - TAIL_LIMIT))
                        child['failure_log_tail'] = stream.read(TAIL_LIMIT).decode(errors='replace')
                    print(f'FAIL {name}: {failure} (exit {code})\n' + child['failure_log_tail'], file=sys.stderr, flush=True)
                else:
                    child['status'] = 'fallback-qualified' if args.fallback else 'passed'
                    if args.fallback:
                        actual = payload['cases'][0]['forced_fallback']['cpu_fallback']
                        print(f'FALLBACK_QUALIFIED {name}: exact frame/source-RNG trace, {actual["weighted_vertices"]} weighted CPU vertices ({args.fallback}); ordinary GPU acceptance false', flush=True)
                    else:
                        gpu = payload['cases'][0]['gpu']
                        print(f'PASS {name}: exact frame/trace, {gpu["gpu_weighted_vertices"]} weighted GPU vertices verified', flush=True)
            child['temporary_directory_removed'] = not Path(directory).exists()
            save(args.report, report)
            if failure:
                report.update(status='failed', error=failure)
                save(args.report, report)
                return 124 if timed_out else (code if code > 0 else 128 - code if code < 0 else 1)
        if binary_identity(args.binary) != binary_stat or sha256(args.binary) != binary_hash or source_hashes() != hashes:
            raise RuntimeError('Native binary or declared sources changed during the suite')
        if args.fallback:
            report.update(passed=False, status='fallback-qualified', fallback_qualification_passed=True,
                          ordinary_gpu_acceptance=False, actual_gpu_acceptance_run=False)
            save(args.report, report)
            return 0
        if report['control_only']:
            raise RuntimeError('Reference-only controls cannot grant ordinary GPU acceptance')
        report.update(passed=True, status='passed', ordinary_gpu_acceptance=True)
        save(args.report, report)
        return 0
    except Exception as error:
        for child in report['children']:
            if 'temporary_directory' in child:
                child['temporary_directory_removed'] = not Path(child['temporary_directory']).exists()
            if child['status'] == 'running':
                child.update(status='failed', failure=str(error))
        report.update(passed=False, status='failed', error=str(error))
        save(args.report, report)
        print('Skin runtime suite failed: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
