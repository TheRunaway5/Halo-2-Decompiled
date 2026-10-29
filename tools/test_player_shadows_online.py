#!/usr/bin/env python3
"""Observe actual host and joining-client shadow GPU frames over ONLINE direct UDP."""
import argparse
import hashlib
import json
import os
from pathlib import Path
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
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    output = scratch / 'online-reports'
    command = ['xvfb-run', '-a', '-s', '-screen 0 800x600x24', 'python3',
               str(Path(__file__).resolve().parents[1] / 'tests/source_authority_publisher_http_test.py'),
               str(args.fixture)]
    report = {'passed': False, 'command': command, 'mode': args.mode,
              'scope': 'Actual ONLINE direct UDP MCC host and joining-client completed GPU frames; fixture Discord; bounded map/mode coverage',
              'retained_large_artifacts': []}
    try:
        environment = {**os.environ, 'HALO2_SHADOW_TRANSPORT_ONLINE': '1',
                       'HALO2_SHADOW_TRANSPORT_TRACE': '1',
                       'HALO2_SHADOW_GPU_MODE': args.mode, 'HALO2_SHADOW_GPU_MAP': str(args.map),
                       'HALO2_PUBLISHER_TEST_OUTPUT': str(output),
                       'SDL_VIDEODRIVER': 'x11', 'SDL_AUDIODRIVER': 'dummy'}
        environment, report['validation_discovery'] = validation_environment(Path(__file__).resolve().parents[1], environment)
        with (scratch / 'online.log').open('w+') as log:
            result = subprocess.run(command, env=environment, stdout=log, stderr=subprocess.STDOUT, timeout=300)
            log.seek(0)
            report['diagnostic_tail'] = log.read()[-24000:]
        report['exit_code'] = result.returncode
        report['endpoint_diagnostics'] = {}
        for account in ('one', 'two'):
            path = output / f'{account}.log'
            if path.is_file():
                text = path.read_text()
                report['endpoint_diagnostics'][account] = {
                    'tail': text[-16000:],
                    'native_adoptions': receipts(text, '[shadow-presentation] '),
                    'transport_progress': [line for line in text.splitlines()
                                           if line.startswith('Native shadow client transport: ')][-16:]}
        if result.returncode:
            raise RuntimeError(f'Actual ONLINE shadow fixture exited {result.returncode}')
        endpoints = report['endpoints'] = []
        for account in ('one', 'two'):
            runtime = json.loads((output / f'native-shadow-{account}.json').read_text())
            text = (output / f'{account}.log').read_text()
            validate_output(text)
            frames = receipts(text, 'Player shadow frame: ')
            if not frames:
                raise RuntimeError(f'{account} has no actual completed shadow GPU frame receipt')
            frame = frames[-1]
            if not runtime['passed'] or runtime['mode'] != args.mode or runtime['tick'] < 180 or \
                    not frame.get('completed') or frame['tick'] != runtime['tick'] or frame['mode'] != args.mode or \
                    frame['video_driver'] != 'x11' or len(frame.get('views', [])) != 1:
                raise RuntimeError(f'{account} captured a different actual tick, graphics mode, driver or view')
            if frame.get('world_tick') != runtime.get('native_root_tick') or not 180 <= frame['world_tick'] <= frame['tick'] or \
                    frame.get('native_world_tick') != frame['world_tick'] or (runtime['role'] == 'joining-client' and frame.get('native_cut_serial', 0) <= 0):
                raise RuntimeError(f'{account} mixed native WORLD/body clocks at its settled GPU capture')
            view = frame['views'][0]
            if view['local_unit'] != runtime['local_unit'] or view.get('local_unit_alive') is not True or \
                    runtime.get('local_unit_alive') is not True or len(runtime.get('players', [])) != 2:
                raise RuntimeError(f'{account} lost its living local full-salt owner or actual peer')
            picture = Path(runtime['picture']).resolve()
            if not picture.is_relative_to(scratch):
                raise RuntimeError('ONLINE frame escaped owned scratch')
            size, pixels = ppm(picture)
            non_black = sum(max(pixels[first:first+3]) > 8 for first in range(0, len(pixels), 3))
            if size != (640, 480) or non_black < 4096 or len(set(pixels)) < 16:
                raise RuntimeError(f'{account} lacks its actual visible MCC world')
            endpoint = {'runtime': runtime, 'captured_frame': frame, 'validation_checked': True,
                        'capture': {'size': size, 'non_black_pixels': non_black,
                                    'sha256': hashlib.sha256(pixels).hexdigest()}}
            if args.mode == 'PathTracing':
                rays = receipts(text, 'Path Tracing frame: ')
                if not rays or not rays[-1].get('completed'):
                    raise RuntimeError(f'{account} never completed its actual ray frame')
                endpoint['path_tracing'] = rays[-1]
                endpoint['local_body_triangles'] = local_body_triangles(rays[-1], frame)
                if endpoint['local_body_triangles'] <= 0:
                    raise RuntimeError(f'{account} lost the full local body from final ray geometry')
            elif view['local_unit'] not in view['selected_player_roots'] or not any(
                    root['owner'] == view['local_unit'] and root['caster_draws'] and root['receiver_draws']
                    for root in view['submitted_player_roots']):
                raise RuntimeError(f'{account} local player missed native Generate or Apply GPU submission')
            endpoints.append(endpoint)
        if {entry['runtime']['role'] for entry in endpoints} != {'host', 'joining-client'} or \
                endpoints[0]['runtime']['local_unit'] == endpoints[1]['runtime']['local_unit']:
            raise RuntimeError('ONLINE receipts did not observe two actual host/client local owners')
        if {entry['player'] for entry in endpoints[0]['runtime']['players']} != \
                {entry['player'] for entry in endpoints[1]['runtime']['players']}:
            raise RuntimeError('ONLINE host/client frames disagree on actual admitted player allocations')
        report['fixture_sha256'] = hashlib.sha256(args.fixture.read_bytes()).hexdigest()
        report['passed'] = True
    except Exception as error:
        report['error'] = str(error)
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report.get(key) for key in ('passed', 'mode', 'error')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
