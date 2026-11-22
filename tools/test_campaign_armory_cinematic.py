#!/usr/bin/env python3
"""Replay the ordinary Armory route into the authored Cairo Johnson cutscene."""

import argparse
import csv
import json
import os
from pathlib import Path
import re
import struct
import subprocess

from PIL import Image

from physics_test_cache import runtime_cache, runtime_map
from run_tests import validation_environment
from test_campaign_new_game_runtime import ARMORY, parse_campaign_lines, sha
from validate_weapon_runtime_tu15 import private_display


ROOT = Path(__file__).resolve().parents[1]
CAIRO = 'scenarios\\solo\\01b_spacestation\\01b_spacestation'
CONTROLS = ROOT / 'tests/fixtures/campaign_armory_tram_inputs.json'
CONTROLS_SHA256 = 'b764a7bb0ba0a8170f5326d1124535ca9a7aba6a8d43db84dc6abb805524c193'
CAIRO_SHA256 = 'e2140fd3d5e27e46d693f82824e1d1e881f7bec30e1b76f7a17fceb2b130c8c2'
JOHNSON_CUES = {0x840B21D0: 'x02_0020_jon', 0x840C21D1: 'x02_0030_jon',
               0x840D21D2: 'x02_0040_jon', 0x840E21D3: 'x02_0050_jon'}
JOHNSON_SCENE = ('x02_scene_01', 'x02_01_setup', 'x02_01_cleanup')
SCENE_MILESTONES = ('johnson - annoyed .75 15', 'johnson - happy .75 30',
                    'johnson - pensive 1 60', 'open bridge door')
VIEWPORT = (896, 504)


def position(line):
    match = re.search(r'\bposition=(-?\d+\.\d+),(-?\d+\.\d+),(-?\d+\.\d+)', line)
    return [float(value) for value in match.groups()] if match else None


def render_camera(bits):
    return struct.unpack('<9f', struct.pack('<9I', *bits))


def observe_cairo_images(lines, capture_dir):
    snapshots = []
    for line in lines:
        capture = re.fullmatch(r'Captured (.*\.ppm) \(\d+x\d+\)', line)
        if not capture:
            continue
        path = Path(capture[1]).resolve(strict=True)
        if path.parent != capture_dir:
            raise ValueError('Native Cairo image escaped the capture directory')
        tick = re.fullmatch(r'tick-(\d+)-frame-\d+\.ppm', path.name)
        if not tick:
            raise ValueError('Native Cairo capture has no source tick')
        with Image.open(path) as image:
            if image.size != VIEWPORT:
                raise ValueError('Native Cairo capture changed dimensions')
            rgb = image.convert('RGB')
            channels = rgb.getextrema()
            midtones = sum(rgb.convert('L').histogram()[8:248]) / (image.width * image.height)
        snapshots.append({'tick': int(tick[1]), 'file': path.name, 'sha256': sha(path), 'rgb_ranges': channels,
                          'midtone_fraction': midtones,
                          'nonuniform': midtones >= .05 and max(high - low for low, high in channels) >= 16})
    return snapshots


def observe_run(lines, trace, gpu_log, capture_dir, requested_ticks, mute, direct_cairo=False):
    source = parse_campaign_lines(lines)
    starts = source['initial_scenarios']
    transitions = source['natural_next_scenarios']
    armory = next((row for row in source['source_status'] if row['scenario'] == ARMORY
                  and row['phase'] == 'status' and row['level_finished']), None)
    cairo = next((row for row in starts if row['scenario'] == CAIRO), None)
    final = next((row for row in reversed(source['source_status']) if row['scenario'] == CAIRO
                  and row['phase'] == 'status'), None)
    armory_position = position(lines[armory['line'] - 1]) if armory else None
    cairo_position = position(lines[cairo['line'] - 1]) if cairo else None
    cairo_lines = lines[cairo['line']:] if cairo else []
    calls = []
    milestones = []
    voices = []
    for line in cairo_lines:
        if match := re.fullmatch(r'Script call: tick=(\d+) name=(\S+)', line):
            if match[2] in JOHNSON_SCENE or match[2] in JOHNSON_CUES.values():
                calls.append({'tick': int(match[1]), 'name': match[2]})
        elif line.startswith('Script: '):
            message = line.removeprefix('Script: ')
            if message in SCENE_MILESTONES:
                milestones.append(message)
        elif match := re.match(r'Script audio packet: tick=(\d+) sound=(\d+) owner=(\d+).* packet_frames=(\d+)', line):
            tick, sound, owner, length = map(int, match.groups())
            if sound in JOHNSON_CUES and length > 0:
                voices.append({'tick': tick, 'sound': JOHNSON_CUES[sound], 'owner': owner, 'packet_frames': length})

    cameras = []
    if trace.is_file():
        with trace.open() as stream:
            for line in stream:
                row = json.loads(line)
                if (row['scenario'] == CAIRO and 50 <= row['tick'] < 550
                        and 'source_camera_bits' in row and row.get('fade', 1) < .5):
                    cameras.append({'tick': row['tick'], 'frame': row['frame'], 'command': row['camera_command'],
                                    'source': render_camera(row['source_camera_bits']),
                                    'rendered': render_camera(row['render_camera_bits'])})
    # Each BSP creates a new renderer which truncates the shared timing file.
    # Completed readbacks witness the cinematic's own submitted frames.
    completed_gpu = []
    if gpu_log.is_file():
        with gpu_log.open() as stream:
            completed_gpu = [int(row['frame']) for row in csv.DictReader(stream, delimiter='\t')
                             if (int(row['render_width']), int(row['render_height'])) == VIEWPORT
                             and int(row['views']) > 0 and float(row['gpu_scene_ms']) > 0]
    completion = next((line for line in reversed(cairo_lines)
                       if line.startswith('Campaign live input complete:')), '')
    info = re.fullmatch(r'Campaign live input complete: steps=(\d+) tick=(\d+) frames=(\d+) level_finished=[01]',
                        completion)
    rendered = next((int(line.split()[1]) for line in reversed(cairo_lines)
                     if re.fullmatch(r'Rendered \d+ frames', line)), None)
    images = observe_cairo_images(cairo_lines, capture_dir)
    scheduled_scene = [row['name'] for row in calls if row['name'] in JOHNSON_SCENE]
    moving_cameras = len({row['source'] for row in cameras})
    camera_ticks = {row['tick'] for row in cameras}
    readback_ticks = {image['tick'] for image in images if image['nonuniform'] and image['tick'] in camera_ticks}
    checks = {
        'fresh_cairo_player_position': bool(cairo and cairo['tick'] == 0 and cairo_position
                                           and all(abs(observed - expected) <= .08
                                           for observed, expected in zip(cairo_position, (60.491, -9.179, 7.063), strict=True))),
        'first_johnson_scene_executed': scheduled_scene[:len(JOHNSON_SCENE)] == list(JOHNSON_SCENE)
                                        and milestones[:len(SCENE_MILESTONES)] == list(SCENE_MILESTONES),
        'authored_camera_moved': bool(cameras and cameras[0]['tick'] < 150 and cameras[-1]['tick'] >= 490
                                     and moving_cameras >= 20),
        'cairo_frames_rendered': len(readback_ticks) >= 2 and bool(completed_gpu),
        'cairo_render_loop_finished': bool(info and rendered is not None and rendered > 20
                                           and rendered == int(info[3])),
        'advanced_past_first_scene': bool(info and final and int(info[1]) == requested_ticks
                                          and int(info[2]) == final['tick'] >= 565),
        'no_script_or_content_faults': (not source['script_faults'] and not any(source['content_error_counts'])
                                      and not any(row['script_faults'] for row in source['source_status'])),
    }
    if direct_cairo:
        checks['direct_cairo_started'] = [row['scenario'] for row in starts] == [CAIRO] and not transitions
    else:
        checks['natural_armory_to_cairo'] = bool(
            [row['scenario'] for row in starts] == [ARMORY, CAIRO]
            and len(transitions) == 1 and armory and cairo
            and armory['line'] < transitions[0]['line'] < cairo['line']
            and transitions[0]['scenario'] == CAIRO
            and all(row['difficulty'] == starts[0]['difficulty'] for row in starts + transitions))
        checks['tram_station_position'] = bool(armory_position and 60.5 <= armory_position[0] <= 64
                                               and -18 <= armory_position[1] <= -14
                                               and 6.9 <= armory_position[2] <= 7.1)
    if not mute:
        checks['authored_johnson_pcm_submitted'] = len({cue['sound'] for cue in voices}) >= 2
    return dict(**source, armory_exit_position=armory_position, cairo_entry_position=cairo_position,
                completion=completion, johnson_script_calls=calls, johnson_cues=voices,
                johnson_scene_milestones=milestones, scripted_camera_rows=len(cameras),
                distinct_source_camera_poses=moving_cameras, camera_endpoints=cameras[:1] + cameras[-1:],
                cairo_rendered_frames=rendered, completed_gpu_rows=len(completed_gpu),
                gpu_timing_scope='last renderer generation', cairo_images=images,
                cinematic_readback_ticks=sorted(readback_ticks), checks=checks)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--mcc-root', type=Path, required=True)
    parser.add_argument('--report-output', type=Path, required=True)
    parser.add_argument('--ticks', type=int, help='Defaults to 12000 for the full route or 650 for a direct Cairo smoke')
    parser.add_argument('--direct-cairo', action='store_true', help='Start Cairo at its authored spawn with neutral controls')
    parser.add_argument('--timeout', type=int, default=540)
    parser.add_argument('--graphics-mode', choices=('Original', 'pStencil'), default='Original')
    parser.add_argument('--validation', action='store_true')
    parser.add_argument('--mute', action='store_true', help='Inspect visuals without qualifying cinematic audio')
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Run through tools/run_test.py')
    minimum = 565 if args.direct_cairo else 11500
    ticks = args.ticks if args.ticks is not None else (650 if args.direct_cairo else 12000)
    if not minimum <= ticks <= 18000 or not 1 <= args.timeout <= 1200:
        parser.error(f'Require {minimum}..18000 input ticks and a timeout within 1..1200 seconds')
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    runtime = args.runtime.resolve(strict=True)
    maps, _ = runtime_cache(args.mcc_root)
    armory = runtime_map(maps, maps / '01a_tutorial.map')
    cairo = runtime_map(maps, maps / '01b_spacestation.map')
    if sha(CONTROLS) != CONTROLS_SHA256 or sha(cairo) != CAIRO_SHA256:
        parser.error('The approved replay controls or Cairo audio-tag fixture changed')
    controls = CONTROLS
    if args.direct_cairo:
        controls = owner / 'neutral-controls.json'
        controls.write_text('[]\n')

    log, trace, gpu = (owner / filename for filename in ('runtime.log', 'camera.jsonl', 'gpu.tsv'))
    captures = owner / 'captures'
    environment = {key: value for key, value in os.environ.items() if not key.startswith('HALO2_')
                   and key not in ('DISPLAY', 'WAYLAND_DISPLAY', 'XAUTHORITY', 'VK_INSTANCE_LAYERS',
                                   'XDG_ACTIVATION_TOKEN')}
    control_environment = {
        'HALO2_TEST_SCRATCH': str(owner), 'HALO2_PIPELINE_CACHE_DIRECTORY': str(owner / 'pipelines'),
        'HALO2_PSTENCIL_CAMPAIGN_CAPTURE_TICK_RANGE': '450:455',
        'HALO2_PSTENCIL_CAMPAIGN_CAPTURES': str(captures), 'HALO2_PSTENCIL_CAMPAIGN_TRACE': str(trace),
        'HALO2_PSTENCIL_CAPTURE_PRESENT_MODE': 'immediate', 'HALO2_RENDER_GPU_TIMINGS': str(gpu),
        'HALO2_SCRIPT_CALL_TRACE': '1',
    }
    environment.update(control_environment, SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy')
    for key in ('XDG_CONFIG_HOME', 'XDG_DATA_HOME', 'XDG_STATE_HOME', 'XDG_CACHE_HOME'):
        environment[key] = str(owner / key.lower())
    size = [str(n) for n in VIEWPORT]
    start_map = cairo if args.direct_cairo else armory
    command = [str(runtime), '--campaign', '--map', str(start_map), '--shared-maps', str(maps), '--hidden',
               '--graphics-mode', args.graphics_mode, '--viewport', *size, '--render-size', *size,
               '--campaign-live-input-script', str(controls), str(ticks)]
    if not args.direct_cairo:
        command.append('--campaign-live-input-continue')
    if args.mute:
        command.append('--mute')
    if args.validation:
        environment, _ = validation_environment(ROOT, environment)
        environment['VK_LAYER_VALIDATE_SYNC'] = '1'
        command.append('--validation')

    runtime_digest = sha(runtime)
    with private_display(owner) as display, log.open('wb') as output:
        environment['DISPLAY'] = display
        try:
            status = subprocess.run(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT,
                                    env=environment, timeout=args.timeout).returncode
        except subprocess.TimeoutExpired:
            status = 124
    lines = log.read_text(errors='replace').split('\n')
    errors = [line[:1200] for line in lines if re.search(r'^error:|VUID-|SYNC-HAZARD|Vulkan validation:.*(?:Error|ERROR)', line)]
    report = dict(runtime=str(runtime), runtime_sha256=runtime_digest, runtime_unchanged=sha(runtime) == runtime_digest,
                  controls_sha256=sha(controls), armory_sha256=sha(armory), cairo_sha256=CAIRO_SHA256,
                  command=command, environment=control_environment, graphics_mode=args.graphics_mode,
                  direct_cairo=args.direct_cairo,
                  audio_requested=not args.mute, exit_code=status, scratch=str(owner), errors=errors[:16],
                  log_tail='\n'.join(lines[-35:])[-10000:], retained_large_artifacts=[])
    try:
        report.update(observe_run(lines, trace, gpu, captures, ticks, args.mute, args.direct_cairo))
    except (OSError, ValueError, KeyError) as failure:
        report['observation_error'] = str(failure)
    report['passed'] = bool(status == 0 and not errors and report['runtime_unchanged']
                            and report.get('checks') and all(report['checks'].values()))
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report.get(key) for key in ('passed', 'checks', 'errors', 'observation_error')}))
    return int(not report['passed'])


if __name__ == '__main__':
    raise SystemExit(main())
