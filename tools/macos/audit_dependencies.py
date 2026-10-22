#!/usr/bin/env python3
"""Read-only architecture/closure audit of macOS dependency libraries.

Defaults inspect host pkg-config packages, not a configured engine bundle.
Supply --library NAME=PATH to audit a staged SDK instead. Run through
tools/run_test.py; only the explicitly requested small JSON report survives.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

from bundle import command, dependencies, digest, resolve, system_library

PACKAGES = {
    'openssl': ['libcrypto.dylib', 'libssl.dylib'],
    'opus': ['libopus.dylib'],
    **{name: [name + '.dylib'] for name in
       ('libavcodec', 'libavformat', 'libavutil', 'libswresample', 'libswscale')},
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--package', action='append', choices=list(PACKAGES))
    parser.add_argument('--library', action='append', default=[], metavar='NAME=PATH')
    parser.add_argument('--arch', action='append', choices=['arm64', 'x86_64'])
    args = parser.parse_args()
    if sys.platform != 'darwin':
        parser.error('This audit requires macOS lipo and otool')
    arches = args.arch or ['arm64', 'x86_64']
    roots, packages = {}, {}
    errors = []
    selected = args.package if args.package is not None else ([] if args.library else list(PACKAGES))
    for package in selected:
        try:
            directory = Path(command('pkg-config', '--variable=libdir', package))
            packages[package] = dict(version=command('pkg-config', '--modversion', package), libdir=str(directory))
            for name in PACKAGES[package]:
                roots[package + ':' + name] = (directory / name).resolve(strict=True)
        except Exception as error:
            errors.append(dict(package=package, error=str(error)))
    for value in args.library:
        label, separator, path = value.partition('=')
        if not separator or not label or not path:
            parser.error('--library requires NAME=PATH')
        roots[label] = Path(path).resolve(strict=True)
    search = list({path.parent for path in roots.values()})
    queue = [(path, path) for path in roots.values()]
    images = {}
    while queue:
        image, origin = queue.pop(0)
        if str(image) in images:
            continue
        try:
            actual = command('lipo', '-archs', image).split()
            row = dict(architectures=actual, missing=sorted(set(arches) - set(actual)),
                       sha256=digest(image), dependencies={})
            images[str(image)] = row
            for arch in actual:
                if arch not in ('arm64', 'x86_64'):
                    continue
                linked = [name for name in dependencies(image, arch) if not system_library(name)]
                row['dependencies'][arch] = linked
                for name in linked:
                    queue.append((resolve(name, image, origin, search, arch), origin))
        except Exception as error:
            images.setdefault(str(image), {})['error'] = str(error)
            errors.append(dict(image=str(image), error=str(error)))
    missing = {path: row['missing'] for path, row in images.items() if row.get('missing')}
    report = dict(schema='H2_MACOS_DEPENDENCY_AUDIT_V1', requested_architectures=arches,
                  scope='explicit libraries' if args.library else 'host pkg-config packages; not configured engine closure',
                  packages=packages, roots={name: str(path) for name, path in roots.items()},
                  images=images, missing_architectures=missing, errors=errors,
                  passed=bool(roots) and not missing and not errors,
                  source_sha256={path.name: digest(path) for path in
                      (Path(__file__), Path(__file__).with_name('bundle.py'))},
                  retained_large_artifacts=[])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], libraries=len(images),
                          libraries_missing_slices=len(missing), errors=len(errors), report=str(args.output))))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
