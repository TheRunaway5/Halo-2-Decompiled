#!/usr/bin/env python3
"""Assert ordinary native player-shadow submission in a real MCC match.

Run through tools/run_test.py. Frames, savegame and timings stay in its owned
scratch; only the explicit bounded JSON report is retained. Positive CPU root
selection alone does not pass: the native Generate and Apply GPU queues must
both receive draws. This is submission coverage, not Xbox pixel parity.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from run_tests import validation_environment


FIELDS = ('world_native_shadow_generate_prepared_draws', 'world_native_shadow_apply_prepared_draws')


def ppm(path):
    """Read the binary payload after the P6 header's exact final delimiter."""
    data = path.read_bytes()
    cursor = 0
    tokens = []
    for _ in range(4):
        while cursor < len(data):
            if data[cursor] in b' \t\r\n':
                cursor += 1
            elif data[cursor] == ord('#'):
                end = data.find(b'\n', cursor)
                if end < 0:
                    raise RuntimeError('Truncated PPM header comment')
                cursor = end + 1
            else:
                break
        first = cursor
        while cursor < len(data) and data[cursor] not in b' \t\r\n#':
            cursor += 1
        tokens.append(data[first:cursor])
    if tokens[0] != b'P6' or tokens[3] != b'255' or cursor >= len(data) or data[cursor] not in b' \t\r\n':
        raise RuntimeError('Invalid native screenshot PPM header')
    width, height = map(int, tokens[1:3])
    if width <= 0 or height <= 0:
        raise RuntimeError('Invalid native screenshot extent')
    cursor += 2 if data[cursor:cursor+2] == b'\r\n' else 1
    pixels = data[cursor:]
    if len(pixels) != width * height * 3:
        raise RuntimeError('Native screenshot payload extent differs from its PPM header')
    return (width, height), pixels


def frame_receipt(output, mode):
    values = [json.loads(line.split(': ', 1)[1]) for line in output.splitlines()
              if line.startswith('Player shadow frame: ')]
    if not values or values[-1].get('schema') != 1 or not values[-1].get('completed'):
        raise RuntimeError('Native screenshot lacks its completed player-shadow frame receipt')
    frame = values[-1]
    if frame.get('video_driver') != 'x11':
        raise RuntimeError('Native screenshot did not use its owned xvfb X11 video driver')
    if frame.get('tick') != 90 or frame.get('mode') != mode or len(frame.get('views', [])) != 1:
        raise RuntimeError('Player-shadow receipt belongs to another tick, graphics mode or view')
    unit = frame['views'][0].get('local_unit')
    if type(unit) is not int or not 0 <= unit < 0xffffffff or frame['views'][0].get('local_unit_alive') is not True:
        raise RuntimeError('Player-shadow screenshot has no actual living local-unit identity')
    return frame


def path_tracing_receipt(output):
    values = [json.loads(line.split(': ', 1)[1]) for line in output.splitlines()
              if line.startswith('Path Tracing frame: ')]
    if not values or not values[-1]['completed']:
        raise RuntimeError('Path Tracing player shadow frame never completed')
    return values[-1]


def local_body_triangles(ray, frame):
    """Require body provenance after final BVH filtering, independently of guns."""
    local = frame['views'][0]
    count = 0
    for view in ray['view_counts']:
        bodies = view.get('native_body_roots', [])
        if view.get('native_body_occluder_triangles', 0) != sum(body.get('triangles', 0) for body in bodies):
            raise RuntimeError('Submitted ray body total differs from its final body-owner counts')
        if view['slot'] != local['view'] + 1:
            continue
        for body in bodies:
            if body.get('owner') != local['local_unit'] or body.get('source_tick') != frame.get('world_tick', frame['tick']):
                continue
            if any(type(body.get(key)) is not int or not 0 <= body[key] < 0xffffffff
                   for key in ('definition', 'render_model')) or not 0 <= body.get('lod', 6) < 6:
                raise RuntimeError('Submitted ray body lost its actual full-model identity or LOD')
            if body.get('triangles', 0) != body.get('eligible_triangles', -1):
                raise RuntimeError('Submitted ray body contains only a subset of its full eligible native bank')
            count += body.get('triangles', 0)
    return count


def validate_output(output):
    messages = [line for line in output.splitlines() if line.startswith('Vulkan validation: ')]
    if messages:
        raise RuntimeError('Vulkan validation diagnostics: ' + '\n'.join(messages[:20])[:12000])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--mode', choices=('Original', 'pStencil', 'PathTracing'), default='Original')
    parser.add_argument('--width', type=int, default=640)
    parser.add_argument('--height', type=int, default=480)
    parser.add_argument('--verify-visible', action='store_true',
                        help='Compare with the same native scene while omitting only shadow GPU packets')
    parser.add_argument('--report-output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    runtime, level = args.runtime.resolve(), args.map.resolve()
    inputs = scratch / 'player-shadow-inputs.tsv'
    inputs.write_text(''.join(f'{tick} 0 0 0 0 0 {-min(1.5, tick * .05)}\n' for tick in range(90)))
    timings = scratch / 'renderer.tsv'
    command = ['xvfb-run', '-a', '-s', f'-screen 0 {max(800, args.width)}x{max(600, args.height)}x24',
               str(runtime), '--multiplayer', '--map', str(level), '--shared-maps', str(level.parent),
               '--local-players', '1', '--mp-starting-weapon', '5', '--mp-source-object-world', str(scratch / 'world'),
               '--mp-input-script', str(inputs), '--mp-capture-tick', '90', '--mp-reference-tick-cadence',
               '--mp-reference-render-fraction', '1', '--hidden', '--mute', '--validation', '--graphics-mode', args.mode,
               '--mp-reference-auxiliary-seed', '305419896',
               '--viewport', str(args.width), str(args.height), '--screenshot', str(scratch / 'player.ppm')]
    report = {'passed': False, 'scope': 'ordinary native player shadow GPU submission; no framebuffer parity claim',
              'command': command, 'runtime': str(runtime), 'map': str(level), 'mode': args.mode,
              'logical_size': [args.width, args.height], 'scratch': str(scratch), 'retained_large_artifacts': []}
    try:
        with runtime.open('rb') as stream:
            report['runtime_sha256'] = hashlib.file_digest(stream, 'sha256').hexdigest()
        with level.open('rb') as stream:
            report['map_sha256'] = hashlib.file_digest(stream, 'sha256').hexdigest()
        with (scratch / 'runtime.log').open('w+') as log:
            environment = {**os.environ, 'HALO2_RENDER_TIMINGS': str(timings),
                           'SDL_VIDEODRIVER': 'x11',
                           'XDG_CACHE_HOME': str(scratch / 'cache'),
                           'XDG_CONFIG_HOME': str(scratch / 'config'),
                           'XDG_DATA_HOME': str(scratch / 'data'),
                           'HALO2_PIPELINE_CACHE_DIRECTORY': str(scratch / 'vulkan-cache'),
                           'MESA_SHADER_CACHE_DIR': str(scratch / 'mesa-cache'),
                           '__GL_SHADER_DISK_CACHE_PATH': str(scratch / 'nvidia-cache')}
            environment, report['validation_discovery'] = validation_environment(Path(__file__).resolve().parents[1], environment)
            result = subprocess.run(command, env=environment,
                                    stdout=log, stderr=subprocess.STDOUT, timeout=180)
            report['exit_code'] = result.returncode
            log.seek(0)
            output = log.read()
        report['diagnostic_tail'] = output[-12000:]
        roots = [int(value) for value in re.findall(r'Source shadow selection:.*? roots=(\d+)', output)]
        report['maximum_selected_roots'] = max(roots, default=0)
        with timings.open() as stream:
            rows = list(csv.DictReader(stream, delimiter='\t'))
        report['frames'] = len(rows)
        report['draws'] = {name: sum(int(row[name]) for row in rows) for name in FIELDS}
        if result.returncode:
            raise RuntimeError(f'Native match exited {result.returncode}')
        validate_output(output)
        report['validation_checked'] = True
        frame = report['captured_frame'] = frame_receipt(output, args.mode)
        if args.mode == 'PathTracing':
            report['path_tracing'] = path_tracing_receipt(output)
            report['local_body_triangles'] = local_body_triangles(report['path_tracing'], frame)
            if report['local_body_triangles'] <= 0:
                raise RuntimeError('Living local full native BODY never reached the final submitted ray geometry')
        else:
            view = frame['views'][0]
            if view['local_unit'] not in view['selected_player_roots'] or not any(
                    root['owner'] == view['local_unit'] and root['caster_draws'] and root['receiver_draws']
                    for root in view['submitted_player_roots']):
                raise RuntimeError('Captured frame has no actual local-player root in both native shadow phases')
            if any(not frame['draws'][name] for name in FIELDS):
                raise RuntimeError('Captured player root never reached both native Generate and Apply GPU queues')
        image = scratch / 'player.ppm'
        with image.open('rb') as stream:
            report['capture'] = {'bytes': image.stat().st_size,
                                 'sha256': hashlib.file_digest(stream, 'sha256').hexdigest()}
        if args.verify_visible:
            off_command = command.copy()
            off_command[off_command.index('--mp-source-object-world') + 1] = str(scratch / 'world-off')
            off_image = scratch / 'player-off.ppm'
            off_command[off_command.index('--screenshot') + 1] = str(off_image)
            off_environment = {**environment, 'HALO2_REFERENCE_DISABLE_PLAYER_SHADOW_GPU': '1',
                               'HALO2_RENDER_TIMINGS': str(scratch / 'renderer-off.tsv')}
            with (scratch / 'runtime-off.log').open('w+') as log:
                off = subprocess.run(off_command, env=off_environment, stdout=log,
                                     stderr=subprocess.STDOUT, timeout=180)
                log.seek(0)
                off_output = log.read()
                report['off_diagnostic_tail'] = off_output[-12000:]
            if off.returncode:
                raise RuntimeError(f'Native shadow GPU-off control exited {off.returncode}')
            validate_output(off_output)
            off_frame = report['gpu_off_frame'] = frame_receipt(off_output, args.mode)
            if off_frame['views'][0]['local_unit'] != frame['views'][0]['local_unit'] or \
                    off_frame['views'][0]['selected_player_roots'] != frame['views'][0]['selected_player_roots']:
                raise RuntimeError('GPU-off control changed the actual player identity or native root selection')
            if any(off_frame['draws'][name] for name in FIELDS) or any(
                    view['submitted_player_roots'] for view in off_frame['views']):
                raise RuntimeError('GPU-off control still submitted native player shadow packets')
            if args.mode == 'PathTracing':
                report['gpu_off_path_tracing'] = path_tracing_receipt(off_output)
                if any(view.get('native_player_occluder_triangles', 0)
                       for view in report['gpu_off_path_tracing']['view_counts']):
                    raise RuntimeError('GPU-off control still included additional native player ray occluders')
                if local_body_triangles(report['gpu_off_path_tracing'], off_frame) or any(
                        view.get('native_body_occluder_triangles', 0)
                        for view in report['gpu_off_path_tracing']['view_counts']):
                    raise RuntimeError('GPU-off control still included authenticated native body triangles')
            on_header, on_pixels = ppm(image)
            off_header, off_pixels = ppm(off_image)
            if on_header != off_header or len(on_pixels) != len(off_pixels):
                raise RuntimeError('Native shadow on/off capture extents differ')
            errors = bytes(abs(a-b) for a, b in zip(on_pixels, off_pixels))
            changed = sum(any(value > 1 for value in errors[first:first+3])
                          for first in range(0, len(errors), 3))
            report['visible_comparison'] = {'changed_pixels_above_one': changed,
                                            'maximum_channel_difference': max(errors, default=0),
                                            'gpu_off_sha256': hashlib.sha256(off_image.read_bytes()).hexdigest(),
                                            'scope': 'same seeded native scene and base depth; shadow GPU packets omitted'}
            if not changed:
                raise RuntimeError('Submitted player shadows produced no visible pixel change')
        report['passed'] = True
    except Exception as error:
        report['error'] = str(error)
    finally:
        args.report_output.parent.mkdir(parents=True, exist_ok=True)
        args.report_output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({name: report.get(name) for name in ('passed', 'maximum_selected_roots', 'draws', 'error')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
