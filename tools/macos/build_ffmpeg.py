#!/usr/bin/env python3
"""Build pinned playback libraries and their AV1 dependency in an owned tree.

CMake owns --work/--prefix. Validation runs through tools/run_tests.py, which
removes downloads, native build tools, slice builds and SDKs after every run.
No Homebrew codec library is admitted into this dependency graph.
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
import zipfile

SOURCES = {
    'ffmpeg': ('9.0.2', 'https://ffmpeg.org/releases/ffmpeg-9.0.2.tar.xz',
               '8c3850283eb25fa026482078a04051e0be17347b09ef81a0849bec15a96e002e'),
    'dav1d': ('1.5.4', 'https://codeload.github.com/videolan/dav1d/tar.gz/refs/tags/1.5.4',
              'a1d5b63d2d38ec9bd03acf643caa51fa22edd1e89c5a109c4807717216bbec07'),
    'nasm': ('3.02', 'https://www.nasm.us/pub/nasm/releasebuilds/3.02/nasm-3.02.tar.xz',
             '87336eba53b4acfe917424ab5d500d2b0054d9f5148d35c2273ccf2cfb712f0d'),
    'meson': ('1.10.0', 'https://files.pythonhosted.org/packages/32/4f/c398c6f06ece1c6c246e008d5dac3824c98f54d3eb3d8014f4910afd6d48/meson-1.10.0-py3-none-any.whl',
              '4b27aafce281e652dcb437b28007457411245d975c48b5db3a797d3e93ae1585'),
}
LIBRARIES = {'avcodec': 63, 'avformat': 63, 'avutil': 61,
             'swresample': 7, 'swscale': 10, 'dav1d': 7}


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def run(*command, cwd=None, env=None):
    subprocess.run([str(arg) for arg in command], cwd=cwd, env=env, check=True)


def extract(work, name):
    version, url, expected = SOURCES[name]
    archive = work / (name + '-source' + ('.whl' if name == 'meson' else '.tar'))
    print('Preparing checksum-pinned ' + name + ' ' + version, flush=True)
    if not archive.is_file() or digest(archive) != expected:
        request = urllib.request.Request(url, headers={'User-Agent': 'Halo2-macOS-dependency-build'})
        with urllib.request.urlopen(request, timeout=45) as response, archive.open('wb') as stream:
            shutil.copyfileobj(response, stream)
    if digest(archive) != expected:
        raise RuntimeError('Pinned source checksum mismatch: ' + name)
    target = work / (name + '-' + version)
    with tempfile.TemporaryDirectory(prefix='extract-', dir=work) as directory:
        temporary = Path(directory)
        if name == 'meson':
            with zipfile.ZipFile(archive) as source:
                source.extractall(temporary)
            extracted = temporary
        else:
            with tarfile.open(archive) as source:
                source.extractall(temporary, filter='data')
            extracted = temporary / (name + '-' + version)
        if target.exists():
            shutil.rmtree(target)
        if name == 'meson':
            shutil.copytree(extracted, target)
        else:
            extracted.rename(target)
    return target


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
    args.prefix.mkdir(parents=True, exist_ok=True)
    (args.prefix / 'built.stamp').unlink(missing_ok=True)
    arches = list(dict.fromkeys(args.arch))
    source = {name: extract(args.work, name) for name in SOURCES}
    sysroot = args.sysroot or subprocess.check_output(['xcrun', '--sdk', 'macosx', '--show-sdk-path'], text=True).strip()
    compiler = subprocess.check_output(['xcrun', '--find', 'clang'], text=True).strip()
    # Build tools are native host programs, not shipped runtime dependencies.
    # NASM still generates x86_64 code when the build machine is Apple Silicon.
    tools = args.work / 'host-tools'
    nasm_command = [source['nasm'] / 'configure', '--prefix=' + str(tools)]
    environment = {k: v for k, v in os.environ.items()
                   if k not in ('CFLAGS', 'CPPFLAGS', 'LDFLAGS', 'CC', 'CXX', 'PKG_CONFIG_PATH', 'PKG_CONFIG_LIBDIR')}
    environment['PYTHONDONTWRITEBYTECODE'] = '1'
    run(*nasm_command, cwd=source['nasm'], env=environment)
    run('make', '-j' + str(args.jobs), 'nasm', cwd=source['nasm'], env=environment)
    (tools / 'bin').mkdir(parents=True, exist_ok=True)
    shutil.copy2(source['nasm'] / 'nasm', tools / 'bin/nasm')
    meson = source['meson'] / 'meson.py'
    meson.write_text('import sys\nfrom pathlib import Path\n'
        'sys.path.insert(0, str(Path(__file__).resolve().parent))\n'
        'from mesonbuild.mesonmain import main\nsys.exit(main())\n')
    environment['PATH'] = str(tools / 'bin') + os.pathsep + environment['PATH']
    inputs = {name: [] for name in LIBRARIES}
    configurations = {}
    headers = None
    for arch in arches:
        install = args.work / (arch + '-install')
        davbuild = args.work / (arch + '-dav1d-build')
        ffbuild = args.work / (arch + '-ffmpeg-build')
        for directory in (install, davbuild, ffbuild):
            if directory.exists():
                shutil.rmtree(directory)
        flags = ['-arch', arch, '-isysroot', sysroot, '-mmacosx-version-min=' + args.deployment_target]
        link_flags = flags + ['-Wl,-headerpad_max_install_names']
        cross = args.work / (arch + '-meson.ini')
        cross.write_text('[binaries]\nc = ' + repr(compiler) + '\n'
            'ar = \'/usr/bin/ar\'\nstrip = \'/usr/bin/strip\'\npkg-config = \'pkg-config\'\n'
            '[host_machine]\nsystem = \'darwin\'\ncpu_family = ' + repr('aarch64' if arch == 'arm64' else 'x86_64') +
            '\ncpu = ' + repr('aarch64' if arch == 'arm64' else 'x86_64') + '\nendian = \'little\'\n'
            '[built-in options]\nc_args = ' + repr(flags) + '\nc_link_args = ' + repr(link_flags) + '\n')
        davcommand = [sys.executable, '-B', meson, 'setup', davbuild, source['dav1d'],
            '--cross-file', cross, '--prefix', install, '--libdir', 'lib',
            '--buildtype', 'release', '--default-library', 'shared',
            '-Denable_tools=false', '-Denable_tests=false', '-Denable_examples=false']
        run(*davcommand, env=environment)
        run('ninja', '-C', davbuild, '-j', args.jobs, env=environment)
        run(sys.executable, '-B', meson, 'install', '-C', davbuild, env=environment)
        ffbuild.mkdir()
        slice_env = dict(environment, PKG_CONFIG_LIBDIR=str(install / 'lib/pkgconfig'),
                         PKG_CONFIG_PATH='', MACOSX_DEPLOYMENT_TARGET=args.deployment_target)
        # Keep all native decoders/demuxers/parsers and the engine's hardware
        # paths. External autodetection is disabled so thin host libraries
        # cannot silently satisfy an optional dependency. AV1 software decoding
        # explicitly uses the matching owned dav1d slice.
        ffcommand = [source['ffmpeg'] / 'configure', '--prefix=' + str(install),
            '--target-os=darwin', '--arch=' + ('aarch64' if arch == 'arm64' else 'x86_64'),
            '--enable-cross-compile', '--cc=' + compiler, '--sysroot=' + sysroot,
            '--extra-cflags=' + ' '.join(flags), '--extra-ldflags=' + ' '.join(link_flags),
            '--enable-shared', '--disable-static', '--disable-programs', '--disable-doc',
            '--disable-debug', '--disable-autodetect', '--disable-encoders', '--disable-muxers',
            '--disable-avdevice', '--disable-avfilter', '--enable-libdav1d',
            '--enable-videotoolbox', '--enable-audiotoolbox', '--enable-zlib',
            '--install-name-dir=@rpath']
        run(*ffcommand, cwd=ffbuild, env=slice_env)
        run('make', '-j' + str(args.jobs), cwd=ffbuild, env=slice_env)
        run('make', 'install', cwd=ffbuild, env=slice_env)
        config = (ffbuild / 'config.h').read_text()
        required = ['CONFIG_LIBDAV1D', 'CONFIG_VIDEOTOOLBOX', 'CONFIG_AUDIOTOOLBOX', 'CONFIG_ZLIB']
        for feature in required:
            if '#define ' + feature + ' 1' not in config:
                raise RuntimeError('Required playback feature is absent: ' + feature)
        slices = {}
        for name in LIBRARIES:
            library = (install / ('lib/lib' + name + '.dylib')).resolve(strict=True)
            run('lipo', library, '-verify_arch', arch)
            inputs[name].append(library)
            slices[name] = dict(path=str(library), sha256=digest(library))
        actual_headers = {str(path.relative_to(install / 'include')): digest(path)
                          for path in sorted((install / 'include').rglob('*.h'))}
        if headers is not None and headers != actual_headers:
            raise RuntimeError('FFmpeg/dav1d public headers differ across architectures')
        headers = actual_headers
        configurations[arch] = dict(configure=[[str(x) for x in davcommand], [str(x) for x in ffcommand]],
            config_sha256=digest(ffbuild / 'config.h'), libraries=slices,
            enabled_defines=[line for line in config.splitlines() if line.endswith(' 1')])
    libdir = args.prefix / 'lib'
    libdir.mkdir(parents=True, exist_ok=True)
    names = {name: f'lib{name}.{major}.dylib' for name, major in LIBRARIES.items()}
    hashes = {}
    for name, filename in names.items():
        library = libdir / filename
        run('lipo', '-create', *inputs[name], '-output', library)
        run('install_name_tool', '-id', '@rpath/' + filename, library)
        changes = {}
        for arch in arches:
            run('lipo', library, '-verify_arch', arch)
            lines = subprocess.check_output(['otool', '-arch', arch, '-L', str(library)], text=True).splitlines()[2:]
            for line in lines:
                dependency = line.strip().split(' (compatibility', 1)[0]
                if dependency.startswith(('/usr/lib/', '/System/Library/')):
                    continue
                candidates = [key for key in names if Path(dependency).name.startswith('lib' + key + '.')]
                if len(candidates) != 1:
                    raise RuntimeError('Unexpected playback runtime dependency: ' + dependency)
                changes[dependency] = '@rpath/' + names[candidates[0]]
        for before, after in changes.items():
            if before != after:
                run('install_name_tool', '-change', before, after, library)
        run('install_name_tool', '-add_rpath', '@loader_path', library)
        run('codesign', '--force', '--sign', '-', library)
        run('codesign', '--verify', '--strict', library)
        hashes[filename] = digest(library)
        alias = libdir / ('lib' + name + '.dylib')
        if alias.is_symlink():
            alias.unlink()
        elif alias.exists():
            raise RuntimeError('Refusing to replace a non-link SDK alias')
        alias.symlink_to(filename)
    first = args.work / (arches[0] + '-install')
    shutil.copytree(first / 'include', args.prefix / 'include', dirs_exist_ok=True)
    pkgdir = libdir / 'pkgconfig'
    pkgdir.mkdir(exist_ok=True)
    for path in (first / 'lib/pkgconfig').glob('*.pc'):
        text = path.read_text().replace('prefix=' + str(first), 'prefix=${pcfiledir}/../..')
        (pkgdir / path.name).write_text(text)
    licensedir = args.prefix / 'share/licenses'
    licensedir.mkdir(parents=True, exist_ok=True)
    for filename in ('COPYING.LGPLv2.1', 'COPYING.LGPLv3', 'LICENSE.md'):
        shutil.copyfile(source['ffmpeg'] / filename, licensedir / ('FFmpeg-' + filename))
    shutil.copyfile(source['dav1d'] / 'COPYING', licensedir / 'Dav1d.txt')
    manifest = dict(schema='H2_MACOS_FFMPEG_SDK_V1', prefix=str(args.prefix),
        architectures=arches, deployment_target=args.deployment_target,
        sources={name: dict(version=v, url=u, sha256=h) for name, (v, u, h) in SOURCES.items()},
        configurations=configurations, libraries=hashes, public_headers=headers,
        licenses={p.name: digest(p) for p in sorted(licensedir.iterdir())})
    (args.prefix / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    (args.prefix / 'built.stamp').write_text(json.dumps(SOURCES, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
