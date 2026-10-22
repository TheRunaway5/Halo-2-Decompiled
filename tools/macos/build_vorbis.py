#!/usr/bin/env python3
"""Build checksum-pinned Ogg/Vorbis in a caller-owned CMake build tree.

Validation uses tools/run_tests.py; downloads, sources, both slice builds and
the merged SDK disappear with its private tree. Normal builds own their SDK.
"""
from __future__ import annotations

import argparse
import hashlib
import http.client
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
import urllib.error

ROOT = Path(__file__).resolve().parents[2]
SOURCES = {
    'ogg': ('1.3.6', '5c8253428e181840cd20d41f3ca16557a9cc04bad4a3d04cce84808677fa1061'),
    'vorbis': ('1.3.7', 'b33cc4934322bcbf6efcbacf49e3ca01aadbea4114ec9589d1b1e9d20f72954b'),
}
LIBRARIES = {'ogg': 'libogg.0.dylib', 'vorbis': 'libvorbis.0.dylib',
             'vorbisenc': 'libvorbisenc.2.dylib', 'vorbisfile': 'libvorbisfile.3.dylib'}


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
    arches = list(dict.fromkeys(args.arch))
    source_paths, provenance = {}, {}
    for name, (version, expected) in SOURCES.items():
        basename = f'lib{name}-{version}'
        url = f'https://downloads.xiph.org/releases/{name}/{basename}.tar.xz'
        archive = args.work / (basename + '.tar.xz')
        if not archive.is_file() or digest(archive) != expected:
            mirror = f'https://ftp.osuosl.org/pub/xiph/releases/{name}/{basename}.tar.xz'
            for download in (url, mirror):
                try:
                    request = urllib.request.Request(download, headers={'User-Agent': 'Halo2-macOS-dependency-build'})
                    with urllib.request.urlopen(request, timeout=45) as response, archive.open('wb') as stream:
                        shutil.copyfileobj(response, stream)
                    break
                except (urllib.error.URLError, http.client.HTTPException, TimeoutError, ConnectionError):
                    archive.unlink(missing_ok=True)
                    if download == mirror:
                        raise
                    print('Xiph download unavailable; trying its checksum-pinned mirror', flush=True)
        if digest(archive) != expected:
            raise RuntimeError(f'Pinned {name} source checksum mismatch')
        # Only publish a complete extraction. Reruns start from pristine source,
        # including after cancellation of an earlier normal development build.
        source = args.work / basename
        with tempfile.TemporaryDirectory(prefix='extract-', dir=args.work) as directory:
            with tarfile.open(archive) as stream:
                stream.extractall(directory, filter='data')
            if source.exists():
                shutil.rmtree(source)
            (Path(directory) / basename).rename(source)
        source_paths[name] = source
        provenance[name] = dict(version=version, url=url, sha256=expected)

    inputs = {name: [] for name in LIBRARIES}
    configurations, headers = {}, None
    for arch in arches:
        install = args.work / (arch + '-install')
        commands = []
        compiler_flags = {}
        for name in SOURCES:
            build = args.work / (arch + '-' + name + '-build')
            command = ['cmake', '-S', str(source_paths[name]), '-B', str(build), '-G', 'Ninja',
                '-DCMAKE_BUILD_TYPE=Release', f'-DCMAKE_INSTALL_PREFIX={install}',
                f'-DCMAKE_TOOLCHAIN_FILE={ROOT}/cmake/macos/DependencyToolchain.cmake',
                f'-DHALO2_DEPENDENCY_ARCH={arch}', f'-DCMAKE_OSX_DEPLOYMENT_TARGET={args.deployment_target}',
                '-DCMAKE_POLICY_VERSION_MINIMUM=3.5', '-DCMAKE_INSTALL_NAME_DIR=@rpath',
                '-DBUILD_SHARED_LIBS=ON', '-DBUILD_FRAMEWORK=OFF', '-DBUILD_TESTING=OFF',
                '-DINSTALL_DOCS=OFF']
            if args.sysroot:
                command.append('-DCMAKE_OSX_SYSROOT=' + args.sysroot)
            if name == 'vorbis':
                command += [f'-DOGG_INCLUDE_DIR={install}/include',
                            f'-DOGG_LIBRARY={install}/lib/libogg.dylib',
                            # Retain the source expression boundaries on both
                            # CPUs. ARM contraction otherwise changes millions
                            # of samples. This alone does not match the retained
                            # Linux FMA-enabled loading-track reference.
                            '-DCMAKE_C_FLAGS=-ffp-contract=off']
            run(*command)
            run('cmake', '--build', build, '--parallel', args.jobs)
            run('cmake', '--install', build)
            commands.append(command)
            compiler_flags[name] = [line for line in (build / 'CMakeCache.txt').read_text().splitlines()
                                    if line.startswith(('CMAKE_C_FLAGS:', 'CMAKE_C_FLAGS_RELEASE:'))]
        slices = {}
        for name in LIBRARIES:
            library = (install / f'lib/lib{name}.dylib').resolve(strict=True)
            run('lipo', library, '-verify_arch', arch)
            inputs[name].append(library)
            slices[name] = dict(path=str(library), sha256=digest(library))
        actual_headers = {str(p.relative_to(install / 'include')): digest(p)
                          for p in sorted((install / 'include').rglob('*.h'))}
        if headers is not None and headers != actual_headers:
            raise RuntimeError('Ogg/Vorbis public headers differ across architectures')
        headers = actual_headers
        configurations[arch] = dict(configure=commands, compiler_flags=compiler_flags, libraries=slices)

    libdir = args.prefix / 'lib'
    libdir.mkdir(parents=True, exist_ok=True)
    merged_hashes = {}
    for name, filename in LIBRARIES.items():
        merged = libdir / filename
        run('lipo', '-create', *inputs[name], '-output', merged)
        run('install_name_tool', '-id', '@rpath/' + filename, merged)
        changes = {}
        for arch in arches:
            run('lipo', merged, '-verify_arch', arch)
            lines = subprocess.check_output(['otool', '-arch', arch, '-L', str(merged)], text=True).splitlines()[2:]
            for line in lines:
                dependency = line.strip().split(' (compatibility', 1)[0]
                if dependency.startswith(('/usr/lib/', '/System/Library/')):
                    continue
                candidates = [key for key in LIBRARIES if Path(dependency).name.startswith('lib' + key + '.')]
                if len(candidates) != 1:
                    raise RuntimeError(f'Unexpected Ogg/Vorbis runtime dependency: {dependency}')
                changes[dependency] = '@rpath/' + LIBRARIES[candidates[0]]
        for before, after in changes.items():
            if before != after:
                run('install_name_tool', '-change', before, after, merged)
        run('install_name_tool', '-add_rpath', '@loader_path', merged)
        run('codesign', '--force', '--sign', '-', merged)
        run('codesign', '--verify', '--strict', merged)
        merged_hashes[filename] = digest(merged)
        alias = libdir / f'lib{name}.dylib'
        if alias.is_symlink():
            alias.unlink()
        elif alias.exists():
            raise RuntimeError('Refusing to replace a non-link SDK alias')
        alias.symlink_to(filename)
    shutil.copytree(args.work / (arches[0] + '-install/include'), args.prefix / 'include', dirs_exist_ok=True)
    licensedir = args.prefix / 'share/licenses'
    licensedir.mkdir(parents=True, exist_ok=True)
    for name in SOURCES:
        shutil.copyfile(source_paths[name] / 'COPYING', licensedir / (name.capitalize() + '.txt'))
    pkgdir = libdir / 'pkgconfig'
    pkgdir.mkdir(exist_ok=True)
    for name in LIBRARIES:
        owner = 'ogg' if name == 'ogg' else 'vorbis'
        requirement = '' if name == 'ogg' else ('ogg' if name == 'vorbis' else 'vorbis')
        (pkgdir / (name + '.pc')).write_text('prefix=${pcfiledir}/../..\nexec_prefix=${prefix}\n'
            'libdir=${prefix}/lib\nincludedir=${prefix}/include\n\nName: ' + name + '\n'
            'Description: Xiph ' + name + '\nVersion: ' + SOURCES[owner][0] + '\n'
            'Requires: ' + requirement + '\nLibs: -L${libdir} -l' + name + '\nCflags: -I${includedir}\n')
    report = dict(schema='H2_MACOS_VORBIS_SDK_V1', sources=provenance, architectures=arches,
        deployment_target=args.deployment_target, configurations=configurations, public_headers=headers,
        libraries=merged_hashes, licenses={p.name: digest(p) for p in sorted(licensedir.iterdir())}, prefix=str(args.prefix))
    (args.prefix / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    (args.prefix / 'built.stamp').write_text(json.dumps(provenance, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
