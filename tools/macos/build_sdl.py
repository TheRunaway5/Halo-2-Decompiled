#!/usr/bin/env python3
"""Build pinned SDL2 slices in the caller's owned dependency directory.

Validation runs through tools/run_tests.py; the enclosing owner removes the
archive, source, per-CPU builds, installs and combined SDK together.
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

from bundle import dependencies, system_library

ROOT = Path(__file__).resolve().parents[2]
VERSION = '2.32.10'
URL = f'https://github.com/libsdl-org/SDL/releases/download/release-{VERSION}/SDL2-{VERSION}.tar.gz'
SHA256 = '5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165'
LICENSE_SHA256 = '97f35b302b361680ec1e891e95d2d52097bb95abff361434916d99dc1305f127'
LIBRARY = 'libSDL2-2.0.0.dylib'


def run(*args):
    subprocess.run([str(x) for x in args], check=True)


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
    archive = args.work / f'SDL2-{VERSION}.tar.gz'
    if not archive.is_file() or digest(archive) != SHA256:
        request = urllib.request.Request(URL, headers={'User-Agent': 'Halo2-macOS-dependency-build'})
        with urllib.request.urlopen(request, timeout=90) as response, archive.open('wb') as stream:
            shutil.copyfileobj(response, stream)
    if digest(archive) != SHA256:
        raise RuntimeError('Pinned SDL archive checksum mismatch')
    source = args.work / f'SDL2-{VERSION}'
    if not source.is_dir():
        with tarfile.open(archive) as stream:
            stream.extractall(args.work, filter='data')
    if digest(source / 'LICENSE.txt') != LICENSE_SHA256:
        raise RuntimeError('Pinned SDL source notice differs')
    arches = list(dict.fromkeys(args.arch))
    libraries, headers, configurations = [], None, {}
    for arch in arches:
        build, install = args.work / (arch + '-build'), args.work / (arch + '-install')
        command = ['cmake', '-S', str(source), '-B', str(build), '-G', 'Ninja',
            '-DCMAKE_BUILD_TYPE=Release', f'-DCMAKE_INSTALL_PREFIX={install}',
            f'-DCMAKE_TOOLCHAIN_FILE={ROOT}/cmake/macos/DependencyToolchain.cmake',
            f'-DHALO2_DEPENDENCY_ARCH={arch}', f'-DCMAKE_OSX_DEPLOYMENT_TARGET={args.deployment_target}',
            '-DCMAKE_INSTALL_NAME_DIR=@rpath', '-DSDL_SHARED=ON', '-DSDL_STATIC=OFF',
            '-DSDL_TEST=OFF', '-DSDL_TESTS=OFF', '-DSDL2_DISABLE_INSTALL=OFF',
            '-DSDL_COCOA=ON', '-DSDL_METAL=ON', '-DSDL_VULKAN=ON', '-DSDL_HIDAPI_LIBUSB=OFF']
        if args.sysroot:
            command.append('-DCMAKE_OSX_SYSROOT=' + args.sysroot)
        run(*command)
        run('cmake', '--build', build, '--parallel', args.jobs)
        run('cmake', '--install', build)
        library = (install / 'lib' / LIBRARY).resolve(strict=True)
        run('lipo', library, '-verify_arch', arch)
        actual = {str(p.relative_to(install / 'include/SDL2')): digest(p)
                  for p in sorted((install / 'include/SDL2').rglob('*.h'))}
        public = {n: sha for n, sha in actual.items() if n != 'SDL_config.h'}
        if headers is not None and headers != public:
            raise RuntimeError('SDL public headers differ between CPU builds')
        headers = public
        loads = dependencies(library, arch)
        if any(not system_library(x) for x in loads):
            raise RuntimeError('SDL unexpectedly depends on a non-system library: ' + repr(loads))
        libraries.append(library)
        configurations[arch] = dict(configure=command, headers=actual,
            library_sha256=digest(library), dependencies=loads)
    libdir, include = args.prefix / 'lib', args.prefix / 'include/SDL2'
    libdir.mkdir(parents=True, exist_ok=True)
    shutil.copytree(args.work / (arches[0] + '-install/include/SDL2'), include, dirs_exist_ok=True)
    # SDL's generated configuration describes CPU capabilities. Preserve each
    # real configuration instead of borrowing the builder's ARM header on x86.
    for arch in arches:
        shutil.copyfile(args.work / (arch + '-install/include/SDL2/SDL_config.h'),
                        include / ('SDL_config_' + arch + '.h'))
    dispatch = []
    for i, arch in enumerate(arches):
        condition = 'defined(__aarch64__) || defined(__arm64__)' if arch == 'arm64' else 'defined(__x86_64__)'
        dispatch += [('#if ' if i == 0 else '#elif ') + condition,
                     '#include "SDL_config_' + arch + '.h"']
    dispatch += ['#else', '#error "SDL SDK has no configuration for this CPU"', '#endif', '']
    (include / 'SDL_config.h').write_text('\n'.join(dispatch))
    merged = libdir / LIBRARY
    run('lipo', '-create', *libraries, '-output', merged)
    run('install_name_tool', '-id', '@rpath/' + LIBRARY, merged)
    for arch in arches:
        run('lipo', merged, '-verify_arch', arch)
    run('codesign', '--force', '--sign', '-', merged)
    run('codesign', '--verify', '--strict', merged)
    alias = libdir / 'libSDL2.dylib'
    if alias.is_symlink():
        alias.unlink()
    elif alias.exists():
        raise RuntimeError('Refusing to replace a non-link SDL SDK alias')
    alias.symlink_to(merged.name)
    licensedir = args.prefix / 'share/licenses'
    licensedir.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source / 'LICENSE.txt', licensedir / 'SDL2.txt')
    report = dict(schema='H2_MACOS_SDL_SDK_V1', version=VERSION, source_url=URL,
        source_sha256=SHA256, architectures=arches, deployment_target=args.deployment_target,
        configurations=configurations, public_headers=headers, library_sha256=digest(merged),
        config_dispatch_sha256=digest(include / 'SDL_config.h'), license_sha256=LICENSE_SHA256,
        prefix=str(args.prefix))
    (args.prefix / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    (args.prefix / 'built.stamp').write_text(SHA256 + '\n')


if __name__ == '__main__':
    main()
