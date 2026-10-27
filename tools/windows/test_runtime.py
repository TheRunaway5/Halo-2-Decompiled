#!/usr/bin/env python3
"""Check a delivered native executable's real Vulkan startup in owned scratch.

Use tools/run_test.py. On Linux this uses a private Wine prefix; on Windows it
runs the PE executable directly. No MCC asset or gameplay acceptance is implied.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--menu-maps', type=Path, help='Also render the real frontend from this prepared MCC maps directory')
    parser.add_argument('--menu-ui-assets', type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    executable = args.executable.resolve(strict=True)
    environment = os.environ.copy()
    command = [str(executable), '--vulkan-smoke']
    wine = os.name != 'nt' and executable.suffix.lower() == '.exe'
    if wine:
        if not shutil.which('wine') or not shutil.which('wineserver'):
            parser.error('Wine and wineserver are required to execute a Windows PE image on Linux')
        environment.update(WINEPREFIX=str(scratch/'wine'), WINEDEBUG='-all',
                           WINEARCH='win64', SDL_VIDEODRIVER='windows',
                           WINEDLLOVERRIDES='winemenubuilder.exe=d;winewayland.drv=d')
        # Windows file owners require Windows absolute paths, even though
        # the surrounding test supervisor owns this POSIX scratch directory.
        windows_scratch = 'Z:' + str(scratch).replace('/', chr(92))
        environment.update(TMP=windows_scratch, TEMP=windows_scratch,
                           HALO2_TEST_SCRATCH=windows_scratch)
        command.insert(0, 'wine')
    with executable.open('rb') as stream:
        executable_sha256 = hashlib.file_digest(stream, 'sha256').hexdigest()
    report = {'passed': False, 'command': command, 'executable': str(executable),
              'sha256': executable_sha256,
              'execution_host': 'Wine on Linux' if wine else os.name,
              'scope': 'native executable SDL/Vulkan instance, GPU, surface, swapchain and three presented frames',
              'scratch': str(scratch), 'retained_large_artifacts': []}
    try:
        if wine:
            boot_environment = environment | {'DISPLAY': '', 'WAYLAND_DISPLAY': ''}
            started = time.monotonic()
            boot = subprocess.run(['wineboot', '--init'], env=boot_environment,
                                  capture_output=True, text=True, errors='replace', timeout=240)
            report['wine_boot'] = {'exit_code': boot.returncode, 'seconds': time.monotonic()-started,
                                   'stdout': boot.stdout[-16384:], 'stderr': boot.stderr[-16384:]}
            if boot.returncode:
                raise RuntimeError('Private Wine prefix initialization failed')
            subprocess.run(['wineserver', '-w'], env=environment, capture_output=True, timeout=30, check=True)
        result = subprocess.run(command, cwd=executable.parent, env=environment,
                                capture_output=True, text=True, errors='replace', timeout=120)
        report.update(exit_code=result.returncode, stdout=result.stdout[-32768:], stderr=result.stderr[-32768:])
        for line in result.stdout.splitlines():
            try:
                evidence = json.loads(line)
            except json.JSONDecodeError:
                continue
            if isinstance(evidence, dict) and evidence.get('presented_frames') == 3:
                report['vulkan'] = evidence
        report['passed'] = result.returncode == 0 and 'vulkan' in report
        if report['passed'] and args.menu_maps:
            mainmenu = args.menu_maps.resolve(strict=True) / 'mainmenu.map'
            with mainmenu.open('rb') as stream:
                report['frontend_inputs'] = {'mainmenu': str(mainmenu),
                    'bytes': mainmenu.stat().st_size,
                    'sha256': hashlib.file_digest(stream, 'sha256').hexdigest()}
            def native_path(path):
                value = str(path.resolve(strict=True))
                return 'Z:' + value.replace('/', chr(92)) if wine else value
            menu = [str(executable), '--menu-screen', '9', '--skip-intro', '--mute', '--menu-wait-intro-clear',
                    '--menu-maps', native_path(args.menu_maps), '--frames', '3']
            if args.menu_ui_assets:
                menu += ['--menu-ui-assets', native_path(args.menu_ui_assets)]
            if wine:
                menu.insert(0, 'wine')
            started = time.monotonic()
            menu_result = subprocess.run(menu, cwd=executable.parent, env=environment,
                                         capture_output=True, text=True, errors='replace', timeout=300)
            report['frontend'] = {'command': menu, 'exit_code': menu_result.returncode,
                                  'seconds': time.monotonic()-started,
                                  'stdout': menu_result.stdout[-32768:], 'stderr': menu_result.stderr[-32768:]}
            for line in menu_result.stdout.splitlines():
                if line.startswith('Frontend intro capture: '):
                    report['frontend']['presentation'] = json.loads(line.split(': ', 1)[1])
            presented = report['frontend'].get('presentation', {})
            report['passed'] = (menu_result.returncode == 0 and presented.get('clear_observed') is True
                                and presented.get('qualified_frames') == 3)
    except subprocess.TimeoutExpired as error:
        report.update(passed=False, error=str(error))
        for field in ('stdout', 'stderr'):
            value = getattr(error, field) or b''
            report[field] = value[-32768:].decode(errors='replace') if isinstance(value, bytes) else value[-32768:]
    except Exception as error:
        report.update(passed=False, error=str(error))
    finally:
        if wine:
            for arguments in (['wineserver', '-k'], ['wineserver', '-w']):
                try:
                    subprocess.run(arguments, env=environment, capture_output=True, timeout=20)
                except (OSError, subprocess.TimeoutExpired) as error:
                    report.update(passed=False, cleanup_error=str(error))
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
