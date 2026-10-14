#!/usr/bin/env python3
"""Actual SDL/Vulkan weapon regressions using authored pickup rows.

The tests declare diagnostic player placement, a private X11 display, SDL dummy
audio, one game tick per rendered frame, and mid-tick animation sampling. They
prove native input/pose/material/PCM submission, not original framebuffer equality,
physical audible output, or complete original weapon/physics ownership.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import select
import subprocess
from contextlib import contextmanager

ROOT = Path(__file__).resolve().parents[1]


@contextmanager
def private_display(output):
    """Prevent other native probes and physical input from changing focus."""
    read, write = os.pipe()
    with (output / 'xvfb.log').open('w') as log:
        process = subprocess.Popen(['Xvfb', '-displayfd', str(write), '-screen',
            '0', '1024x768x24', '-nolisten', 'tcp', '-noreset'],
            pass_fds=(write,), stdout=log, stderr=subprocess.STDOUT)
        os.close(write)
        try:
            assert select.select([read], [], [], 10)[0], 'Private X11 startup timed out'
            number = os.read(read, 64).decode().strip()
            assert number.isdigit(), 'Private X11 display was not allocated'
            yield ':' + number
        finally:
            os.close(read)
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--build', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--weapon', action='append', help='Run only named cases; repeat as needed')
    ap.add_argument('--xbe', type=Path, default=Path.home() /
        '.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
    args = ap.parse_args()
    assert hashlib.sha256(args.xbe.read_bytes()).hexdigest() == '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
    build = args.build.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    env.update(SDL_AUDIODRIVER='dummy', SDL_VIDEODRIVER='x11', SDL_VIDEO_DRIVER='x11',
        HALO2_ANIMATION_VALIDATION='1',
        VK_LAYER_PATH=str(ROOT / '.tools/validation/usr/share/vulkan/explicit_layer.d'))
    env['LD_LIBRARY_PATH'] = ':'.join([str(ROOT / '.tools/validation/usr/lib'),
        str(ROOT / '.tools/validation/usr/lib/x86_64-linux-gnu'), env.get('LD_LIBRARY_PATH', '')])
    cases = [('battle_rifle', 'lockout', []), ('magnum', 'ascension', []),
        ('covenant_carbine', 'burial_mounds', []), ('sniper_rifle', 'lockout', []),
        ('brute_shot', 'coagulation', []), ('rocket_launcher', 'ascension', []),
        ('shotgun', 'lockout', []), ('needler', 'lockout', ['', 'ammo']),
        ('sword', 'lockout', ['', 'ready']),
        ('plasma_rifle', 'lockout', ['', 'heat']),
        ('beam_rifle', 'burial_mounds', ['', 'heat']),
        ('brute_plasma_rifle', str(Path.home()/'.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/c/4d53006400000002/turf.map'), ['', 'heat'])]
    if args.weapon:
        assert set(args.weapon) <= {weapon for weapon, _, _ in cases}, 'Unknown weapon case'
        cases = [case for case in cases if case[0] in args.weapon]
    with private_display(args.output) as display:
        env['DISPLAY'] = display
        run_cases(args, build, env, cases)


def run_cases(args, build, env, cases):
    results = []
    for weapon, map_, extra in cases:
        map_path=Path(map_) if map_.endswith('.map') else ROOT/'assets/disc/maps'/(map_+'.map')
        command = [str(build / 'native_covenant_weapon_runtime_tests'),
            str(map_path),
            str(ROOT / 'assets/disc/maps'), str(args.xbe), weapon, *extra]
        log = args.output / (weapon + '.log')
        with log.open('w') as output:
            try:
                process = subprocess.run(command, env=env, stdout=output,
                    stderr=subprocess.STDOUT, timeout=90)
                status = process.returncode
            except subprocess.TimeoutExpired:
                status = 'timeout'
        text = log.read_text()
        errors = len(re.findall(r'Validation Error|VUID-', text))
        passed = (status == 0 and f'PASS {weapon}' in text and errors == 0 and
                  'Native SDL video driver: x11' in text)
        result = dict(weapon=weapon, map=map_, command=command, exit=status,
            passed=passed, validation_errors=errors, log=str(log),
            log_sha256=hashlib.sha256(log.read_bytes()).hexdigest(),
            observations=[line for line in text.splitlines() if line.startswith(
                ('PASS ', 'Ordinary firing observations:', 'Needler live effects:',
                 'Needler ammunition observations:', 'Ready observations:', 'Multiplayer frames=',
                 'Energy heat observations:', 'Attack observations:', 'Native SDL video driver:'))])
        results.append(result)
        report = dict(scope=__doc__, tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            executable_sha256=hashlib.sha256(Path(command[0]).read_bytes()).hexdigest(),
            xbe_sha256=hashlib.sha256(args.xbe.read_bytes()).hexdigest(), cases=results)
        (args.output / 'report.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
        print(json.dumps(dict(weapon=weapon, passed=passed, exit=status, validation_errors=errors)), flush=True)
    if not all(result['passed'] for result in results):
        raise SystemExit('Some actual weapon runtime cases failed; see retained logs.')


if __name__ == '__main__':
    main()
