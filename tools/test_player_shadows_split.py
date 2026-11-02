#!/usr/bin/env python3
"""Check the native fixed-shadow split policy in actual offline MCC frames.

Invoke through tools/run_test.py. This is a startup/view-policy smoke, not a
framebuffer comparison or permission to enable native split-screen shadows.
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
from test_player_shadows_native import FIELDS, ppm, validate_output

CAPTURE_TICK = 90

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--players', type=int, choices=(2, 3, 4), required=True)
    parser.add_argument('--report-output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    runtime, level = args.runtime.resolve(), args.map.resolve()
    image, timings = scratch / 'split.ppm', scratch / 'renderer.tsv'
    inputs = scratch / 'inputs.txt'
    inputs.write_text(''.join(f'{tick} {view} 0 0 0 0 0\n' for tick in range(CAPTURE_TICK) for view in range(args.players)))
    command = ['xvfb-run', '-a', '-s', '-screen 0 800x600x24', str(runtime),
               '--multiplayer', '--map', str(level), '--shared-maps', str(level.parent),
               '--local-players', str(args.players), '--mp-starting-weapon', '5',
               '--mp-source-object-world', str(scratch / 'world'), '--mp-input-script', str(inputs), '--mp-capture-tick', str(CAPTURE_TICK),
               '--mp-reference-tick-cadence', '--mp-reference-render-fraction', '1',
               '--mp-reference-auxiliary-seed', '305419896', '--hidden', '--mute', '--validation',
               '--graphics-mode', 'Original', '--viewport', '640', '480', '--screenshot', str(image)]
    report = {'passed': False, 'scope': 'Actual offline MCC split startup and native disabled fixed-shadow policy; no all-mode or framebuffer-parity claim',
              'players': args.players, 'mode': 'Original', 'tick': CAPTURE_TICK, 'command': command,
              'scratch': str(scratch), 'retained_large_artifacts': []}
    try:
        for name, path in [('runtime', runtime), ('map', level)]:
            with path.open('rb') as stream:
                report[name + '_sha256'] = hashlib.file_digest(stream, 'sha256').hexdigest()
        environment = os.environ.copy()
        environment.pop('HALO2_REFERENCE_DISABLE_PLAYER_SHADOW_GPU', None)
        environment.update(SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy', HALO2_RENDER_TIMINGS=str(timings),
                           XDG_CACHE_HOME=str(scratch / 'cache'), XDG_CONFIG_HOME=str(scratch / 'config'),
                           XDG_DATA_HOME=str(scratch / 'data'), HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch / 'vulkan-cache'),
                           MESA_SHADER_CACHE_DIR=str(scratch / 'mesa-cache'), __GL_SHADER_DISK_CACHE_PATH=str(scratch / 'nvidia-cache'))
        environment.update({key: str(scratch) for key in ('HALO2_TEST_SCRATCH', 'TMPDIR', 'TMP', 'TEMP')})
        environment, report['validation_discovery'] = validation_environment(Path(__file__).resolve().parents[1], environment)
        with (scratch / 'runtime.log').open('w+') as log:
            result = subprocess.run(command, env=environment, stdout=log, stderr=subprocess.STDOUT, timeout=180)
            log.seek(0)
            output = log.read()
        report['exit_code'], report['diagnostic_tail'] = result.returncode, output[-16000:]
        if result.returncode:
            raise RuntimeError(f'Actual split native runtime exited {result.returncode}')
        validate_output(output)
        frames = [json.loads(line[len('Player shadow frame: '):]) for line in output.splitlines()
                  if line.startswith('Player shadow frame: ')]
        if not frames:
            raise RuntimeError('Split smoke has no completed native frame receipt')
        frame = report['captured_frame'] = frames[-1]
        if frame.get('schema') != 1 or frame.get('completed') is not True or frame.get('tick') != CAPTURE_TICK or \
                frame.get('world_tick') != CAPTURE_TICK or frame.get('mode') != 'Original' or frame.get('video_driver') != 'x11':
            raise RuntimeError('Split smoke receipt belongs to another completed native tick/mode/driver')
        views = frame.get('views', [])
        if len(views) != args.players or [v.get('view') for v in views] != list(range(args.players)):
            raise RuntimeError('Split smoke did not submit every requested local viewport')
        units = [v.get('local_unit') for v in views]
        if len(set(units)) != args.players or any(type(unit) is not int or not 0 <= unit < 0xffffffff for unit in units) or \
                any(v.get('local_unit_alive') is not True for v in views):
            raise RuntimeError('Split smoke lacks distinct actual living local-unit identities')
        # Native selection can retain candidates before the disabled root
        # route. Completed GPU submission, rather than candidate count, is
        # the policy boundary this smoke must prove.
        if any(v.get('submitted_player_roots') for v in views) or \
                any(frame['draws'][field] for field in FIELDS):
            raise RuntimeError('Native split policy submitted disabled fixed player shadows')
        selections = []
        for line in output.splitlines():
            if not line.startswith('Source shadow selection: '):
                continue
            values = dict((key, int(value)) for key, value in re.findall(r'(\w+)=(-?\d+)', line))
            if values.get('tick') == CAPTURE_TICK:
                selections.append(values)
        report['actual_native_selections'] = selections
        if {s.get('view') for s in selections} != set(range(args.players)) or any(
                s.get('fixed_enabled') != 0 for s in selections):
            raise RuntimeError('Actual completed native selection did not preserve the disabled split feature for every view')
        with timings.open() as stream:
            rows = list(csv.DictReader(stream, delimiter='\t'))
        report['submitted_frames'] = len(rows)
        report['draws'] = {field: sum(int(row[field]) for row in rows) for field in FIELDS}
        if not rows or any(report['draws'].values()):
            raise RuntimeError('Split smoke lost completed frames or emitted disabled GPU shadow packets')
        size, pixels = ppm(image)
        non_black = sum(max(pixels[i:i+3]) > 8 for i in range(0, len(pixels), 3))
        report['capture'] = {'size': size, 'non_black_pixels': non_black, 'sha256': hashlib.sha256(pixels).hexdigest()}
        if size != (640, 480) or non_black < 4096:
            raise RuntimeError('Split smoke did not capture its actual visible world')
        report['validation_checked'] = True
        report['passed'] = True
        print(f'PASS actual native fixed-shadow split policy players={args.players} tick={CAPTURE_TICK}')
    except Exception as error:
        report['error'] = str(error)
    finally:
        args.report_output.parent.mkdir(parents=True, exist_ok=True)
        data = json.dumps(report, indent=2) + '\n'
        if len(data.encode()) > 1024 * 1024:
            raise RuntimeError('Split smoke report exceeds its bounded retention limit')
        args.report_output.write_text(data)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
