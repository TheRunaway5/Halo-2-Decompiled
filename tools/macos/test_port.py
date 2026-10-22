#!/usr/bin/env python3
"""Disposable macOS universal graphics validation with pinned upstream SDK inputs.

Invoke through tools/run_test.py. All downloaded SDKs and mounted images live
under HALO2_TEST_SCRATCH; only run_tests.py's bounded reports survive.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[2]
INPUTS = {
    'MoltenVK-macos.tar': (
        'https://github.com/KhronosGroup/MoltenVK/releases/download/v1.4.2/MoltenVK-macos.tar',
        'f95765a6229cb7b915990a2890ce12ebe36a730b021545d3d52ae69ce4c4024e'),
    'SDL2.dmg': (
        'https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.dmg',
        '4a7ac31640d70214e848f994be8a12849c0f97918a7e6c2e27a40036166d1a7f'),
    'SDL2-LICENSE.txt': (
        'https://raw.githubusercontent.com/libsdl-org/SDL/release-2.32.10/LICENSE.txt',
        '97f35b302b361680ec1e891e95d2d52097bb95abff361434916d99dc1305f127'),
}


def main():
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(143))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/test-results/macos-universal')
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--engine-check', action='store_true', help='Attempt the full engine with installed host dependencies')
    mode.add_argument('--engine-configure-check', action='store_true',
                      help='Configure the full engine, then build/run only the portability test')
    parser.add_argument('--engine-target', action='append', default=[],
                        help='Also build this target during an engine configure/check run')
    parser.add_argument('--engine-test-filter', help='CTest expression for the selected engine component targets')
    parser.add_argument('--retain-report', action='append', default=[],
                        help='Small private-build report to retain through tools/run_tests.py')
    parser.add_argument('--cmake-arg', action='append', default=[],
                        help='Additional configure argument forwarded to tools/run_tests.py')
    parser.add_argument('--source', type=Path,
                        help='Standalone CMake component source for an engine configure/check run')
    parser.add_argument('--owned-build-worker', action='store_true',
                        help='Use the enclosing run_test.py owner for an active incremental investigation')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True, exist_ok=True)
    for name, (url, expected) in INPUTS.items():
        path = scratch / name
        request = urllib.request.Request(url, headers={'User-Agent': 'Halo2-macOS-port-validation'})
        with urllib.request.urlopen(request, timeout=90) as response, path.open('wb') as stream:
            shutil.copyfileobj(response, stream)
        with path.open('rb') as stream:
            actual = hashlib.file_digest(stream, 'sha256').hexdigest()
        if actual != expected:
            raise RuntimeError('Upstream input checksum mismatch: ' + name)
    (args.output / 'dependencies.json').write_text(json.dumps(INPUTS, indent=2) + '\n')
    sdk = scratch / 'sdk'
    sdk.mkdir()
    with tarfile.open(scratch / 'MoltenVK-macos.tar') as archive:
        archive.extractall(sdk, filter='data')
    libraries = list(sdk.glob('**/dynamic/dylib/macOS/libMoltenVK.dylib'))
    if len(libraries) != 1:
        raise RuntimeError('Unexpected MoltenVK distribution layout')
    license_path = sdk / 'MoltenVK/LICENSE'
    if not license_path.is_file():
        raise RuntimeError('MoltenVK distribution has no root license')
    mount = scratch / 'sdl-mount'
    mount.mkdir()
    try:
        subprocess.run(['hdiutil', 'attach', '-nobrowse', '-readonly', '-mountpoint', str(mount),
                        str(scratch / 'SDL2.dmg')], check=True)
        shutil.copytree(mount / 'SDL2.framework', sdk / 'SDL2.framework', symlinks=True)
    finally:
        if os.path.ismount(mount):
            subprocess.run(['hdiutil', 'detach', str(mount)], check=True, timeout=30)
    framework = sdk / 'SDL2.framework'
    config = sdk / 'cmake/SDL2'
    config.mkdir(parents=True)
    (config / 'SDL2Config.cmake').write_text(
        'add_library(SDL2::SDL2 SHARED IMPORTED)\n'
        f'set_target_properties(SDL2::SDL2 PROPERTIES IMPORTED_LOCATION "{framework}/SDL2" '
        f'INTERFACE_INCLUDE_DIRECTORIES "{framework}/Headers")\n')
    engine_configuration = args.engine_check or args.engine_configure_check
    if (args.engine_target or args.engine_test_filter or args.source) and not engine_configuration:
        parser.error('--engine-target/--engine-test-filter/--source require an engine check mode')
    targets = ['vulkan_portability_tests']
    targets.extend(args.engine_target)
    if args.engine_check:
        targets.insert(0, 'halo2_native')
    elif not engine_configuration:
        targets.insert(0, 'halo2_macos_graphics_probe')
    command = [sys.executable, '-B', str(ROOT / 'tools/run_tests.py'), '--output', str(args.output),
               *[value for target in targets for value in ('--target', target)],
               '--jobs', '2', '--test-jobs', '1',
               '--filter', (args.engine_test_filter or '^vulkan_portability$') if engine_configuration else '^(macos_moltenvk_|macos_bundle$|vulkan_portability$)',
               '--cmake-arg=-GNinja', f'--cmake-arg=-DSDL2_DIR={config}',
               f'--cmake-arg=-DHALO2_MOLTENVK_LIBRARY={libraries[0]}',
               f'--cmake-arg=-DHALO2_MOLTENVK_LICENSE={license_path}',
               f'--cmake-arg=-DHALO2_SDL_LICENSE={scratch / "SDL2-LICENSE.txt"}']
    if args.owned_build_worker:
        command.append('--build-worker')
    command += [value for report in args.retain_report for value in ('--retain-report', report)]
    if args.source:
        command += ['--source', str(args.source.resolve(strict=True))]
    command += ['--cmake-arg=' + value for value in args.cmake_arg]
    if engine_configuration:
        command += ['--cmake-arg=-DHALO2_STAGE_BUILD_OUTPUT=OFF', '--cmake-arg=-DHALO2_BUNDLE_MOVIES=OFF']
    else:
        command += ['--cmake-arg=-DHALO2_MACOS_BRINGUP_ONLY=ON',
                    '--retain-report', 'macos-arm64.json', '--retain-report', 'macos-x86_64.json',
                    '--retain-report', 'Halo 2 Graphics Check.bundle.json',
                    '--retain-report', 'Halo 2 Graphics Check.app/Contents/Resources/bundle-manifest.json']
    return subprocess.run(command).returncode


if __name__ == '__main__':
    raise SystemExit(main())
