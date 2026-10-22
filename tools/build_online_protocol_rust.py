#!/usr/bin/env python3
"""Build the pinned Rust/CXX module inside the caller's CMake build tree."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys


def run(command, cwd, env):
    result = subprocess.run(command, cwd=cwd, env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    print(result.stdout, end='', flush=True)
    if result.returncode:
        raise RuntimeError(f'Rust build command failed with exit {result.returncode}')
    return result.stdout


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cargo', required=True)
    parser.add_argument('--rustc', required=True)
    parser.add_argument('--toolchain', default='1.96.0')
    parser.add_argument('--target', action='append', required=True)
    parser.add_argument('--cxx', required=True)
    parser.add_argument('--cxx-flags', default='')
    parser.add_argument('--library-name', required=True)
    parser.add_argument('--deployment-target', default='')
    parser.add_argument('--sysroot', default='')
    parser.add_argument('--offline', action='store_true')
    parser.add_argument('--eos-sdk-root', type=Path)
    args = parser.parse_args()
    manifest = args.manifest.resolve(strict=True)
    output = args.output.resolve()
    if output == manifest.parent or output.is_relative_to(manifest.parent):
        raise ValueError('Cargo output must be in the CMake build tree, outside source')
    output.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.update(CARGO_TARGET_DIR=str(output/'cargo'), CARGO_INCREMENTAL='0',
               RUSTUP_TOOLCHAIN=args.toolchain, RUSTUP_AUTO_INSTALL='0', CXX=args.cxx)
    env.pop('HALO2_EOS_SDK_ROOT', None)
    if args.eos_sdk_root:
        env['HALO2_EOS_SDK_ROOT'] = str(args.eos_sdk_root.resolve(strict=True))
    flags = args.cxx_flags
    if args.sysroot:
        flags += ' -isysroot ' + shlex.quote(args.sysroot)
    env['CXXFLAGS'] = flags
    if args.deployment_target:
        env['MACOSX_DEPLOYMENT_TARGET'] = args.deployment_target
    version = run([args.rustc, '--version', '--verbose'], manifest.parent, env)
    if not re.search(r'^release: 1\.96\.0$', version, re.M):
        raise ValueError('ONLINE protocol requires the pinned Rust 1.96.0 toolchain')
    lock = manifest.parent/'Cargo.lock'
    before_lock = digest(lock)
    archives, native_flags, headers = [], None, None
    for target in args.target:
        command = [args.cargo, 'rustc', '--manifest-path', str(manifest), '--locked',
                   '--release', '--lib', '--target', target]
        if args.offline:
            command.append('--offline')
        command += ['--', '--print=native-static-libs']
        text = run(command, manifest.parent, env)
        matches = re.findall(r'native-static-libs: (.+)', text)
        if not matches:
            raise ValueError('rustc did not report the actual native linker requirements')
        selected = shlex.split(matches[-1], posix='windows-msvc' not in target)
        if native_flags is not None and native_flags != selected:
            raise ValueError('Universal Rust slices have different native link requirements')
        native_flags = selected
        archive = output/'cargo'/target/'release'/args.library_name
        if not archive.is_file():
            raise ValueError(f'Missing compiled Rust archive: {archive}')
        archives.append(archive)
        include = output/'cargo'/'cxxbridge'
        names = ['rust/cxx.h', 'halo2-online-protocol-bridge/src/lib.rs.h']
        current = {name: (include/name).read_bytes() for name in names}
        current['src/eos_sdk.hpp'] = (manifest.parent/'src/eos_sdk.hpp').read_bytes()
        if headers is not None and current != headers:
            raise ValueError('Universal Rust slices disagree on their generated interface')
        headers = current
    destination = output/args.library_name
    if len(archives) == 1:
        shutil.copyfile(archives[0], destination)
    else:
        run(['lipo', '-create', *map(str, archives), '-output', str(destination)],
            manifest.parent, env)
    for name, data in headers.items():
        path = output/'include'/name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    # The final C++ linker consumes rustc's actual native dependencies. Do not
    # guess a platform library list or pass an rlib to the native linker.
    response = (subprocess.list2cmdline(native_flags) if any('windows-msvc' in t for t in args.target)
                else ' '.join(shlex.quote(value) for value in native_flags))
    (output/'native-libs.rsp').write_text(response+'\n')
    if digest(lock) != before_lock:
        raise ValueError('Locked Rust build changed Cargo.lock')
    report = {'scope': 'Rust static library and generated CXX interface, not runtime acceptance',
              'rustc': version, 'targets': args.target, 'lock_sha256': before_lock,
              'cxx_version': '1.0.205', 'native_flags': native_flags,
              'requested_cxx': args.cxx, 'requested_cxx_flags': env['CXXFLAGS'],
              'archive': str(destination), 'archive_sha256': digest(destination),
              'header_sha256': {name: hashlib.sha256(data).hexdigest() for name, data in headers.items()},
              'cargo_target_dir': env['CARGO_TARGET_DIR']}
    (output/'build-report.json').write_text(json.dumps(report, indent=2)+'\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
