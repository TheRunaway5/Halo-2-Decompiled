#!/usr/bin/env python3
"""Finite natural --campaign cached-decal pixel control under run_test.py.

Runs actual neutral original-owned campaign startup enabled/hidden/enabled.
No actors, decals, camera, hits, AI results or frames are supplied. Empty/black
views reject visible-decal acceptance; this is not a complete playthrough or
an Xbox framebuffer comparison. Only the bounded report is retained.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import time

sys.dont_write_bytecode = True
LIMIT = 64 * 1024 * 1024


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def read(path):
    if path.stat().st_size > LIMIT:
        raise RuntimeError(f'Actual capture exceeds {LIMIT} bytes: {path.name}')
    return path.read_bytes()


def ppm(path):
    data = read(path)
    match = re.match(rb'P6\s+(\d+)\s+(\d+)\s+255\s', data)
    if not match:
        raise RuntimeError('Actual native frame is not P6 RGB')
    width, height = map(int, match.groups())
    values = data[match.end():]
    if len(values) != width * height * 3:
        raise RuntimeError('Actual native frame is incomplete')
    return width, height, values


def changed(first, second):
    if first[:2] != second[:2]:
        raise RuntimeError('Actual pixel controls changed dimensions')
    return sum(first[2][n:n+3] != second[2][n:n+3] for n in range(0, len(first[2]), 3))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--runtime', type=Path, required=True)
    ap.add_argument('--expected-native-sha256', required=True)
    ap.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument('--map', type=Path, required=True)
    ap.add_argument('--shader-dir', type=Path, required=True)
    ap.add_argument('--video-driver', choices=('x11', 'wayland'), required=True)
    ap.add_argument('--frames', type=int, default=60)
    ap.add_argument('--width', type=int, default=640)
    ap.add_argument('--height', type=int, default=480)
    ap.add_argument('--timeout', type=float, default=300)
    ap.add_argument('--report', type=Path, required=True)
    args = ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        ap.error('Run through tools/run_test.py')
    if not 1 <= args.frames <= 600 or not all(64 <= n <= 4096 for n in (args.width, args.height)) or \
            not math.isfinite(args.timeout) or not 0 < args.timeout <= 900:
        ap.error('Finite frame/dimension/timeout bounds are required')
    args.runtime = args.runtime.resolve(strict=True)
    args.map = args.map.resolve(strict=True)
    args.source_root = args.source_root.resolve(strict=True)
    args.shader_dir = args.shader_dir.resolve(strict=True)
    args.report = args.report.resolve()
    if args.report.exists() or args.report.suffix != '.json':
        ap.error('Choose a new bounded JSON report path')
    if sha(args.runtime) != args.expected_native_sha256:
        ap.error('Native executable differs from declared hash pin')
    if args.video_driver == 'x11' and not os.environ.get('DISPLAY'):
        ap.error('Declared X11 provider has no DISPLAY')
    if args.video_driver == 'wayland' and not all(os.environ.get(n) for n in ('WAYLAND_DISPLAY', 'XDG_RUNTIME_DIR')):
        ap.error('Declared Wayland provider has no actual display')
    with args.map.open('rb') as stream:
        if struct.unpack('<II', stream.read(8)) != (0x68656164, 13):
            ap.error('Ordinary runtime control requires prepared MCC version13 map')
    sys.path.insert(0, str(args.source_root / 'tools'))
    from run_campaign_skin_runtime import configuration_errors, construction_errors, source_stage_errors, log_errors, normalized
    args.interval, args.fraction, args.auxiliary_seed, args.squad = 1/30, 0.5, 1, None
    shaders = sorted(args.shader_dir.rglob('*.spv'))
    if not shaders:
        ap.error('Declared configured shader directory is empty')
    inputs = [args.runtime, args.map, args.map.parent/'shared.map', args.map.parent/'single_player_shared.map', *shaders]
    pins = {str(p): sha(p) for p in inputs}
    source_paths = ['src/app/main.cpp', 'src/gameplay/simulation.cpp', 'include/halo2/source_effect_frame_tu15.hpp',
                    'src/rendering/effects/source_decal_draw.cpp', 'src/rendering/effects/source_decal_vertex_cache.cpp',
                    'src/rendering/renderer/renderer.cpp']
    source_pins = {p: sha(args.source_root/p) for p in source_paths}
    report = dict(passed=False, scope=__doc__, tool_sha256=sha(Path(__file__)), natural_campaign_startup=True,
                  pins=pins, source_hashes=source_pins, source_root=str(args.source_root),
                  shader_directory=str(args.shader_dir), shader_directory_override=False,
                  raw_frames_retained=False, full_goal_complete=False, runs=[], failures=[])
    images, source_traces, plans = [], [], []
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    work = None
    try:
        with tempfile.TemporaryDirectory(prefix='campaign-decals-', dir=scratch) as work_name:
            work = Path(work_name)
            for role in ('enabled', 'hidden', 'repeat'):
                owned = work/role; owned.mkdir()
                for directory in ('config', 'data', 'cache', 'tmp', 'pipeline', 'mesa', 'nvidia'):
                    (owned/directory).mkdir()
                frame, trace, log = (owned/name for name in ('frame.ppm', 'trace.jsonl', 'native.log'))
                env = {k:v for k,v in os.environ.items() if not k.startswith('HALO2_') or k == 'HALO2_TEST_SCRATCH'}
                env.update(SDL_VIDEODRIVER=args.video_driver, SDL_AUDIODRIVER='dummy',
                           XDG_CONFIG_HOME=str(owned/'config'), XDG_DATA_HOME=str(owned/'data'),
                           XDG_CACHE_HOME=str(owned/'cache'), TMPDIR=str(owned/'tmp'), TMP=str(owned/'tmp'), TEMP=str(owned/'tmp'),
                           HALO2_PIPELINE_CACHE_DIRECTORY=str(owned/'pipeline'), MESA_SHADER_CACHE_DIR=str(owned/'mesa'),
                           __GL_SHADER_DISK_CACHE_PATH=str(owned/'nvidia'))
                if role == 'hidden':
                    env['HALO2_REFERENCE_CACHE_DECALS_HIDE'] = '1'
                command = [str(args.runtime), '--campaign', '--map', str(args.map), '--shared-maps', str(args.map.parent),
                           '--frames', str(args.frames), '--viewport', str(args.width), str(args.height),
                           '--render-size', str(args.width), str(args.height), '--mute', '--hidden', '--validation',
                           '--graphics-mode', 'Original', '--screenshot', str(frame), '--campaign-reference-capture',
                           str(args.interval), str(args.fraction), str(args.auxiliary_seed), str(trace)]
                row = dict(role=role, command=command, errors=[]); report['runs'].append(row)
                started = time.monotonic()
                try:
                    with log.open('wb') as output:
                        row['exit_code'] = subprocess.run(command, env=env, stdout=output, stderr=subprocess.STDOUT,
                                                          timeout=args.timeout).returncode
                except subprocess.TimeoutExpired:
                    row['exit_code'] = 124; row['timed_out'] = True
                row['seconds'] = round(time.monotonic()-started, 3)
                output = read(log); row['errors'] += log_errors(output)
                if row['exit_code']:
                    row['errors'].append(f"Actual natural campaign process exited {row['exit_code']}")
                row['log_tail'] = output[-4096:].decode(errors='replace')
                events = [json.loads(line) for line in read(trace).splitlines()] if trace.exists() else []
                configurations = [e for e in events if e.get('event') == 'configuration']
                if len(configurations) != 1:
                    row['errors'].append('Actual natural campaign configuration receipt missing')
                else:
                    row['errors'] += configuration_errors(configurations[0], args)
                row['errors'] += construction_errors([e for e in events if e.get('stage') == 'before_scenario_factories'], args)
                row['errors'] += source_stage_errors(events, args)
                renderer = [e for e in events if e.get('event') == 'renderer']
                if [e.get('frame') for e in renderer] != list(range(args.frames)) or any(not e.get('submitted') for e in renderer):
                    row['errors'].append('Actual native campaign did not submit all requested frames')
                if any((e.get('width'), e.get('height')) != (args.width, args.height) for e in renderer):
                    row['errors'].append('Actual native renderer changed requested resolution')
                plan = [(e.get('source_decal_draws'), e.get('source_decal_quads'), e.get('source_decal_draw_hash')) for e in renderer]
                row['last_decal_plan'] = plan[-1] if plan else None
                if not plan or any(any(type(v) is not int for v in p) for p in plan):
                    row['errors'].append('Actual campaign cached-decal ModelView receipt missing')
                elif role != 'hidden' and not (plan[-1][0] > 0 and plan[-1][1] > 0):
                    row['errors'].append('Final actual campaign view has no cached-decal geometry; visible proof is empty')
                elif role == 'hidden' and any(p[0] or p[1] for p in plan):
                    row['errors'].append('Hidden control still submitted cached-decal geometry')
                clocks = [e['state']['source_tick'] for e in events if 'source_tick' in e.get('state', {})]
                row['first_source_tick'] = clocks[0] if clocks else None; row['last_source_tick'] = clocks[-1] if clocks else None
                if not any(0 < ((b-a)&0xffffffff) < 0x80000000 for a,b in zip(clocks, clocks[1:])):
                    row['errors'].append('Actual original source clock did not advance')
                image = ppm(frame) if frame.exists() else None
                if image is None or image[:2] != (args.width, args.height):
                    row['errors'].append('Complete declared native campaign framebuffer missing')
                row['frame_sha256'] = sha(frame) if frame.exists() else None
                images.append(image); plans.append(plan); source_traces.append(normalized(events))
            if all(image is not None for image in images):
                report['enabled_hidden_changed_pixels'] = changed(images[0], images[1])
                report['enabled_repeat_changed_pixels'] = changed(images[0], images[2])
                if not report['enabled_hidden_changed_pixels']:
                    report['failures'].append('Hiding actual campaign cached decals changes no surviving pixel')
                if report['enabled_repeat_changed_pixels']:
                    report['failures'].append('Repeated unchanged actual native campaign frame is not pixel-identical')
            report['source_traces_identical'] = source_traces[0] == source_traces[1] == source_traces[2]
            report['enabled_repeat_plans_identical'] = plans[0] == plans[2]
            if not report['source_traces_identical']:
                report['failures'].append('Cached-decal draw control changed original source/camera/actor trace')
            if not report['enabled_repeat_plans_identical']:
                report['failures'].append('Repeated unchanged campaign decal plan differs')
        report['owned_capture_directory_removed'] = not work.exists()
        report['inputs_unchanged'] = all(sha(Path(p)) == value for p,value in pins.items()) and \
            all(sha(args.source_root/p) == value for p,value in source_pins.items())
        report['passed'] = bool(report['owned_capture_directory_removed'] and report['inputs_unchanged'] and
                                not report['failures'] and all(not row['errors'] for row in report['runs']))
    except Exception as error:
        report['failure'] = f'{type(error).__name__}: {error}'
        report['owned_capture_directory_removed'] = work is None or not work.exists()
        report['passed'] = False
    encoded = json.dumps(report, indent=2)+'\n'
    if len(encoded.encode()) > 1024*1024:
        raise RuntimeError('Bounded campaign report exceeded1MiB')
    args.report.parent.mkdir(parents=True, exist_ok=True); args.report.write_text(encoded)
    print(json.dumps(dict(passed=report['passed'], report=str(args.report), failures=report['failures'],
                         run_errors={row['role']:row['errors'] for row in report['runs']})))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
