#!/usr/bin/env python3
"""Audit the two explicitly scoped Steam helper runtimes on macOS.

Managed runtime files are architecture specific; each private tree must close
within its own CPU owner. This does not label these helpers universal Mach-O
images or weaken the engine/launcher bundle's universal-image requirement.
"""
import argparse
import json
from pathlib import Path
import sys

from bundle import command, dependencies, digest, macho, resolve, system_library


def verify(sdk, sign=False):
    sdk = sdk.resolve(strict=True)
    metadata = json.loads((sdk / 'manifest.json').read_text())
    if metadata.get('schema') != 'H2_MACOS_STEAM_SDK_V1':
        raise RuntimeError('Unrecognized Steam helper SDK')
    actual_files = {p.relative_to(sdk).as_posix() for p in sdk.rglob('*') if p.is_file()}
    if actual_files != set(metadata['files']) | {'manifest.json', 'built.stamp'}:
        raise RuntimeError('Steam SDK contains unlisted or missing files')
    for name, row in metadata['files'].items():
        path = (sdk / name).resolve(strict=True)
        if not path.is_relative_to(sdk) or path.stat().st_size != row['bytes'] or digest(path) != row['sha256']:
            raise RuntimeError('Steam SDK input changed: ' + name)
    images = {}
    for path in sorted(sdk.rglob('*')):
        if path.is_symlink():
            raise RuntimeError('Unexpected Steam SDK symlink')
        if not macho(path):
            continue
        relative = path.relative_to(sdk)
        arch = relative.parts[0]
        if arch not in metadata['architectures']:
            raise RuntimeError('Native Steam SDK image has no declared CPU owner: ' + str(relative))
        owner = sdk / arch
        available = command('lipo', '-archs', path).split()
        if set(available) != {arch}:
            raise RuntimeError('Steam SDK CPU owner mismatch: ' + str(relative))
        edges = {}
        for name in dependencies(path, arch):
            if system_library(name):
                continue
            target = resolve(name, path, owner / 'Halo2Steam', [owner], arch)
            if not target.is_relative_to(owner):
                raise RuntimeError('Steam runtime dependency escaped its CPU owner: ' + name)
            command('lipo', target, '-verify_arch', arch)
            edges[name] = str(target.relative_to(sdk))
        if sign:
            command('codesign', '--force', '--sign', '-', path)
        command('codesign', '--verify', '--strict', path)
        load = command('otool', '-arch', arch, '-l', path).splitlines()
        minimum = [line.strip() for line in load if line.strip().startswith('minos ')]
        images[str(relative)] = dict(architecture=arch, available_architectures=available,
            dependencies=edges, minimum_system=minimum, sha256=digest(path))
    for arch in metadata['architectures']:
        if str(Path(arch) / 'Halo2Steam') not in images:
            raise RuntimeError('Steam SDK has no native entry point for ' + arch)
    if sign:
        metadata['unsigned_manifest_sha256'] = digest(sdk / 'manifest.json')
        metadata.update(signed=True, images=images)
        for name, row in metadata['files'].items():
            path = sdk / name
            row.update(bytes=path.stat().st_size, sha256=digest(path))
        (sdk / 'manifest.json').write_text(json.dumps(metadata, indent=2) + '\n')
    return dict(schema='H2_MACOS_STEAM_SDK_AUDIT_V1', images=images,
        architectures=metadata['architectures'], signed=metadata['signed'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--sign', action='store_true')
    args = parser.parse_args()
    if sys.platform != 'darwin':
        parser.error('Steam SDK native auditing requires macOS tools')
    args.report.write_text(json.dumps(verify(args.sdk, args.sign), indent=2) + '\n')


if __name__ == '__main__':
    main()
