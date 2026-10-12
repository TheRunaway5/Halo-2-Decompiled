#!/usr/bin/env python3
"""Verify the selected native audio runtime through actual SDL/Vulkan/LAN paths.

Run inside an isolated X11 display. SDL's dummy output consumes real packets;
these checks do not measure physical speakers, Xbox DSP or original waveforms.
Every nonzero return, skip, timeout and changed binary remains a failure.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--image', type=Path, required=True)
    parser.add_argument('--audio-binary', type=Path,
                        help='Separately pinned input-harness binary using the selected runtime objects')
    parser.add_argument('--cases', nargs='+', required=True, choices=(
        'cpu', 'all-cpu', 'hud-xbox', 'hud-selected', 'species-single', 'species-mixed',
        'species-lan', 'world1', 'world4', 'prop-lan', 'local1', 'local4',
        'lan1', 'lan4', 'lan-slow'))
    parser.add_argument('--timeout', type=float, default=180)
    args = parser.parse_args()
    assert os.environ.get('DISPLAY'), 'An isolated X11 display is required'
    build, out, image = args.build.resolve(), args.output.resolve(), args.image.resolve()
    out.mkdir(parents=True, exist_ok=False)
    environment = os.environ.copy()
    environment['SDL_AUDIODRIVER'] = 'dummy'
    environment['SDL_VIDEODRIVER'] = 'x11'
    environment['HALO2_TEST_VALIDATION'] = '1'
    validation = ROOT / '.tools/validation/usr'
    environment['VK_LAYER_PATH'] = str(validation / 'share/vulkan/explicit_layer.d')
    environment['LD_LIBRARY_PATH'] = os.pathsep.join(filter(None, (
        str(validation / 'lib'), environment.get('LD_LIBRARY_PATH'))))
    maps = ROOT / 'assets/disc/maps'
    # The owned capture controls fire into the floor, release, then throw once.
    inputs = out / 'world.inputs.txt'
    inputs.write_text(''.join(f'{tick} 0 1 0 0 0 -1.2\n' for tick in range(15)) +
                      '15 0 0 0 0 0 -1.2\n60 0 0 0 1 1.35 -1.2\n70 0 0 0 0 1.35 0\n')
    audio = [(args.audio_binary.resolve() if args.audio_binary else build / 'multiplayer_audio_runtime_tests'),
             maps / 'lockout.map', maps, image]
    commands = {
        'cpu': ['ctest', '--test-dir', build, '--output-on-failure', '-R',
            r'^(original_live_sound_(admission|spatial)|original_sound_(driver_(spatial|parameters)|sdk_(spatial|spatial_data|hrtf|calculate|voice_data|device|commit|setters|construction|voice_initialization|hardware_voice|voice_dispatch|sample_pages|voice_attachment|notifications|voice_retirement|voice_stop))|multiplayer_authored_audio|native_hud_motion_sensor(_viewports)?)$'],
        'all-cpu': ['ctest', '--test-dir', build, '--output-on-failure', '-R',
            r'(sound|audio|announcer|footstep|^native_hud_motion_sensor(_viewports)?$)'],
        'hud-xbox': [build / 'hud_render_tests', maps, out / 'hud-xbox'],
        'hud-selected': [build / 'hud_render_tests', maps, out / 'hud-selected', ROOT / 'assets/mcc-classic-hud'],
        'prop-lan': [build / 'multiplayer_prop_audio_runtime_tests', maps / 'lockout.map', maps, image],
    }
    for mode in ('single', 'mixed', 'lan'):
        commands['species-' + mode] = [build / 'multiplayer_species_runtime_tests', ROOT,
                                      out / 'species', mode, image]
    for count in (1, 4):
        commands[f'world{count}'] = [build / 'multiplayer_world_audio_runtime_tests',
            maps / 'lockout.map', maps, image, inputs, out / f'world{count}.ppm',
            *(['--split4'] if count == 4 else [])]
    for name, extra in [('local1', []), ('local4', ['--split4']),
                       ('lan1', ['--lan']), ('lan4', ['--lan', '--split4']),
                       ('lan-slow', ['--lan', '--slow-client'])]:
        commands[name] = audio + extra
    binaries = {str(path): sha(path) for path in [build / 'halo2_native', *(
        Path(commands[name][0]) for name in args.cases if name not in ('cpu', 'all-cpu'))]}
    report = dict(boundary=__doc__, tool_sha256=sha(__file__), build=str(build),
        video_driver=environment['SDL_VIDEODRIVER'], audio_driver=environment['SDL_AUDIODRIVER'],
        binaries=binaries, image_sha256=sha(image), map_sha256=sha(maps / 'lockout.map'),
        controls_sha256=sha(inputs), cases=[])
    for name in args.cases:
        command = list(map(str, commands[name]))
        started = time.monotonic()
        timed_out = False
        launch_error = None
        with (out / f'{name}.log').open('w') as log:
            try:
                result = subprocess.run(command, cwd=ROOT, env=environment,
                    stdout=log, stderr=subprocess.STDOUT, timeout=args.timeout)
                code = result.returncode
            except subprocess.TimeoutExpired:
                timed_out, code = True, None
            except OSError as error:
                launch_error, code = str(error), None
                log.write(launch_error + '\n')
        log_text = (out / f'{name}.log').read_text()
        validation_errors = [line for line in log_text.splitlines()
                             if 'Validation Error' in line or 'ERROR / SPEC' in line]
        skipped = name in ('cpu', 'all-cpu') and ('***Skipped' in log_text or 'Not Run' in log_text or 'No tests were found' in log_text)
        report['cases'].append(dict(name=name, command=command, exit_code=code,
            timeout=timed_out, launch_error=launch_error, seconds=time.monotonic() - started,
            validation_errors=validation_errors, skipped=skipped,
            passed=code == 0 and not validation_errors and not skipped,
            log_sha256=sha(out / f'{name}.log')))
        (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        print(name, 'PASS' if report['cases'][-1]['passed'] else 'FAIL', flush=True)
    report['binaries_unchanged'] = all(sha(path) == pin for path, pin in binaries.items())
    report['passed'] = report['binaries_unchanged'] and all(c['passed'] for c in report['cases'])
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
