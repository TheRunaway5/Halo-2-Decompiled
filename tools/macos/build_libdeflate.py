#!/usr/bin/env python3
"""Build checksum-pinned libdeflate in the caller's owned build directory.

CMake validation uses tools/run_tests.py, which removes downloads, both CPU
builds and the combined SDK along with the private build owner.
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
VERSION = '1.26'
URL = f'https://github.com/ebiggers/libdeflate/releases/download/v{VERSION}/libdeflate-{VERSION}.tar.gz'
SHA256 = '125856d4656e0feab660f94842f835923410c9281fedbcee64598c918da42b5a'
LICENSE_SHA256 = '4ad69099cb4374836fd27583d9991e2838cd86b6e4666ab26b2d32582c91e73a'
LIBRARY = 'libdeflate.0.dylib'


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
    args.work, args.prefix = args.work.resolve(), args.prefix.resolve()
    args.work.mkdir(parents=True, exist_ok=True)
    archive = args.work / f'libdeflate-{VERSION}.tar.gz'
    if not archive.is_file() or digest(archive) != SHA256:
        request = urllib.request.Request(URL, headers={'User-Agent': 'Halo2-macOS-dependency-build'})
        with urllib.request.urlopen(request, timeout=90) as response, archive.open('wb') as stream:
            shutil.copyfileobj(response, stream)
    if digest(archive) != SHA256:
        raise RuntimeError('Pinned libdeflate archive checksum mismatch')
    source = args.work / f'libdeflate-{VERSION}'
    if not source.is_dir():
        with tarfile.open(archive) as stream:
            stream.extractall(args.work, filter='data')
    if digest(source / 'COPYING') != LICENSE_SHA256:
        raise RuntimeError('Pinned libdeflate source notice differs')
    arches = list(dict.fromkeys(args.arch))
    libraries, header, configurations = [], None, {}
    for arch in arches:
        build, install = args.work / (arch + '-build'), args.work / (arch + '-install')
        command = ['cmake', '-S', str(source), '-B', str(build), '-G', 'Ninja',
            '-DCMAKE_BUILD_TYPE=Release', f'-DCMAKE_INSTALL_PREFIX={install}',
            f'-DCMAKE_TOOLCHAIN_FILE={ROOT}/cmake/macos/DependencyToolchain.cmake',
            f'-DHALO2_DEPENDENCY_ARCH={arch}', f'-DCMAKE_OSX_DEPLOYMENT_TARGET={args.deployment_target}',
            '-DCMAKE_INSTALL_NAME_DIR=@rpath', '-DLIBDEFLATE_BUILD_SHARED_LIB=ON',
            '-DLIBDEFLATE_BUILD_STATIC_LIB=OFF', '-DLIBDEFLATE_BUILD_GZIP=OFF',
            '-DLIBDEFLATE_BUILD_TESTS=OFF', '-DLIBDEFLATE_INSTALL=ON']
        if args.sysroot:
            command.append('-DCMAKE_OSX_SYSROOT=' + args.sysroot)
        run(*command)
        run('cmake', '--build', build, '--parallel', args.jobs)
        run('cmake', '--install', build)
        library = (install / 'lib' / LIBRARY).resolve(strict=True)
        run('lipo', library, '-verify_arch', arch)
        actual_header = digest(install / 'include/libdeflate.h')
        if header is not None and header != actual_header:
            raise RuntimeError('libdeflate public headers differ between CPU builds')
        header = actual_header
        loads = dependencies(library, arch)
        if any(not system_library(x) for x in loads):
            raise RuntimeError('libdeflate unexpectedly depends on a non-system library: ' + repr(loads))
        libraries.append(library)
        configurations[arch] = dict(configure=command, header_sha256=header,
            library_sha256=digest(library), dependencies=loads)
    libdir, include = args.prefix / 'lib', args.prefix / 'include'
    libdir.mkdir(parents=True, exist_ok=True)
    include.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source / 'libdeflate.h', include / 'libdeflate.h')
    if digest(include / 'libdeflate.h') != header:
        raise RuntimeError('Installed libdeflate header differs from the pinned source')
    merged = libdir / LIBRARY
    run('lipo', '-create', *libraries, '-output', merged)
    run('install_name_tool', '-id', '@rpath/' + LIBRARY, merged)
    for arch in arches:
        run('lipo', merged, '-verify_arch', arch)
    run('codesign', '--force', '--sign', '-', merged)
    run('codesign', '--verify', '--strict', merged)
    alias = libdir / 'libdeflate.dylib'
    if alias.is_symlink():
        alias.unlink()
    elif alias.exists():
        raise RuntimeError('Refusing to replace a non-link libdeflate SDK alias')
    alias.symlink_to(merged.name)
    licensedir = args.prefix / 'share/licenses'
    licensedir.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source / 'COPYING', licensedir / 'Libdeflate.txt')
    report = dict(schema='H2_MACOS_LIBDEFLATE_SDK_V1', version=VERSION, source_url=URL,
        source_sha256=SHA256, architectures=arches, deployment_target=args.deployment_target,
        configurations=configurations, header_sha256=header, library_sha256=digest(merged),
        license_sha256=LICENSE_SHA256, prefix=str(args.prefix))
    (args.prefix / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    (args.prefix / 'built.stamp').write_text(SHA256 + '\n')


if __name__ == '__main__':
    main()
