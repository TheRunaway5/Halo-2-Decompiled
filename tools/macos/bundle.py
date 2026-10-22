#!/usr/bin/env python3
"""Close a development .app's dylib graph, verify every slice, then ad-hoc sign.

Only the supplied build bundle is modified. SDK/Homebrew inputs stay untouched.
This does not notarize or publish a release.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys

MACHO_MAGIC = {bytes.fromhex(value) for value in ('cafebabe', 'bebafeca', 'cafebabf',
    'bfbafeca', 'feedface', 'cefaedfe', 'feedfacf', 'cffaedfe')}


def macho(path):
    if not path.is_file() or path.is_symlink():
        return False
    with path.open('rb') as stream:
        return stream.read(4) in MACHO_MAGIC


def command(*args):
    return subprocess.check_output([str(a) for a in args], text=True).strip()


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def system_library(name):
    return name.startswith(('/System/Library/', '/usr/lib/'))


def load_paths(image, arch):
    lines = command('otool', '-arch', arch, '-l', image).splitlines()
    result = []
    for index, line in enumerate(lines):
        if line.strip() == 'cmd LC_RPATH':
            result.append(lines[index + 2].strip().split('path ', 1)[1].split(' (offset', 1)[0])
    return result


def dependencies(image, arch):
    lines = command('otool', '-arch', arch, '-L', image).splitlines()[1:]
    # dylibs list their own install-name first. Executables have no LC_ID_DYLIB.
    ids = command('otool', '-arch', arch, '-D', image).splitlines()[1:]
    return [line.strip().split(' (compatibility', 1)[0] for line in lines
            if line.startswith('\t') and line.strip().split(' (compatibility', 1)[0] not in ids]


def expand(name, image, executable):
    return Path(name.replace('@loader_path', str(image.parent))
                .replace('@executable_path', str(executable.parent)))


def resolve(name, image, executable, search, arch):
    if name.startswith('@rpath/'):
        suffix = name[len('@rpath/'):]
        candidates = [expand(p, image, executable) / suffix for p in load_paths(image, arch)]
        candidates += [expand(p, executable, executable) / suffix for p in load_paths(executable, arch)]
        candidates += [p / suffix for p in search]
    else:
        candidates = [expand(name, image, executable)]
    for path in candidates:
        if path.is_file():
            return path.resolve()
    raise RuntimeError(f'Cannot resolve {name} required by {image} ({arch})')


def bundle(app, arches, libraries, licenses, *, resource_sealers=()):
    app = app.resolve(strict=True)
    if app.suffix != '.app':
        raise RuntimeError('Expected a CMake .app build output')
    contents = app / 'Contents'
    info = plistlib.loads((contents / 'Info.plist').read_bytes())
    executable = contents / 'MacOS' / info['CFBundleExecutable']
    frameworks = contents / 'Frameworks'
    frameworks.mkdir(exist_ok=True)
    search = [p.resolve().parent for p in libraries] + [frameworks]
    queue = []
    copied = {}
    private_images = set()
    private_runtime = {}
    steam = contents / 'Resources/steam'
    if steam.exists():
        from verify_steam_sdk import verify
        audited = verify(steam, sign=True)
        if set(audited['architectures']) != set(arches):
            raise RuntimeError('Steam helper CPU owners do not match the app architectures')
        private_images = {steam / name for name in audited['images']}
        private_runtime['steam'] = dict(architectures=audited['architectures'],
            images={str((steam / name).relative_to(contents)): row['architecture']
                    for name, row in audited['images'].items()}, manifest_sha256=digest(steam / 'manifest.json'))

    def copy_library(source):
        source = source.resolve(strict=True)
        if source.is_relative_to(contents):
            # Embedded runtimes retain their private library layout. Flattening
            # Python's OpenSSL/Pillow libraries would collide with the engine's
            # independently owned libraries and break loader-relative imports.
            queue.append((source, source))
            return source
        name = source.name
        if name in copied:
            if source not in copied[name] and digest(copied[name][0]) != digest(source):
                raise RuntimeError(f'Conflicting bundled library basename: {name}')
            return copied[name][1]
        destination = frameworks / name
        if source != destination:
            shutil.copy2(source, destination)
        destination.chmod(destination.stat().st_mode | 0o200)
        copied[name] = (source, destination)
        queue.append((source, destination))
        return destination

    for library in libraries:
        copy_library(library)
    queue.extend((image, image) for image in sorted(contents.rglob('*')) if macho(image) and image not in private_images)
    if not macho(executable):
        raise RuntimeError('The application entry point is not a Mach-O image')
    seen = set()
    while queue:
        source, destination = queue.pop(0)
        if destination in seen:
            continue
        seen.add(destination)
        actual_arches = set(command('lipo', '-archs', destination).split())
        if not set(arches) <= actual_arches:
            raise RuntimeError(f'{destination} has {sorted(actual_arches)}; required {arches}')
        changes = {}
        for arch in arches:
            for name in dependencies(source, arch):
                if system_library(name):
                    continue
                dependency = resolve(name, source, executable, search, arch)
                target = copy_library(dependency)
                changes[name] = '@loader_path/' + os.path.relpath(target, destination.parent)
        if command('otool', '-arch', arches[0], '-D', destination).splitlines()[1:]:
            command('install_name_tool', '-id', '@rpath/' + destination.name, destination)
        for before, after in changes.items():
            if before != after:
                command('install_name_tool', '-change', before, after, destination)

    license_dir = contents / 'Resources/licenses'
    license_dir.mkdir(parents=True, exist_ok=True)
    for label, source in licenses:
        # Versioned notices such as LGPL-2.1 remain plain file basenames.
        if not label.replace('-', '').replace('.', '').isalnum():
            raise RuntimeError('Invalid dependency license label')
        shutil.copyfile(source, license_dir / (label + '.txt'))
    # Verify closure after all rewrites, before signatures make the bundle final.
    for image in seen:
        for arch in arches:
            for name in dependencies(image, arch):
                if not system_library(name):
                    resolved = resolve(name, image, executable, search, arch)
                    if not resolved.is_relative_to(contents):
                        raise RuntimeError(f'External runtime dependency remains: {name}')
    for image in sorted(seen - {executable}):
        command('codesign', '--force', '--sign', '-', image)
    if private_images:
        # These images were separately signed and verified without rewriting
        # CLR loader paths. Their complete graph stays within its CPU owner.
        verify(steam)
        seen.update(private_images)
    for child in sorted((p for p in contents.rglob('*') if p.is_dir() and
                         p.suffix in ('.app', '.framework')), key=lambda p: len(p.parts), reverse=True):
        command('codesign', '--force', '--sign', '-', child)
    command('codesign', '--force', '--sign', '-', executable)
    # Resource manifests must pin final rewritten/signed helper bytes. They
    # are written before the enclosing app seals its Resources directory.
    for seal in resource_sealers:
        seal()
    record = {'schema': 'H2_MACOS_BUNDLE_V1', 'architectures': arches,
              'signing': 'ad-hoc development; not notarized',
              'private_runtimes': private_runtime,
              'files': {str(p.relative_to(contents)): {'bytes': p.stat().st_size, 'sha256': digest(p)}
                        for p in sorted(seen - {executable})}}
    (contents / 'Resources/bundle-manifest.json').write_text(json.dumps(record, indent=2) + '\n')
    command('codesign', '--force', '--sign', '-', app)
    command('codesign', '--verify', '--deep', '--strict', '--verbose=4', app)
    # The enclosing signature changes the executable after sealing Resources.
    # Keep its final hash outside the bundle to avoid a signature/hash cycle.
    record['files'][str(executable.relative_to(contents))] = {
        'bytes': executable.stat().st_size, 'sha256': digest(executable)}
    app.with_suffix('.bundle.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps({'app': str(app), 'architectures': arches, 'bundled_libraries': len(copied)}))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, required=True)
    parser.add_argument('--arch', action='append', choices=['arm64', 'x86_64'])
    parser.add_argument('--library', type=Path, action='append', default=[])
    parser.add_argument('--license', action='append', default=[], metavar='NAME=PATH')
    args = parser.parse_args()
    if sys.platform != 'darwin':
        parser.error('Bundle fixup requires macOS lipo, otool, install_name_tool and codesign')
    bundle(args.app, args.arch or ['arm64', 'x86_64'], args.library,
           [(v.split('=', 1)[0], Path(v.split('=', 1)[1])) for v in args.license])


if __name__ == '__main__':
    main()
