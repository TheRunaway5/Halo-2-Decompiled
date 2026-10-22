#!/usr/bin/env python3
"""Build pinned OpenSSL in a caller-owned CMake build tree on macOS.

Validation must use tools/run_tests.py. Its private tree owns downloads,
generated headers, objects, slice archives and the merged SDK. Normal engine
builds keep their dependency SDK within their own build directory.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request

from bundle import command, dependencies, load_paths, system_library

VERSION = '3.5.9'
URL = f'https://github.com/openssl/openssl/releases/download/openssl-{VERSION}/openssl-{VERSION}.tar.gz'
SHA256 = '603f5602e2eef00d77fbd429d34dcd5822bb301757a1bc9cdb24c670f1eb859a'


def run(*args, **kwargs):
    subprocess.run([str(a) for a in args], check=True, **kwargs)


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def install_headers(installs, prefix):
    inventories = {arch: {str(p.relative_to(path / 'include')): p
                         for p in sorted((path / 'include').rglob('*.h'))}
                   for arch, path in installs.items()}
    names = set(next(iter(inventories.values())))
    if not names or any(set(files) != names for files in inventories.values()):
        raise RuntimeError('OpenSSL public header inventories differ across slices')
    hashes = {}
    for name in sorted(names):
        destination = prefix / 'include' / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        slices = {arch: files[name].read_bytes() for arch, files in inventories.items()}
        hashes[name] = {arch: hashlib.sha256(data).hexdigest() for arch, data in slices.items()}
        if len(set(slices.values())) != 1:
            raise RuntimeError('OpenSSL public header differs across the pinned Mac configurations: ' + name)
        destination.write_bytes(next(iter(slices.values())))
    return hashes


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
    archive = args.work / f'openssl-{VERSION}.tar.gz'
    if not archive.is_file() or digest(archive) != SHA256:
        request = urllib.request.Request(URL, headers={'User-Agent': 'Halo2-macOS-dependency-build'})
        with urllib.request.urlopen(request, timeout=90) as response, archive.open('wb') as stream:
            shutil.copyfileobj(response, stream)
    if digest(archive) != SHA256:
        raise RuntimeError('Pinned OpenSSL source checksum mismatch')
    source = args.work / f'openssl-{VERSION}'
    with tempfile.TemporaryDirectory(prefix='extract-', dir=args.work) as directory:
        with tarfile.open(archive) as stream:
            stream.extractall(directory, filter='data')
        if source.exists():
            shutil.rmtree(source)
        (Path(directory) / source.name).rename(source)
    sysroot = args.sysroot or 'macosx'
    if not Path(sysroot).is_dir():
        sysroot = command('xcrun', '--sdk', sysroot, '--show-sdk-path')
    arches = list(dict.fromkeys(args.arch))
    configurations, installs = {}, {}
    libraries = {'crypto': [], 'ssl': []}
    for arch in arches:
        build = args.work / (arch + '-build')
        install = args.work / (arch + '-install')
        # Configure owns these generated directories; source remains pristine.
        for path in (build, install):
            if path.exists():
                shutil.rmtree(path)
        build.mkdir()
        configure = ['perl', str(source / 'Configure'), 'darwin64-' + arch, 'shared',
            'no-apps', 'no-module', '--prefix=' + str(install), '--libdir=lib',
            '--openssldir=' + str(args.prefix / 'ssl'),
            '-mmacosx-version-min=' + args.deployment_target, '-isysroot', sysroot]
        environment = dict(os.environ, CC='/usr/bin/clang', MACOSX_DEPLOYMENT_TARGET=args.deployment_target)
        run(*configure, cwd=build, env=environment)
        # no-module keeps the default/base/legacy providers in libcrypto; no
        # provider code or algorithms are substituted to obtain universal code.
        run('make', '-s', '-j' + str(args.jobs), 'build_sw', cwd=build, env=environment)
        run('make', '-s', 'install_sw', cwd=build, env=environment)
        if list((install / 'lib').rglob('*.so')):
            raise RuntimeError('A dynamic OpenSSL module escaped the built-in provider policy')
        slices = {}
        for name in libraries:
            library = install / f'lib/lib{name}.3.dylib'
            run('lipo', library, '-verify_arch', arch)
            libraries[name].append(library)
            slices[name] = digest(library)
        installs[arch] = install
        configurations[arch] = dict(configure=configure, library_sha256=slices,
            configuration_sha256=digest(build / 'configdata.pm'))
    public_headers = install_headers(installs, args.prefix)
    libdir = args.prefix / 'lib'
    libdir.mkdir(parents=True, exist_ok=True)
    merged_hashes = {}
    for name, slices in libraries.items():
        merged = libdir / f'lib{name}.3.dylib'
        run('lipo', '-create', *slices, '-output', merged)
        run('install_name_tool', '-id', '@rpath/' + merged.name, merged)
        changes = set()
        for arch in arches:
            run('lipo', merged, '-verify_arch', arch)
            for dependency in dependencies(merged, arch):
                if system_library(dependency):
                    continue
                if name != 'ssl' or Path(dependency).name != 'libcrypto.3.dylib':
                    raise RuntimeError('Unexpected OpenSSL runtime dependency: ' + dependency)
                changes.add(dependency)
        for before in changes:
            if before != '@rpath/libcrypto.3.dylib':
                run('install_name_tool', '-change', before, '@rpath/libcrypto.3.dylib', merged)
        if '@loader_path' not in load_paths(merged, arches[0]):
            run('install_name_tool', '-add_rpath', '@loader_path', merged)
        run('codesign', '--force', '--sign', '-', merged)
        run('codesign', '--verify', '--strict', merged)
        merged_hashes[merged.name] = digest(merged)
        alias = libdir / f'lib{name}.dylib'
        if alias.is_symlink():
            alias.unlink()
        elif alias.exists():
            raise RuntimeError('Refusing to replace a non-link OpenSSL SDK alias')
        alias.symlink_to(merged.name)
    licensedir = args.prefix / 'share/licenses'
    licensedir.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source / 'LICENSE.txt', licensedir / 'OpenSSL.txt')
    pkgdir = libdir / 'pkgconfig'
    pkgdir.mkdir(exist_ok=True)
    for name in ('libcrypto', 'libssl', 'openssl'):
        libs = '-lcrypto' if name == 'libcrypto' else ('-lssl' if name == 'libssl' else '')
        requires = 'libcrypto' if name == 'libssl' else ('libssl libcrypto' if name == 'openssl' else '')
        (pkgdir / (name + '.pc')).write_text('prefix=${pcfiledir}/../..\nexec_prefix=${prefix}\n'
            'libdir=${prefix}/lib\nincludedir=${prefix}/include\n\nName: ' + name + '\n'
            'Description: OpenSSL ' + name + '\nVersion: ' + VERSION + '\nRequires: ' + requires + '\n'
            'Libs: ' + ('-L${libdir} ' + libs if libs else '') + '\nCflags: -I${includedir}\n')
    report = dict(schema='H2_MACOS_OPENSSL_SDK_V1', version=VERSION, source_url=URL,
        source_sha256=SHA256, architectures=arches, deployment_target=args.deployment_target,
        sysroot=sysroot, providers='upstream built-in default/base/legacy via no-module',
        configurations=configurations, public_headers=public_headers, public_headers_identical=True,
        libraries=merged_hashes, license_sha256=digest(licensedir / 'OpenSSL.txt'), prefix=str(args.prefix))
    (args.prefix / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    (args.prefix / 'built.stamp').write_text(SHA256 + '\n')


if __name__ == '__main__':
    main()
