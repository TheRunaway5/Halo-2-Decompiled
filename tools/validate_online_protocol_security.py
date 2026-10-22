#!/usr/bin/env python3
"""Qualify real protocol sources in one owned, immutable source/build cohort.

Run through tools/run_test.py. Copies and builds live in HALO2_TEST_SCRATCH;
only bounded reports, source hashes and changed reference preimages survive.
"""
from source_layout import source_file
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
EXCLUDED = {'.git', '__pycache__', 'target', 'node_modules', 'build'}


def identity(path):
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + '\n')


def freeze(source, native):
    copied = {}

    def copy(path):
        relative = path.relative_to(ROOT)
        target = source / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        data = path.read_bytes()
        target.write_bytes(data)
        copied[str(relative)] = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}

    def tree(name):
        base = ROOT / name
        if not base.exists():
            return
        for path in sorted(base.rglob('*')):
            if any(part in EXCLUDED for part in path.relative_to(base).parts):
                continue
            if path.is_file():
                copy(path)

    source.mkdir()
    if native:
        copy(ROOT / 'CMakeLists.txt')
        for name in ('src', 'include', 'cmake', 'shaders', 'third_party', 'external', 'rust'):
            tree(name)
        for path in sorted((ROOT / 'tools').iterdir()):
            if path.is_file() and path.suffix in ('.py', '.cpp', '.hpp', '.h', '.c', '.S', '.sh', '.txt'):
                copy(path)
        for name in ('data', 'reference', 'pstencil'):
            path = ROOT / 'tools' / name
            if path.exists():
                (source / 'tools' / name).symlink_to(path, target_is_directory=True)
        for path in sorted((ROOT / 'tests').iterdir()):
            if path.is_file():
                copy(path)
            elif path.is_dir() and path.name not in ('fixtures', '__pycache__'):
                # Protocol helpers themselves are frozen, not borrowed.
                if path.name == 'online_protocol':
                    tree('tests/online_protocol')
                else:
                    (source / 'tests' / path.name).symlink_to(path, target_is_directory=True)
        fixtures = source / 'tests/fixtures'
        fixtures.mkdir()
        for path in (ROOT / 'tests/fixtures').iterdir():
            if path.is_file() and (path.stat().st_size <= 65536 or path.name == 'campaign_object_attachment_tu15.json'):
                copy(path)
            else:
                (fixtures / path.name).symlink_to(path, target_is_directory=path.is_dir())
        for name in ('assets', 'analysis', 'resources', 'data', 'multiplayer', '.tools'):
            if (ROOT / name).exists():
                (source / name).symlink_to(ROOT / name, target_is_directory=True)
    else:
        for name in ('rust/online_protocol', 'rust/online_protocol_bridge', 'tests/online_protocol'):
            tree(name)
        files = ['cmake/OnlineProtocolRust.cmake', 'cmake/TestStorage.cmake',
                 'tools/build_online_protocol_rust.py', 'tools/run_tests.py', 'tools/run_test.py']
        for stem in ('online_voice_protocol', 'native_request_protocol', 'authority_archive_protocol'):
            files += [f'include/halo2/{stem}.hpp', str(source_file(ROOT, stem + '.cpp').relative_to(ROOT)), f'tests/{stem}_tests.cpp']
        files += ['include/halo2/multiplayer_datagrams.hpp']
        for name in files:
            copy(ROOT / name)
    return copied


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--native', action='store_true')
    parser.add_argument('--offline-reference', action='store_true')
    parser.add_argument('--sanitizers', action='store_true')
    parser.add_argument('--windows', action='store_true', help='Windows GNU bridge build and Wine controls')
    parser.add_argument('--jobs', type=int, default=3)
    args = parser.parse_args()
    if args.offline_reference and not args.native:
        parser.error('--offline-reference requires --native')
    if args.windows and (args.native or args.sanitizers):
        parser.error('--windows covers the unsanitized bridge project only')
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('run through tools/run_test.py for owned source/build cleanup')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    source = scratch / 'source'
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    # Freeze the full code cohort even for the small bridge project. Shared
    # CMake/CXX integrations may add ordinary source/header dependencies while
    # another chat is working; a partial file allowlist is not a source owner.
    pins = freeze(source, True)
    initial_drift = [name for name, expected in pins.items() if identity(ROOT / name) != expected]
    save(output / 'source-manifest.json', {'source': str(source), 'scratch': str(scratch),
         'files': pins, 'drift_during_copy': initial_drift,
         'borrowed_large_reference_inputs': True,
         'scope': 'Copied production code cohort; borrowed assets are references, not MCC runtime acceptance'})
    if initial_drift:
        raise RuntimeError('Source changed during freeze: ' + ', '.join(initial_drift[:8]))
    targets = ['online_voice_protocol_tests', 'native_request_protocol_tests', 'authority_archive_protocol_tests']
    tests = ['online_voice_protocol', 'native_request_protocol', 'authority_archive_protocol']
    if args.native:
        targets += ['source_authority_schema3_tests']
        tests += ['source_authority_schema3']
        if args.offline_reference:
            targets += ['source_authority_codec_tests']
            tests += ['source_authority_codec_tu15']
    command = ['python3', str(source / 'tools/run_tests.py'), '--source',
               str(source if args.native else source / 'tests/online_protocol'),
               '--jobs', str(args.jobs), '--test-jobs', '1',
               '--filter', '^(' + '|'.join(tests) + ')$',
               '--scratch-root', str(scratch / 'build-owner'), '--output', str(output / 'cmake'),
               '--cmake-arg=-DHALO2_ONLINE_RUST_TOOLCHAIN=stable',
               '--cmake-arg=-DHALO2_ONLINE_RUST_OFFLINE=ON']
    for target in targets:
        command += ['--target', target]
    if args.sanitizers:
        command += ['--sanitizers']
    environment = dict(os.environ)
    # These codec tests never initialize the vendor transport. Its ABI stub is
    # compiled, but live SDK credentials/callbacks are outside this acceptance.
    environment.pop('HALO2_EOS_SDK_ROOT', None)
    if args.windows:
        environment.update(WINEPREFIX=str(scratch / 'wine'), WINEDEBUG='-all',
                           WINEDLLOVERRIDES='winemenubuilder.exe=d')
        command += ['--cmake-arg=-DCMAKE_SYSTEM_NAME=Windows',
                    '--cmake-arg=-DCMAKE_SYSTEM_PROCESSOR=x86_64',
                    '--cmake-arg=-DCMAKE_CXX_COMPILER=/usr/bin/x86_64-w64-mingw32-g++',
                    '--cmake-arg=-DCMAKE_CROSSCOMPILING_EMULATOR=/usr/bin/wine',
                    '--cmake-arg=-DCMAKE_EXE_LINKER_FLAGS=-static -static-libgcc -static-libstdc++']
    if args.native:
        command += [
            '--cmake-arg=-DFETCHCONTENT_SOURCE_DIR_HALO2_DATACHANNEL=' + str(ROOT / 'build-multiplayer/_deps/halo2_datachannel-src'),
            '--cmake-arg=-DHALO2_DISCORD_SOCIAL_SDK_ROOT=' + str(ROOT / '.local/discord-sdk/discord_social_sdk'),
            '--cmake-arg=-DHALO2_STAGE_BUILD_OUTPUT=OFF', '--cmake-arg=-DHALO2_BUNDLE_MOVIES=OFF',
            '--cmake-arg=-DHALO2_TU15_GAMEPLAY_XBE=']
    command += ['--retain-report', 'online-protocol/Release/build-report.json',
                '--retain-report', 'Testing/Temporary/LastTest.log',
                '--retain-report', 'CMakeFiles/authority_archive_protocol_tests.dir/link.txt']
    report = {'command': command, 'targets': targets, 'tests': tests, 'scratch': str(scratch),
              'status': 'running', 'passed': False,
              'scope': 'Archive/protocol qualification; transport SDK callbacks, deployment and full migration are not accepted'}
    save(output / 'qualification.json', report)
    exit_code = 0
    if not args.native and not args.sanitizers:
        env = dict(os.environ, RUSTUP_TOOLCHAIN='stable', RUSTUP_AUTO_INSTALL='0',
                   CARGO_INCREMENTAL='0', CARGO_TARGET_DIR=str(scratch / 'cargo-target'))
        cargo_steps = []
        for name, argv in [
            ('rust-tests', ['cargo', 'test', '--locked', '--offline']),
            ('rust-clippy', ['cargo', 'clippy', '--locked', '--offline', '--all-targets', '--', '-D', 'warnings']),
        ]:
            argv[2:2] = ['--manifest-path', str(source / 'rust/online_protocol/Cargo.toml')]
            with (output / (name + '.log')).open('w') as log:
                checked = subprocess.run(argv, env=env, stdout=log, stderr=subprocess.STDOUT, check=False)
            cargo_steps.append({'command': argv, 'exit_code': checked.returncode})
            report['cargo_steps'] = cargo_steps
            save(output / 'qualification.json', report)
            if checked.returncode:
                exit_code = checked.returncode
                break
    try:
        if not exit_code:
            exit_code = subprocess.run(command, env=environment, check=False).returncode
    finally:
        if args.windows and (scratch / 'wine').exists():
            cleanup = []
            for argv in (['wineserver', '-k'], ['wineserver', '-w']):
                stopped = subprocess.run(argv, env=environment, timeout=30, check=False,
                                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
                cleanup.append({'command': argv, 'exit_code': stopped.returncode,
                                'output': stopped.stdout[-1000:]})
            report['wine_cleanup'] = cleanup
    report.update(exit_code=exit_code, status='passed' if exit_code == 0 else 'failed',
                  passed=exit_code == 0)
    # Never silently promote a changing checkout to the compiled source cohort.
    # Retain only actually changed copied source inputs as reference evidence.
    closing = []
    for name, expected in pins.items():
        actual = identity(ROOT / name) if (ROOT / name).is_file() else None
        if actual != expected:
            destination = output / 'changed-source-preimages' / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source / name, destination)
            closing.append({'path': name, 'compiled': expected, 'current': actual})
        if identity(source / name) != expected:
            report.update(passed=False, status='failed')
            raise RuntimeError('Qualification source was mutated: ' + name)
    report['closing_worktree_drift'] = closing
    report['retained_changed_source_bytes'] = sum(row['compiled']['bytes'] for row in closing)
    save(output / 'qualification.json', report)
    return exit_code


if __name__ == '__main__':
    raise SystemExit(main())
