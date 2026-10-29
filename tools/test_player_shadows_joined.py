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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--mode', choices=('Original', 'pStencil', 'PathTracing'), required=True)
    parser.add_argument('--report-output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    child_report = scratch / 'joined-receipt.json'
    command = ['xvfb-run', '-a', '-s', '-screen 0 800x600x24', str(args.fixture),
               str(args.map), str(args.map.parent), args.mode, str(child_report)]
    report = {'passed': False, 'command': command, 'mode': args.mode,
              'scope': 'real authenticated UDP joining local-player shadow GPU submission',
              'retained_large_artifacts': []}
    try:
        environment = {**os.environ, 'SDL_VIDEODRIVER': 'x11', 'SDL_AUDIODRIVER': 'dummy',
                       'HALO2_SHADOW_TRANSPORT_TRACE': '1',
                       'XDG_CACHE_HOME': str(scratch / 'cache'),
                       'XDG_CONFIG_HOME': str(scratch / 'config'),
                       'XDG_DATA_HOME': str(scratch / 'data'),
                       'HALO2_PIPELINE_CACHE_DIRECTORY': str(scratch / 'vulkan-cache'),
                       'MESA_SHADER_CACHE_DIR': str(scratch / 'mesa-cache'),
                       '__GL_SHADER_DISK_CACHE_PATH': str(scratch / 'nvidia-cache')}
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
        if not frame.get('completed') or frame.get('tick') != runtime['tick'] or \
                frame.get('mode') != args.mode or frame.get('video_driver') != 'x11' or len(frame.get('views', [])) != 1:
            raise RuntimeError('Joined frame differs from its actual tick, graphics mode, driver or view')
        if frame.get('world_tick') != frame['tick'] or frame.get('native_world_tick') != frame['world_tick'] or \
                frame.get('native_cut_serial', 0) <= 0 or frame.get('world_sequence', 0) > frame.get('gameplay_sequence', 0):
            raise RuntimeError('Joined settled capture mixed native world/body clocks or lacked an acknowledged cut')
        local = frame['views'][0]
        if local.get('local_unit') != runtime['local_unit'] or local.get('local_unit_alive') is not True:
            raise RuntimeError('Joined shadow frame lost its actual living recipient unit')
        size, pixels = ppm(scratch / 'joined-player-shadow.ppm')
        non_black = sum(max(pixels[first:first+3]) > 8 for first in range(0, len(pixels), 3))
        if size != (640, 480) or non_black < 4096 or len(set(pixels)) < 16:
            raise RuntimeError('Joined shadow capture lacks its actual visible world')
        report['capture'] = {'size': size, 'non_black_pixels': non_black,
                             'sha256': hashlib.sha256(pixels).hexdigest()}
        if args.mode == 'PathTracing':
            rays = receipts(output, 'Path Tracing frame: ')
            if not rays or not rays[-1].get('completed'):
                raise RuntimeError('Joined Path Tracing frame never completed')
            report['path_tracing'] = rays[-1]
            report['local_body_triangles'] = local_body_triangles(rays[-1], frame)
            if report['local_body_triangles'] <= 0:
                raise RuntimeError('Joined local full body never reached the final ray geometry')
        elif local['local_unit'] not in local['selected_player_roots'] or not any(
                root['owner'] == local['local_unit'] and root['caster_draws'] and root['receiver_draws']
                for root in local['submitted_player_roots']):
            raise RuntimeError('Joined local player never reached both native GPU shadow phases')
        with args.fixture.open('rb') as source:
            report['fixture_sha256'] = hashlib.file_digest(source, 'sha256').hexdigest()
        report['passed'] = True
    except Exception as error:
        report['error'] = str(error)
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report.get(key) for key in ('passed', 'mode', 'local_body_triangles', 'error')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
