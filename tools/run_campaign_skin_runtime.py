#!/usr/bin/env python3
"""Exact fresh-process CPU/GPU campaign reference pair; run via tools/run_test.py.

This declares neutral input, a finite caller interval and paired render fraction.
It never supplies actors, nodes, AI output or an alternative source scheduler.
--squad is an explicitly labelled real authored-placement fixture; without it
the actual --campaign startup must succeed unaided. No zero-GPU run can pass.
All native logs, PPMs, traces, settings and timing files are private and removed.
Only the explicitly requested bounded JSON receipt is retained.
"""
import argparse
from collections import Counter
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import struct
import sys
import tempfile
import time

sys.dont_write_bytecode = True
from weapon_test_cache import require_mcc_map

LIMIT = 64 * 1024 * 1024
SOURCES = (
    'src/app/main.cpp', 'src/app/campaign_reference_capture.hpp',
    'include/halo2/campaign_source_runtime.hpp', 'src/campaign/campaign_source_runtime.cpp',
    'src/gameplay/simulation.cpp', 'src/physics/physics_live_world_tu15.cpp',
    'src/gameplay/multiplayer/multiplayer_object_runtime.cpp',
    'src/campaign/source_campaign_model.cpp', 'include/halo2/source_campaign_model.hpp',
    'include/halo2/model_skin_submission.hpp', 'include/halo2/model_skin_gpu_admission.hpp',
    'include/halo2/renderer_timing.hpp', 'src/rendering/renderer/renderer.cpp',
    'src/rendering/renderer/frame_model_skin_plan.hpp', 'src/rendering/models/model_skin_gpu.cpp',
    'src/rendering/models/model_skin_cpu_materialization.hpp', 'shaders/model_skin.comp',
    'tools/run_campaign_skin_runtime.py',
    'tests/campaign_reference_capture_driver_tests.py', 'cmake/CampaignReferenceCapture.cmake',
)
CONTROL_ENV = (
    'HALO2_PSTENCIL_CAMPAIGN_CAPTURES', 'HALO2_PSTENCIL_CAMPAIGN_CAPTURE_TICK_RANGE',
    'HALO2_PSTENCIL_CAMPAIGN_MATERIAL_CONTROLS', 'HALO2_PSTENCIL_CAMPAIGN_MODE_CONTROLS',
    'HALO2_PSTENCIL_CAMPAIGN_PROJECTED_CONTROLS', 'HALO2_PSTENCIL_CAMPAIGN_SHADOW_CONTROLS',
    'HALO2_PSTENCIL_CAMPAIGN_TRACE', 'HALO2_REFERENCE_TICK_CAMERA',
    'HALO2_TEST_CAMPAIGN_WEAPON_GPU_REPORT', 'HALO2_TEST_GPU_SKIN_PREPARE_FAILURE',
    'HALO2_REFERENCE_CPU_SKINNING', 'HALO2_RENDER_TIMINGS',
)


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def digest(data):
    return hashlib.sha256(data).hexdigest()


def bounded_read(path):
    if path.stat().st_size > LIMIT:
        raise RuntimeError(f'Capture output exceeds {LIMIT} bytes: {path.name}')
    return path.read_bytes()


def stop_process(process):
    if process.poll() is None:
        try:
            process.terminate()
        except ProcessLookupError:
            pass
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=10)
    return process.returncode


def normalized(events):
    # The ordered source/actor/node/complete-geometry receipts are comparable.
    # Dispatch receipts deliberately differ between CPU and GPU implementations.
    return [{k: v for k, v in event.items() if k != 'sequence'} for event in events
            if event.get('event') not in ('renderer', 'actor_gpu_submission')]


def sequence_digest(events):
    return digest(json.dumps(events, sort_keys=True, separators=(',', ':')).encode())


def configuration_errors(configuration, args):
    float32_bits = lambda value: struct.unpack('<I', struct.pack('<f', value))[0]
    expected = dict(schema='H2_CAMPAIGN_SKIN_REFERENCE_V1', frames=args.frames, neutral_input=True,
                    interval_bits=float32_bits(args.interval), render_fraction_bits=float32_bits(args.fraction),
                    primary_seed=0x12345678, auxiliary_seed=args.auxiliary_seed, width=args.width, height=args.height,
                    place_squad_fixture=args.squad or '', natural_startup=not bool(args.squad),
                    scenario_start_requested=not bool(args.squad))
    return [f'Capture configuration declaration differs: {key}' for key, value in expected.items()
            if type(configuration.get(key)) is not type(value) or configuration.get(key) != value]


def log_errors(output):
    errors = []
    if b'Vulkan validation:' in output:
        errors.append('Actual Vulkan validation messages were reported')
    if b'cannot allocate memory in static TLS block' in output:
        errors.append('Actual loader reported static TLS exhaustion; recovery cannot grant acceptance')
    return errors


def construction_errors(construction, args):
    if len(construction) != 1:
        return ['Missing actual before-scenario-factories construction receipt']
    state = construction[0].get('state', {})
    words = state.get('rng_words')
    # Original new-map noise consumes the PRIMARY stream after its admitted
    # options seed is delivered. Observe that advanced word, never reseed it
    # or assert it still equals the original external input.
    if state.get('game_options_seed') != 0x12345678 or not isinstance(words, list) or len(words) != 2 or \
            not all(type(word) is int and 0 <= word <= 0xffffffff for word in words) or words[1] != args.auxiliary_seed:
        return ['Actual construction game-options primary seed/auxiliary RNG readback differs from declared owners']
    return []


def valid_state(state):
    if not isinstance(state, dict):
        return False
    words = state.get('rng_words')
    tick = state.get('source_tick')
    encoded = state.get('clock_24_hex')
    if not isinstance(words, list) or len(words) != 2 or not all(type(word) is int and 0 <= word <= 0xffffffff for word in words) or \
            type(tick) is not int or not 0 <= tick <= 0xffffffff or not isinstance(encoded, str) or len(encoded) != 72 or \
            not all(type(state.get(key)) is int and state[key] > 0 for key in ('rng_address', 'clock_address')):
        return False
    try:
        clock = bytes.fromhex(encoded)
    except ValueError:
        return False
    return len(clock) == 0x24 and tick == int.from_bytes(clock[8:12], 'little')


def source_stage_errors(events, args):
    expected = [('before_scenario_factories', None), ('after_scenario_factories', None), ('before_frames', 0)]
    for frame in range(args.frames):
        expected += [(stage, frame) for stage in ('frame_begin', 'simulation_end', 'before_render', 'after_render')]
    states = [e for e in events if e.get('event') == 'source_state']
    actual = [(e.get('stage'), e.get('frame')) for e in states]
    if actual != expected or any('frame' in e and type(e['frame']) is not int for e in states):
        return ['Actual ordered construction-to-frame source-state stages differ']
    return []


def qualify(events, args, role):
    errors = []
    configuration = [e for e in events if e.get('event') == 'configuration']
    if len(configuration) != 1:
        errors.append('Missing exact finite neutral-capture configuration receipt')
    else:
        errors += configuration_errors(configuration[0], args)
    construction = [e for e in events if e.get('event') == 'source_state' and
                    e.get('stage') == 'before_scenario_factories']
    errors += construction_errors(construction, args)
    states = [e for e in events if e.get('event') == 'source_state' or e.get('event') == 'main_view']
    if any(not valid_state(e.get('state')) for e in states):
        errors.append('Incomplete or inconsistent actual two-word RNG/0x24 source-clock receipt')
    errors += source_stage_errors(events, args)
    if [e.get('sequence') for e in events] != list(range(len(events))):
        errors.append('Trace sequence is incomplete, duplicated or reordered')
    render = [e for e in events if e.get('event') == 'renderer']
    if len(render) != args.frames or [e.get('frame') for e in render] != list(range(args.frames)) or \
            any(e.get('submitted') is not True or e.get('measured') is not True or
                e.get('width') != args.width or e.get('height') != args.height for e in render):
        errors.append('Actual submitted measured frame dimensions/count differ')
    for stage in ('frame_begin', 'simulation_end', 'before_render', 'after_render'):
        rows = [e for e in states if e.get('stage') == stage]
        if [e.get('frame') for e in rows] != list(range(args.frames)):
            errors.append(f'Missing exact per-frame {stage} source receipt')
    actual_ticks = [e.get('state', {}).get('source_tick') for e in states]
    if any(type(tick) is not int or not 0 <= tick <= 0xffffffff for tick in actual_ticks) or \
            not any(0 < ((b - a) & 0xffffffff) < 0x80000000 for a, b in zip(actual_ticks, actual_ticks[1:])
                    if type(a) is int and type(b) is int):
        errors.append('Actual original clock+8 did not advance across the finite capture')
    for kind in ('actors', 'main_view'):
        if [e.get('frame') for e in events if e.get('event') == kind] != list(range(args.frames)):
            errors.append(f'Missing actual ordered {kind} frame trace')
    poses = [e for e in events if e.get('event') == 'actor_pose']
    geometries = [e for e in events if e.get('event') == 'actor_geometry']
    if not poses or len(poses) != len(geometries) or \
            [(e.get('frame'), e.get('owner')) for e in poses] != [(e.get('frame'), e.get('owner')) for e in geometries] or \
            not any(e.get('vertices', 0) > 0 for e in geometries):
        errors.append('No complete real ordinary campaign actor pose/geometry handoff')
    if any(not e.get('WORLD_node_bits') or not e.get('sampled_node_bits') or not e.get('ordinary_main_units') for e in poses):
        errors.append('Actor pose did not retain its actual WORLD/sample/main-unit inputs')
    declarations = [e for e in events if e.get('event') == 'actor_gpu_submission']
    if [(e.get('frame'), e.get('owner')) for e in declarations] != [(e.get('frame'), e.get('owner')) for e in poses]:
        errors.append('Actual actor dispatch declarations do not cover the ordered pose handoffs')
    if any(e.get('cpu_fallback_jobs') or e.get('cpu_fallback_vertices') or e.get('cpu_fallback_deformed_vertices') or
           e.get('cpu_fallback_post_identity_vertices') or e.get('cpu_fallback_vulkan_result') or
           e.get('cpu_fallback_quota_jobs') or e.get('cpu_fallback_allocation_jobs') or e.get('cpu_fallback_mapping_jobs')
           for e in render):
        errors.append('Resource CPU fallback cannot count as ordinary GPU acceptance')
    if role == 'cpu':
        if any(e.get('gpu_jobs') or e.get('gpu_vertices') or e.get('gpu_deformed_vertices') or
               e.get('gpu_verified_vertices') or e.get('gpu_post_identity_vertices') for e in render) or \
                any(e.get('records') or e.get('weighted_records') for e in declarations):
            errors.append('CPU reference unexpectedly submitted deferred/GPU skin work')
    else:
        if not any(e.get('weighted_records', 0) > 0 for e in declarations):
            errors.append('Candidate supplied no real weighted campaign actor GPU records')
        for frame in range(args.frames):
            rows = [e for e in declarations if e.get('frame') == frame]
            expected = sum(e.get('records', 0) for e in rows)
            weighted = sum(e.get('weighted_records', 0) for e in rows)
            actual = next((e for e in render if e.get('frame') == frame), {})
            if expected:
                if not actual.get('gpu_jobs') or actual.get('gpu_vertices', 0) < expected or \
                        actual.get('gpu_deformed_vertices', 0) < weighted or \
                        actual.get('gpu_verified_vertices') != actual.get('gpu_vertices'):
                    errors.append(f'Frame {frame}: real declared actor records lack complete GPU dispatch/readback')
            elif actual.get('gpu_vertices'):
                errors.append(f'Frame {frame}: GPU records have no declared ordinary actor owner')
    return errors


def run_role(args, role, parent):
    with tempfile.TemporaryDirectory(prefix=f'campaign-{role}-', dir=parent) as temporary:
        work = Path(temporary)
        trace, ppm, log = (work / name for name in ('trace.jsonl', 'frame.ppm', 'application.log'))
        for name in ('data', 'cache', 'config', 'tmp'):
            (work / name).mkdir()
        for name in ('pipeline', 'mesa', 'nvidia'):
            (work / 'cache' / name).mkdir()
        env = {k: v for k, v in os.environ.items() if k not in CONTROL_ENV}
        env.update(SDL_VIDEODRIVER=args.video_driver, SDL_AUDIODRIVER='dummy', HALO2_ENABLE_GPU_SKINNING='1',
                   HALO2_VERIFY_GPU_SKINNING='1', HALO2_RENDER_TIMINGS=str(work / 'timings.tsv'),
                   XDG_DATA_HOME=str(work / 'data'), XDG_CACHE_HOME=str(work / 'cache'),
                   XDG_CONFIG_HOME=str(work / 'config'), TMPDIR=str(work / 'tmp'), TMP=str(work / 'tmp'),
                   TEMP=str(work / 'tmp'), HALO2_PIPELINE_CACHE_DIRECTORY=str(work / 'cache' / 'pipeline'),
                   MESA_SHADER_CACHE_DIR=str(work / 'cache' / 'mesa'),
                   __GL_SHADER_DISK_CACHE_PATH=str(work / 'cache' / 'nvidia'))
        if role == 'cpu' or args.force_cpu_candidate:
            env['HALO2_REFERENCE_CPU_SKINNING'] = '1'
        command = [str(args.native), '--map', str(args.map), '--shared-maps', str(args.map.parent),
                   '--frames', str(args.frames), '--viewport', str(args.width), str(args.height),
                   '--render-size', str(args.width), str(args.height),
                   '--mute', '--validation', '--graphics-mode', 'Original',
                   '--screenshot', str(ppm), '--campaign-reference-capture', str(args.interval),
                   str(args.fraction), str(args.auxiliary_seed), str(trace)]
        if args.squad:
            command += ['--play', '--place-squad', args.squad]
        else:
            command += ['--campaign']
        started = time.monotonic()
        code, timed_out = None, False
        with log.open('wb') as output:
            # Inherit the enclosing run_test.py owned process group. It owns
            # cancellation/timeout cleanup of every child; do not escape it.
            process = subprocess.Popen(command, env=env, stdout=output, stderr=subprocess.STDOUT)
            try:
                code = process.wait(timeout=args.timeout)
            except subprocess.TimeoutExpired:
                timed_out = True
                code = stop_process(process)
            finally:
                # Also stop on KeyboardInterrupt or another wait exception,
                # before TemporaryDirectory retires the live source files.
                if process.poll() is None:
                    stop_process(process)
        errors = []
        if code != 0 or timed_out:
            errors.append(f'Actual campaign process failed: exit={code}, timeout={timed_out}')
        output = bounded_read(log)
        validation_messages = output.count(b'Vulkan validation:')
        errors += log_errors(output)
        records, frame = [], b''
        if trace.exists():
            records = [json.loads(line) for line in bounded_read(trace).splitlines()]
        else:
            errors.append('Actual campaign trace is missing')
        if ppm.exists():
            frame = bounded_read(ppm)
        header = f'P6\n{args.width} {args.height}\n255\n'.encode()
        if len(frame) != len(header) + args.width * args.height * 3 or not frame.startswith(header):
            errors.append('Complete requested framebuffer PPM is missing or malformed')
        errors += qualify(records, args, role)
        timing_path = work / 'timings.tsv'
        extents = []
        if timing_path.exists():
            for row in csv.DictReader(bounded_read(timing_path).decode().splitlines(), delimiter='\t'):
                extents.append({key: int(row[key]) for key in
                                ('render_width', 'render_height', 'present_width', 'present_height')})
        if len(extents) != args.frames or any(e != dict(render_width=args.width, render_height=args.height,
                present_width=args.width, present_height=args.height) for e in extents):
            errors.append('Actual renderer/presentation extent receipt differs from the requested native resolution')
        timing = [e for e in records if e.get('event') == 'renderer']
        receipt = dict(role=role, command=command, exit_code=code, timed_out=timed_out,
                       elapsed_seconds=time.monotonic() - started, errors=errors,
                       validation_messages=validation_messages, device_names=sorted({e.get('device', '') for e in timing}),
                       events=len(records), event_counts=dict(Counter(e.get('event') for e in records)),
                       submitted_frames=sum(e.get('submitted') is True for e in timing),
                       gpu_jobs=sum(e.get('gpu_jobs', 0) for e in timing),
                       gpu_vertices=sum(e.get('gpu_vertices', 0) for e in timing),
                       gpu_weighted_vertices=sum(e.get('gpu_deformed_vertices', 0) for e in timing),
                       gpu_verified_vertices=sum(e.get('gpu_verified_vertices', 0) for e in timing),
                       cpu_fallback_jobs=sum(e.get('cpu_fallback_jobs', 0) for e in timing),
                       complete_ppm_bytes=len(frame), complete_ppm_sha256=digest(frame),
                       ordered_trace_sha256=sequence_digest(normalized(records)),
                       raw_trace_sha256=sha(trace) if trace.exists() else None,
                       native_render_and_present_extents=extents,
                       timing_report_sha256=sha(timing_path) if timing_path.exists() else None,
                       source_tick_receipts=[dict(stage=e.get('stage', e.get('event')), frame=e.get('frame'),
                            tick=e['state']['source_tick']) for e in records if 'source_tick' in e.get('state', {})],
                       application_log_sha256=digest(output), application_log_tail=output[-8192:].decode(errors='replace'))
        return receipt, frame, normalized(records)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--expected-native-sha256', required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--video-driver', choices=('wayland', 'x11'), default=os.environ.get('SDL_VIDEODRIVER'),
                        help='Explicit SDL backend, defaults to the inherited provider setting; never starts/substitutes a display')
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--frames', type=int, default=60)
    parser.add_argument('--width', type=int, default=640)
    parser.add_argument('--height', type=int, default=480)
    parser.add_argument('--interval', type=float, default=1 / 30)
    parser.add_argument('--fraction', type=float, default=.5)
    parser.add_argument('--auxiliary-seed', type=int, default=1)
    parser.add_argument('--timeout', type=float, default=300)
    parser.add_argument('--squad', help='Explicit authored placement fixture; never natural mission startup proof')
    parser.add_argument('--force-cpu-candidate', action='store_true', help='Negative control: must exit nonzero for zero real GPU work')
    args = parser.parse_args()
    if not 1 <= args.frames <= 600 or not 64 <= args.width <= 4096 or not 64 <= args.height <= 4096 or \
            not math.isfinite(args.interval) or not 0 < args.interval <= .05 or \
            not math.isfinite(args.fraction) or not 0 <= args.fraction <= 1 or \
            not 0 <= args.auxiliary_seed <= 0xffffffff or not math.isfinite(args.timeout) or not 0 < args.timeout <= 900:
        parser.error('Finite frame/dimension/interval/fraction/uint32 seed/timeout bounds are required')
    args.native = args.native.resolve(strict=True)
    args.map = require_mcc_map(args.map)
    args.source_root = args.source_root.resolve(strict=True)
    args.report = args.report.resolve()
    if args.report.suffix != '.json' or args.report.exists():
        parser.error('Choose a new bounded JSON receipt path; existing evidence is never overwritten')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.video_driver not in ('wayland', 'x11'):
        parser.error('Declare --video-driver or inherit SDL_VIDEODRIVER from the actual display provider')
    if args.video_driver == 'x11' and not os.environ.get('DISPLAY'):
        parser.error('The declared X11 provider has no DISPLAY')
    if args.video_driver == 'wayland' and (not os.environ.get('WAYLAND_DISPLAY') or not os.environ.get('XDG_RUNTIME_DIR')):
        parser.error('The declared Wayland provider has no WAYLAND_DISPLAY/XDG_RUNTIME_DIR')
    if sha(args.native) != args.expected_native_sha256:
        parser.error('Native executable differs from the caller-declared hash pin')
    inputs = {str(args.map): sha(args.map), str(args.native): sha(args.native)}
    for name in ('shared.map', 'single_player_shared.map'):
        path = require_mcc_map(args.map.parent / name)
        inputs[str(path)] = sha(path)
    source_hashes = {name: sha(args.source_root / name) for name in SOURCES}
    report = dict(schema='H2_CAMPAIGN_SKIN_RUNTIME_PAIR_V1', passed=False,
                  scope='Exact finite neutral ordinary-actor campaign frame/source pair; not a playthrough or Xbox framebuffer comparison',
                  natural_startup=not bool(args.squad), place_squad_fixture=args.squad,
                  ordinary_gpu_acceptance=False, negative_control=args.force_cpu_candidate,
                  requested=dict(frames=args.frames, width=args.width, height=args.height, interval=args.interval,
                                 render_fraction=args.fraction, primary_seed=0x12345678, auxiliary_seed=args.auxiliary_seed),
                  native_sha256=args.expected_native_sha256, input_hashes=inputs, source_hashes=source_hashes,
                  display_provider=dict(video_driver=args.video_driver, DISPLAY=os.environ.get('DISPLAY'),
                                        WAYLAND_DISPLAY=os.environ.get('WAYLAND_DISPLAY'), XDG_RUNTIME_DIR=os.environ.get('XDG_RUNTIME_DIR')),
                  large_artifacts_retained=[], scratch_removed=False)
    with tempfile.TemporaryDirectory(prefix='campaign-skin-pair-', dir=scratch) as temporary:
        owned = Path(temporary)
        try:
            cpu, cpu_frame, cpu_trace = run_role(args, 'cpu', owned)
            report['cpu'] = cpu
            gpu, gpu_frame, gpu_trace = run_role(args, 'gpu', owned)
            report['gpu'] = gpu
            report['complete_ppm_identical'] = bool(cpu_frame and cpu_frame == gpu_frame)
            report['ordered_trace_identical'] = bool(cpu_trace and cpu_trace == gpu_trace)
            if cpu_trace != gpu_trace:
                first = next((i for i, pair in enumerate(zip(cpu_trace, gpu_trace)) if pair[0] != pair[1]), min(len(cpu_trace), len(gpu_trace)))
                report['first_trace_difference'] = dict(index=first,
                    cpu_event=cpu_trace[first].get('event') if first < len(cpu_trace) else None,
                    gpu_event=gpu_trace[first].get('event') if first < len(gpu_trace) else None,
                    cpu_record_preview=json.dumps(cpu_trace[first], sort_keys=True)[:8192] if first < len(cpu_trace) else None,
                    gpu_record_preview=json.dumps(gpu_trace[first], sort_keys=True)[:8192] if first < len(gpu_trace) else None)
            unchanged = all(sha(Path(path)) == value for path, value in inputs.items()) and \
                all(sha(args.source_root / path) == value for path, value in source_hashes.items())
            report['inputs_and_sources_unchanged'] = unchanged
            pair_passed = bool(unchanged and not cpu['errors'] and not gpu['errors'] and
                               report['complete_ppm_identical'] and report['ordered_trace_identical'])
            report['negative_control_qualification_passed'] = bool(args.force_cpu_candidate and unchanged and
                not cpu['errors'] and gpu['errors'] == ['Candidate supplied no real weighted campaign actor GPU records'] and
                gpu['gpu_jobs'] == 0 and gpu['gpu_vertices'] == 0 and gpu['gpu_verified_vertices'] == 0 and
                report['complete_ppm_identical'] and report['ordered_trace_identical'])
            report['passed'] = bool(pair_passed and not args.force_cpu_candidate)
            report['ordinary_gpu_acceptance'] = report['passed']
        except Exception as error:
            report['failure'] = f'{type(error).__name__}: {error}'
    report['scratch_removed'] = not owned.exists()
    report['passed'] = bool(report['passed'] and report['scratch_removed'])
    report['ordinary_gpu_acceptance'] = report['passed']
    encoded = json.dumps(report, indent=2) + '\n'
    if len(encoded.encode()) > 1024 * 1024:
        raise RuntimeError('Campaign pair receipt exceeded its 1 MiB retention budget')
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(encoded)
    print(json.dumps({key: report.get(key) for key in ('passed', 'complete_ppm_identical', 'ordered_trace_identical', 'scratch_removed')}), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
