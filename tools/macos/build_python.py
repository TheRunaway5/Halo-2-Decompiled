#!/usr/bin/env python3
"""Assemble a pinned universal Python/Tk/Pillow runtime in an owned build tree.

The PSF installer is expanded, never installed or executed as an installer.
Validation uses run_tests.py, so downloads, expanded payloads and the runtime
are removed with its private build. No host Python packages are copied.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import urllib.request
import zipfile

from bundle import command, dependencies, load_paths, macho, system_library

ROOT = Path(__file__).resolve().parents[2]
INPUTS = Path(__file__).with_name('python_inputs.json')


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def download(name, specification, work):
    path = work / name
    if not path.is_file() or digest(path) != specification['sha256']:
        request = urllib.request.Request(specification['url'], headers={'User-Agent': 'Halo2-macOS-dependency-build'})
        with urllib.request.urlopen(request, timeout=90) as response, path.open('wb') as stream:
            shutil.copyfileobj(response, stream)
    if digest(path) != specification['sha256']:
        raise RuntimeError('Pinned Python runtime input checksum mismatch: ' + name)
    return path


def unpack_wheel(path, output):
    if output.exists():
        shutil.rmtree(output)
    output.mkdir()
    with zipfile.ZipFile(path) as archive:
        for info in archive.infolist():
            name = Path(info.filename)
            if name.is_absolute() or '..' in name.parts or (info.external_attr >> 16) & 0o170000 == 0o120000:
                raise RuntimeError('Unexpected wheel member: ' + info.filename)
        archive.extractall(output)


def merge_pillow(wheels, work, destination):
    roots = {}
    for arch, wheel in wheels.items():
        roots[arch] = work / ('pillow-' + arch)
        unpack_wheel(wheel, roots[arch])
    inventories = {arch: {p.relative_to(root): p for p in root.rglob('*') if p.is_file()}
                   for arch, root in roots.items()}
    names = set(next(iter(inventories.values())))
    if any(set(files) != names for files in inventories.values()):
        raise RuntimeError('Pillow wheel file inventories differ across architectures')
    for name in sorted(names):
        target = destination / name
        target.parent.mkdir(parents=True, exist_ok=True)
        sources = {arch: files[name] for arch, files in inventories.items()}
        if all(macho(p) for p in sources.values()):
            for arch, source in sources.items():
                command('lipo', source, '-verify_arch', arch)
            command('lipo', '-create', *sources.values(), '-output', target)
        elif name.name == 'RECORD':
            # RECORD will be regenerated after dylib fixup and signatures.
            continue
        elif name.name == 'WHEEL':
            lines = next(iter(sources.values())).read_text().splitlines()
            target.write_text('\n'.join(line for line in lines if not line.startswith('Tag:')) +
                '\nTag: cp314-cp314-macosx_11_0_universal2\n')
        else:
            if len({digest(p) for p in sources.values()}) != 1:
                raise RuntimeError('Pillow non-code input differs across architectures: ' + str(name))
            shutil.copy2(next(iter(sources.values())), target)


def relocate(runtime, arches):
    images = sorted(p for p in runtime.rglob('*') if macho(p))
    original = '/Library/Frameworks/Python.framework/Versions/3.14/'
    search = sorted({runtime, runtime / 'lib', runtime / 'Frameworks', *(p.parent for p in images)})
    records = {}
    for image in images:
        available = set(command('lipo', '-archs', image).split())
        if not set(arches) <= available:
            raise RuntimeError(f'Python runtime image {image} has {available}; required {arches}')
        changes = {}
        for arch in arches:
            for name in dependencies(image, arch):
                if system_library(name):
                    continue
                if name.startswith(original):
                    target = runtime / name[len(original):]
                elif name.startswith('@loader_path/'):
                    target = image.parent / name[len('@loader_path/'):]
                elif name.startswith('@rpath/'):
                    suffix = name[len('@rpath/'):]
                    candidates = [Path((str(runtime) + '/' + p[len(original):]) if p.startswith(original)
                        else p.replace('@loader_path', str(image.parent))) / suffix
                        for p in load_paths(image, arch)]
                    candidates += [p / suffix for p in search]
                    matches = {p.resolve() for p in candidates if p.is_file()}
                    if len(matches) != 1:
                        raise RuntimeError(f'Ambiguous Python runtime dependency: {name} in {image}: {matches}')
                    target = matches.pop()
                else:
                    raise RuntimeError(f'External Python runtime dependency: {name} in {image}')
                target = target.resolve(strict=True)
                if not target.is_relative_to(runtime):
                    raise RuntimeError('Python runtime dependency escapes its owner: ' + str(target))
                changes[name] = '@loader_path/' + os.path.relpath(target, image.parent)
        ids = command('otool', '-arch', arches[0], '-D', image).splitlines()[1:]
        if ids:
            command('install_name_tool', '-id', '@rpath/' + image.relative_to(runtime).as_posix(), image)
        for before, after in changes.items():
            if before != after:
                command('install_name_tool', '-change', before, after, image)
        records[str(image.relative_to(runtime))] = dict(architectures=sorted(available), changes=changes)
    # Recheck the complete graph after every install-name mutation.
    for image in images:
        for arch in arches:
            for name in dependencies(image, arch):
                if system_library(name):
                    continue
                if not name.startswith('@loader_path/'):
                    raise RuntimeError('Unrelocated Python dependency: ' + name)
                target = (image.parent / name[len('@loader_path/'):]).resolve(strict=True)
                if not target.is_relative_to(runtime):
                    raise RuntimeError('Python dependency is outside the runtime')
        command('codesign', '--force', '--sign', '-', image)
    for app in sorted((p for p in runtime.rglob('*') if p.is_dir() and
                       p.suffix in ('.app', '.framework')), key=lambda p: len(p.parts), reverse=True):
        command('codesign', '--force', '--sign', '-', app)
        command('codesign', '--verify', '--deep', '--strict', app)
    return records


def record_wheel_files(site):
    import base64
    for metadata in site.glob('*.dist-info'):
        record = metadata / 'RECORD'
        # Include private wheel dylibs and licenses as well as Python files.
        top_level = metadata.name.split('-', 1)[0]
        paths = list(metadata.rglob('*'))
        paths += list((site / ('PIL' if top_level == 'pillow' else top_level)).rglob('*'))
        with record.open('w', newline='') as output:
            writer = csv.writer(output)
            for path in sorted(set(paths)):
                if not path.is_file() or path == record:
                    continue
                checksum = base64.urlsafe_b64encode(bytes.fromhex(digest(path))).rstrip(b'=').decode()
                writer.writerow([path.relative_to(site).as_posix(), 'sha256=' + checksum, path.stat().st_size])
            writer.writerow([record.relative_to(site).as_posix(), '', ''])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--arch', action='append', choices=['arm64', 'x86_64'], required=True)
    parser.add_argument('--installer', type=Path, help='Reuse a verified, caller-owned installer input')
    args = parser.parse_args()
    if sys.platform != 'darwin':
        parser.error('Python runtime assembly requires macOS SDK tools')
    specification = json.loads(INPUTS.read_text())
    args.work = args.work.resolve(); args.work.mkdir(parents=True, exist_ok=True)
    output_prefix = args.prefix.resolve()
    if output_prefix.is_relative_to(args.work) or args.work.is_relative_to(output_prefix):
        raise RuntimeError('Python SDK output and build work directories must be separate')
    # Ninja creates parent directories for declared byproducts before this
    # command runs. Assemble separately so a partial SDK is never an input to
    # its own next build, then publish into the caller-owned output directory.
    args.prefix = args.work / 'staged-sdk'
    if args.prefix.exists():
        shutil.rmtree(args.prefix)
    args.prefix.mkdir()
    arches = list(dict.fromkeys(args.arch))
    pkg_name = f'python-{specification["python"]}-macos11.pkg'
    inputs = {}
    for name, row in specification['files'].items():
        if name == pkg_name and args.installer:
            path = args.installer.resolve(strict=True)
            if digest(path) != row['sha256']:
                raise RuntimeError('Caller-owned Python installer checksum mismatch')
        else:
            path = download(name, row, args.work)
        inputs[name] = path
    signature = command('pkgutil', '--check-signature', inputs[pkg_name])
    if 'Developer ID Installer: Python Software Foundation' not in signature:
        raise RuntimeError('The Python installer does not have the expected PSF installer signature')
    expanded = args.work / 'expanded'
    if expanded.exists():
        shutil.rmtree(expanded)
    command('pkgutil', '--expand-full', inputs[pkg_name], expanded)
    source = expanded / 'Python_Framework.pkg/Payload/Versions/3.14'
    runtime = args.prefix / 'python'
    shutil.copytree(source, runtime, symlinks=True, ignore=shutil.ignore_patterns(
        '__pycache__', '*.pyc', '*.a', '*.la', '*Config.sh', 'site-packages', 'test', 'tests',
        'idlelib', 'ensurepip', 'include', 'Headers', 'PrivateHeaders'))
    # Keep the upstream GUI-capable framework launcher and its real Python.app.
    binary = source / 'bin/python3.14'
    shutil.rmtree(runtime / 'bin')
    (runtime / 'bin').mkdir()
    shutil.copy2(binary, runtime / 'bin/python3.real')
    for name in ('share', 'lib/pkgconfig', 'lib/python3.14/config-3.14-darwin'):
        path = runtime / name
        if path.is_dir():
            shutil.rmtree(path)
    site = runtime / 'lib/python3.14/site-packages'
    site.mkdir()
    merge_pillow({arch: next(path for name, path in inputs.items()
        if name.startswith('pillow-') and name.endswith(arch + '.whl')) for arch in arches}, args.work, site)
    certifi = next(path for name, path in inputs.items() if name.startswith('certifi-'))
    certifi_work = args.work / 'certifi'
    unpack_wheel(certifi, certifi_work)
    shutil.copytree(certifi_work, site, dirs_exist_ok=True)
    tcl = [p.parent.relative_to(runtime) for p in runtime.rglob('init.tcl')]
    tk = [p.parent.relative_to(runtime) for p in runtime.rglob('tk.tcl')]
    if len(tcl) != 1 or len(tk) != 1:
        raise RuntimeError(f'Private Tcl/Tk script owners are ambiguous: {tcl}, {tk}')
    command('xcrun', 'clang++', '-std=c++20', *[value for arch in arches for value in ('-arch', arch)],
        '-mmacosx-version-min=13.0', '-Wl,-headerpad_max_install_names', '-I' + str(ROOT / 'include'),
        '-DHALO2_TCL_LIBRARY="' + str(tcl[0]) + '"', '-DHALO2_TK_LIBRARY="' + str(tk[0]) + '"',
        ROOT / 'src/macos/python_launcher.cpp', '-o', runtime / 'bin/python3')
    for path in runtime.rglob('*'):
        if path.is_symlink():
            try:
                target = path.resolve(strict=True)
            except OSError as error:
                raise RuntimeError('Python runtime contains a dangling link: ' +
                    str(path.relative_to(runtime)) + ' -> ' + os.readlink(path)) from error
            if not target.is_relative_to(runtime):
                raise RuntimeError('Python runtime contains an external symlink: ' + str(path))
    images = relocate(runtime, arches)
    record_wheel_files(site)
    licenses = args.prefix / 'licenses'
    licenses.mkdir()
    for path in sorted((expanded / 'Resources').rglob('*')):
        if path.is_file() and 'license' in path.name.lower():
            destination = licenses / path.relative_to(expanded / 'Resources')
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, destination)
    if not any(licenses.rglob('*')):
        raise RuntimeError('The Python installer license notices are unavailable')
    report = dict(schema='H2_MACOS_PYTHON_RUNTIME_V1', python=specification['python'],
        pillow=specification['pillow'], certifi=specification['certifi'], architectures=arches,
        inputs=specification['files'], installer_signature=signature, prefix=str(output_prefix), images=images,
        tcl_library=str(tcl[0]), tk_library=str(tk[0]),
        files={str(p.relative_to(args.prefix)): dict(bytes=p.stat().st_size, sha256=digest(p))
               for p in sorted(args.prefix.rglob('*')) if p.is_file() and not p.is_symlink()})
    (args.prefix / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    (args.prefix / 'built.stamp').write_text(digest(INPUTS) + '\n')
    if output_prefix.exists():
        shutil.rmtree(output_prefix)
    args.prefix.rename(output_prefix)


if __name__ == '__main__':
    main()
