#!/usr/bin/env python3
"""Require a real joining player's body in the submitted shadow GPU frame."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from run_tests import validation_environment
from test_player_shadows_native import local_body_triangles, ppm, validate_output


def receipts(output, prefix):
    return [json.loads(line[len(prefix):]) for line in output.splitlines() if line.startswith(prefix)]


def native_transport_diagnostics(output):
    """Retain bounded producer evidence before verbose GPU logs hide it."""
    errors, progress, timing = [], [], []
    for line in output.splitlines():
        if line.startswith(('[shadow-transport] native capture rejected',
                            '[shadow-transport] native encode rejected',
                            '[shadow-transport] native cut abandoned')):
            errors.append(line[:1024])
        elif line.startswith('[shadow-transport] native host progress'):
            progress.append(line[:1024])
        elif line.startswith('Shadow GPU timing: '):
            timing.append(line[:1024])
    return {'rejection_count': len(errors), 'rejections_first16': errors[:16],
            'host_progress_count': len(progress), 'host_progress_first16': progress[:16],
            'host_progress_last16': progress[-16:], 'frame_timing_count': len(timing),
            'frame_timing_first16': timing[:16]}


def geometry_usage_receipts(output):
    """Retain the first bounded native witnesses, independently of log tails."""
    prefix = '[native-fp-continuation] '
    result = []
    for line in output.splitlines():
        if not line.startswith(prefix):
            continue
        if len(result) == 16:
            break
        row = json.loads(line[len(prefix):])
        allocator, rows, address, index = (row[key] for key in
            ('geometry_allocator', 'geometry_rows', 'first_geometry_usage', 'geometry_usage_index'))
        result.append({**row, 'native_inline_rows_offset': 0xbc,
                       'native_row_stride': 0x18, 'native_usage_offset': 0x14,
                       'matches_native_layout': rows == allocator + 0xbc and
                           0 <= index < 0x200 and address == rows + index * 0x18 + 0x14})
    return result


def bitmap_prediction_receipts(output):
    """Keep actual cold-request identities even when later logs are verbose."""
    prefix = '[native-fp-bitmap-prediction] '
    result = []
    for line in output.splitlines():
        if line.startswith(prefix):
            result.append(json.loads(line[len(prefix):]))
            if len(result) == 16:
                break
    return result


def run_case(args, scratch, report, gpu_off=False):
    child_report = scratch / 'joined-receipt.json'
    command = ['xvfb-run', '-a', '-s', '-screen 0 800x600x24', str(args.fixture),
               str(args.map), str(args.map.parent), args.mode, str(child_report)]
    if args.host_local_players != 1:
        command += ['--host-local-players', str(args.host_local_players)]
    if gpu_off:
        command.append('--gpu-off')
    report.update({'passed': False, 'command': command, 'mode': args.mode,
              'scope': 'real authenticated UDP joining local-player shadow GPU submission',
              'retained_large_artifacts': [], 'gpu_disabled_control': gpu_off})
    try:
        environment = {**os.environ, 'SDL_VIDEODRIVER': 'x11', 'SDL_AUDIODRIVER': 'dummy',
                       'HALO2_SHADOW_TRANSPORT_TRACE': '1',
                       'XDG_CACHE_HOME': str(scratch / 'cache'),
                       'XDG_CONFIG_HOME': str(scratch / 'config'),
                       'XDG_DATA_HOME': str(scratch / 'data'),
                       'HALO2_PIPELINE_CACHE_DIRECTORY': str(scratch / 'vulkan-cache'),
                       'MESA_SHADER_CACHE_DIR': str(scratch / 'mesa-cache'),
                       '__GL_SHADER_DISK_CACHE_PATH': str(scratch / 'nvidia-cache')}
        environment.update({name: str(scratch) for name in ('HALO2_TEST_SCRATCH', 'TMPDIR', 'TMP', 'TEMP')})
        environment.pop('HALO2_REFERENCE_DISABLE_PLAYER_SHADOW_GPU', None)
        if gpu_off:
            environment['HALO2_REFERENCE_DISABLE_PLAYER_SHADOW_GPU'] = '1'
        environment, report['validation_discovery'] = validation_environment(Path(__file__).resolve().parents[1], environment)
        with (scratch / 'joined.log').open('w+') as log:
            result = subprocess.run(command, env=environment, stdout=log,
                                    stderr=subprocess.STDOUT, timeout=240)
            log.seek(0)
            output = log.read()
        report['exit_code'] = result.returncode
        report['diagnostic_tail'] = re.sub(r'#[0-9a-fA-F]{64}', '#<REDACTED>', output[-16000:])
        report['native_adoptions'] = receipts(output, '[shadow-presentation] ')
        report['producer_progress'] = [line for line in output.splitlines()
                                       if line.startswith('Joined shadow producer progress: ')][-16:]
        report['transport_progress'] = [line for line in output.splitlines()
                                        if line.startswith('Native shadow client transport: ')][-16:]
        if child_report.is_file():
            report['runtime'] = json.loads(child_report.read_text())
        if result.returncode:
            raise RuntimeError(f'Joined shadow fixture exited {result.returncode}')
        validate_output(output)
        report['validation_checked'] = True
        frames = receipts(output, 'Player shadow frame: ')
        if not frames:
            raise RuntimeError('Joined capture has no actual completed player-shadow frame receipt')
        frame = report['captured_frame'] = frames[-1]
        runtime = report['runtime']
        if runtime.get('producer_local_players') != args.host_local_players or runtime.get('recipient_local_players') != 1:
            raise RuntimeError('Joined capture used another actual producer/recipient local-view role')
        if not frame.get('completed') or frame.get('tick') != runtime['tick'] or \
                frame.get('mode') != args.mode or frame.get('video_driver') != 'x11' or len(frame.get('views', [])) != 1:
            raise RuntimeError('Joined frame differs from its actual tick, graphics mode, driver or view')
        if frame.get('world_tick') != frame['tick'] or frame.get('native_world_tick') != frame['world_tick'] or \
                frame.get('native_cut_serial', 0) <= 0 or frame.get('world_sequence', 0) > frame.get('gameplay_sequence', 0):
            raise RuntimeError('Joined settled capture mixed native world/body clocks or lacked an acknowledged cut')
        if frame.get('world_blend') != 1:
            raise RuntimeError('Joined pixel control captured before its native interpolation settled')
        local = frame['views'][0]
        if local.get('local_unit') != runtime['local_unit'] or local.get('local_unit_alive') is not True:
            raise RuntimeError('Joined shadow frame lost its actual living recipient unit')
        size, pixels = ppm(scratch / 'joined-player-shadow.ppm')
        non_black = sum(max(pixels[first:first+3]) > 8 for first in range(0, len(pixels), 3))
        if size != (640, 480) or non_black < 4096 or len(set(pixels)) < 16:
            raise RuntimeError('Joined shadow capture lacks its actual visible world')
        report['capture'] = {'size': size, 'non_black_pixels': non_black,
                             'sha256': hashlib.sha256(pixels).hexdigest()}
        if gpu_off and (runtime.get('generate_draws') or runtime.get('apply_draws') or local['submitted_player_roots']):
            raise RuntimeError('GPU-off control submitted fixed shadow draws')
        if args.mode == 'PathTracing':
            rays = receipts(output, 'Path Tracing frame: ')
            if not rays or not rays[-1].get('completed'):
                raise RuntimeError('Joined Path Tracing frame never completed')
            report['path_tracing'] = rays[-1]
            report['local_body_triangles'] = local_body_triangles(rays[-1], frame)
            if gpu_off:
                if report['local_body_triangles'] != 0:
                    raise RuntimeError('GPU-off ray control retained its designated full body')
            elif report['local_body_triangles'] <= 0:
                raise RuntimeError('Joined local full body never reached the final ray geometry')
        elif not gpu_off and (local['local_unit'] not in local['selected_player_roots'] or not any(
                root['owner'] == local['local_unit'] and root['caster_draws'] and root['receiver_draws']
                for root in local['submitted_player_roots'])):
            raise RuntimeError('Joined local player never reached both native GPU shadow phases')
        with args.fixture.open('rb') as source:
            report['fixture_sha256'] = hashlib.file_digest(source, 'sha256').hexdigest()
        report['passed'] = True
        return pixels
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        log = scratch / 'joined.log'
        if log.is_file():
            try:
                text = log.read_text()
                report['native_transport_diagnostics'] = native_transport_diagnostics(text)
                report['geometry_usage_receipts'] = geometry_usage_receipts(text)
                report['bitmap_prediction_receipts'] = bitmap_prediction_receipts(text)
            except Exception as error:
                report['passed'] = False
                report.setdefault('error', str(error))
                report['geometry_usage_diagnostic_error'] = str(error)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--mode', choices=('Original', 'pStencil', 'PathTracing'), required=True)
    parser.add_argument('--host-local-players', type=int, choices=(1, 2), default=1)
    parser.add_argument('--report-output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    report = {'passed': False, 'mode': args.mode, 'producer_local_players': args.host_local_players,
              'recipient_local_players': 1, 'retained_large_artifacts': []}
    try:
        positive = scratch / 'on'
        positive.mkdir()
        pixels = run_case(args, positive, report)
        report['passed'] = False
        off = report['gpu_off_control'] = {}
        negative = scratch / 'off'
        negative.mkdir()
        omitted = run_case(args, negative, off, gpu_off=True)
        on_frame, off_frame = report['captured_frame'], off['captured_frame']
        a, b = on_frame['views'][0], off_frame['views'][0]
        if any(on_frame[key] != off_frame[key] for key in ('tick', 'world_tick', 'native_world_tick', 'mode')) or \
                any(a[key] != b[key] for key in ('local_unit', 'local_unit_alive', 'selected_player_roots')) or \
                report['runtime']['local_player'] != off['runtime']['local_player']:
            raise RuntimeError('Settled GPU on/off controls changed their native tick, player or root ownership')
        if a['local_unit'] not in b['selected_player_roots']:
            raise RuntimeError('GPU-off control lost actual native local-root selection')
        on_inputs, off_inputs = report['runtime']['submitted_inputs'], off['runtime']['submitted_inputs']
        streams = ['camera', 'primary_geometry', 'node_palettes']
        if args.mode != 'PathTracing':
            streams += ['model_depth', 'material_uploads']
        mismatched = [stream for stream in streams if on_inputs[stream] != off_inputs[stream]]
        report['submitted_input_control'] = {
            'passed': not mismatched, 'compared_streams': streams, 'mismatched_streams': mismatched,
            'scope': 'Equality of observed final camera/body/FP palettes and geometry' +
                     (' plus native model-depth and completed material uploads; excludes GPU handles/frame counters; no whole-frame parity claim'
                      if args.mode != 'PathTracing' else
                      '; independent ray histories/material sampling remain outside causal pixel qualification')}
        if mismatched:
            raise RuntimeError('GPU on/off controls changed actual submitted inputs: ' + ', '.join(mismatched))
        maximum = max(abs(a - b) for a, b in zip(pixels, omitted))
        changed = sum(any(abs(pixels[n + lane] - omitted[n + lane]) > 1 for lane in range(3))
                      for n in range(0, len(pixels), 3))
        report['visible_control'] = {'changed_pixels_above_one': changed, 'maximum_channel_difference': maximum,
            'scope': 'settled same-native-tick fixed shadow on/off' if args.mode != 'PathTracing' else
                'separate ray histories; full-body membership control, no causal pixel-parity claim'}
        if args.mode != 'PathTracing' and not changed:
            raise RuntimeError('Native fixed shadow GPU on/off controls produced no visible pixel contribution')
        if report.get('geometry_usage_diagnostic_error') or off.get('geometry_usage_diagnostic_error'):
            raise RuntimeError('Native geometry ownership diagnostic could not be decoded')
        report['passed'] = True
    except Exception as error:
        report['passed'] = False
        report['error'] = str(error)
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report.get(key) for key in ('passed', 'mode', 'local_body_triangles', 'error')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
