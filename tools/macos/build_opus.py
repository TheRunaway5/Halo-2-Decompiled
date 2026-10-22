#!/usr/bin/env python3
"""Build the engine's pinned Opus dependency in a caller-owned build tree.

Invoked by cmake/macos/Opus.cmake. Validation must use tools/run_tests.py so
the SDK, downloads and both architecture builds are removed with its tree.
Normal engine builds retain their dependency SDK in their own build directory.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[2]
VERSION = '1.6.1'
URL = f'https://downloads.xiph.org/releases/opus/opus-{VERSION}.tar.gz'
SHA256 = '6ffcb593207be92584df15b32466ed64bbec99109f007c82205f0194572411a1'


def run(*args):
    subprocess.run([str(a) for a in args], check=True)


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--arch', action='append', required=True, choices=['arm64', 'x86_64'])
    parser.add_argument('--deployment-target', default='13.0')
    parser.add_argument('--sysroot')
    parser.add_argument('--jobs', type=int, default=2)
    args = parser.parse_args()
    if sys.platform != 'darwin' or args.jobs < 1:
        parser.error('This build requires macOS and a positive job count')
    args.work = args.work.resolve()
    args.prefix = args.prefix.resolve()
    args.work.mkdir(parents=True, exist_ok=True)
    archive = args.work / f'opus-{VERSION}.tar.gz'
    if not archive.is_file() or digest(archive) != SHA256:
        request = urllib.request.Request(URL, headers={'User-Agent': 'Halo2-macOS-dependency-build'})
        with urllib.request.urlopen(request, timeout=90) as response, archive.open('wb') as stream:
            shutil.copyfileobj(response, stream)
    if digest(archive) != SHA256:
        raise RuntimeError('Pinned Opus archive checksum mismatch')
    source = args.work / f'opus-{VERSION}'
    if not source.is_dir():
        with tarfile.open(archive) as stream:
            stream.extractall(args.work, filter='data')
    arches = list(dict.fromkeys(args.arch))
    libraries, headers = [], None
    configurations = {}
    for arch in arches:
        build = args.work / (arch + '-build')
        install = args.work / (arch + '-install')
        command = ['cmake', '-S', str(source), '-B', str(build), '-G', 'Ninja',
            '-DCMAKE_BUILD_TYPE=Release', f'-DCMAKE_INSTALL_PREFIX={install}',
            f'-DCMAKE_TOOLCHAIN_FILE={ROOT}/cmake/macos/DependencyToolchain.cmake',
            f'-DHALO2_DEPENDENCY_ARCH={arch}', f'-DCMAKE_OSX_DEPLOYMENT_TARGET={args.deployment_target}',
            '-DCMAKE_INSTALL_NAME_DIR=@rpath', '-DOPUS_BUILD_SHARED_LIBRARY=ON',
            '-DBUILD_TESTING=OFF', '-DOPUS_BUILD_TESTING=OFF', '-DOPUS_BUILD_PROGRAMS=OFF',
            '-DOPUS_FLOAT_APPROX=OFF', '-DOPUS_FAST_MATH=OFF']
        if args.sysroot:
            command.append('-DCMAKE_OSX_SYSROOT=' + args.sysroot)
        run(*command)
        run('cmake', '--build', build, '--parallel', args.jobs)
        run('cmake', '--install', build)
        library = (install / 'lib/libopus.0.dylib').resolve(strict=True)
        run('lipo', library, '-verify_arch', arch)
        libraries.append(library)
        actual_headers = {str(p.relative_to(install / 'include')): digest(p)
                          for p in sorted((install / 'include').rglob('*.h'))}
        if headers is not None and headers != actual_headers:
            raise RuntimeError('Opus public headers differ across architecture builds')
        headers = actual_headers
        configurations[arch] = dict(configure=command, library_sha256=digest(library))
    libdir = args.prefix / 'lib'
    libdir.mkdir(parents=True, exist_ok=True)
    merged = libdir / 'libopus.0.dylib'
    run('lipo', '-create', *libraries, '-output', merged)
    run('install_name_tool', '-id', '@rpath/libopus.0.dylib', merged)
    for arch in arches:
        run('lipo', merged, '-verify_arch', arch)
    run('codesign', '--force', '--sign', '-', merged)
    run('codesign', '--verify', '--strict', merged)
    link = libdir / 'libopus.dylib'
    if link.is_symlink():
        link.unlink()
    elif link.exists():
        raise RuntimeError('Refusing to replace a non-link Opus SDK alias')
    link.symlink_to(merged.name)
    shutil.copytree(args.work / (arches[0] + '-install/include'), args.prefix / 'include', dirs_exist_ok=True)
    licensedir = args.prefix / 'share/licenses'
    licensedir.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source / 'COPYING', licensedir / 'Opus.txt')
    pkgconfig = libdir / 'pkgconfig'
    pkgconfig.mkdir(exist_ok=True)
    (pkgconfig / 'opus.pc').write_text('prefix=${pcfiledir}/../..\nexec_prefix=${prefix}\n'
        'libdir=${prefix}/lib\nincludedir=${prefix}/include\n\nName: Opus\n'
        'Description: Opus codec\nVersion: ' + VERSION + '\n'
        'Libs: -L${libdir} -lopus\nLibs.private: -lm\nCflags: -I${includedir} -I${includedir}/opus\n')
    report = dict(schema='H2_MACOS_OPUS_SDK_V1', version=VERSION, source_url=URL,
        source_sha256=SHA256, architectures=arches, deployment_target=args.deployment_target,
        configurations=configurations, public_headers=headers, library_sha256=digest(merged),
        license_sha256=digest(licensedir / 'Opus.txt'), prefix=str(args.prefix))
    (args.prefix / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    (args.prefix / 'built.stamp').write_text(SHA256 + '\n')


if __name__ == '__main__':
    main()
