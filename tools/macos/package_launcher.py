#!/usr/bin/env python3
"""Package an existing universal launcher with the owned Python setup runtime.

Outputs must be new build artifacts. Existing apps and installed runtimes are
never replaced. A setup-only app does not claim to include a playable engine.
Validation invokes this helper within run_tests.py's disposable build.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import plistlib
import shutil
import sys

from bundle import bundle, command, digest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from native_variant_assets import install as install_variants
from controller_prompt_assets import install as install_controller_prompts


def package(root, launcher, sdk, app, arches, libraries, licenses, engine=None, shaders=None, steam_sdk=None,
            loading_converter=None, vorbis_sdk=None):
    root, launcher, sdk = (p.resolve(strict=True) for p in (root, launcher, sdk))
    app = app.absolute()
    if app.exists() or app.is_symlink() or app.suffix != '.app':
        raise RuntimeError('Choose a new .app build output; existing applications are preserved')
    if bool(engine) != bool(shaders):
        raise RuntimeError('Include an engine and its shader directory together')
    if bool(loading_converter) != bool(vorbis_sdk):
        raise RuntimeError('Include the loading converter and its owned Vorbis SDK together')
    manifest = json.loads((sdk / 'manifest.json').read_text())
    if manifest.get('schema') != 'H2_MACOS_PYTHON_RUNTIME_V1' or not set(arches) <= set(manifest['architectures']):
        raise RuntimeError('Python runtime does not provide the requested architectures')
    for name, row in manifest['files'].items():
        path = (sdk / name).resolve(strict=True)
        if not path.is_relative_to(sdk) or path.stat().st_size != row['bytes'] or digest(path) != row['sha256']:
            raise RuntimeError('Python SDK input changed: ' + name)
    for arch in arches:
        command('lipo', launcher, '-verify_arch', arch)
    contents = app / 'Contents'
    macos, resources = contents / 'MacOS', contents / 'Resources'
    macos.mkdir(parents=True)
    resources.mkdir()
    variants = install_variants(root, resources)
    shutil.copy2(launcher, macos / 'Halo2-Launcher')
    (contents / 'Info.plist').write_bytes(plistlib.dumps(dict(
        CFBundleExecutable='Halo2-Launcher', CFBundleIdentifier='net.halo2forever.launcher',
        CFBundleName='Halo 2', CFBundlePackageType='APPL', CFBundleVersion='0.1.0',
        CFBundleShortVersionString='0.1.0', LSMinimumSystemVersion='13.0',
        NSHighResolutionCapable=True,
        NSLocalNetworkUsageDescription='Halo 2 connects to local multiplayer games and other players.')))
    shutil.copytree(sdk / 'python', resources / 'python', symlinks=True)
    shutil.copytree(sdk / 'licenses', resources / 'licenses/Python-installer')
    if steam_sdk is not None:
        steam_sdk = steam_sdk.resolve(strict=True)
        steam_manifest = json.loads((steam_sdk / 'manifest.json').read_text())
        if steam_manifest.get('schema') != 'H2_MACOS_STEAM_SDK_V1' or set(steam_manifest['architectures']) != set(arches):
            raise RuntimeError('Steam helper does not provide the app CPU owners')
        for name, row in steam_manifest['files'].items():
            path = (steam_sdk / name).resolve(strict=True)
            if not path.is_relative_to(steam_sdk) or path.stat().st_size != row['bytes'] or digest(path) != row['sha256']:
                raise RuntimeError('Steam SDK input changed: ' + name)
        shutil.copytree(steam_sdk, resources / 'steam')
    tools = resources / 'tools'
    tools.mkdir()
    for source in (root / 'tools').glob('*.py'):
        if not source.is_symlink():
            shutil.copy2(source, tools / source.name)
    (tools / 'steam').mkdir()
    shutil.copy2(root / 'tools/steam/content-profile.json', tools / 'steam/content-profile.json')
    shutil.copytree(root / 'tools/data', tools / 'data', ignore=shutil.ignore_patterns('__pycache__'))
    reference = tools / 'reference'
    reference.mkdir()
    for source in (root / 'tools/reference').glob('*.tsv'):
        if not source.is_symlink():
            shutil.copy2(source, reference / source.name)
    # Only the Darwin runtime Python package is needed by setup. Dependency
    # builders and their download inputs stay in the build tree.
    (tools / 'macos').mkdir()
    for name in ('__init__.py', 'atomic_files.py', 'steam_setup.py', 'loading_converter.py'):
        shutil.copy2(root / 'tools/macos' / name, tools / 'macos' / name)
    for name in ('launcher.svg', 'frontend/mcc-native-layout.json'):
        destination = resources / 'resources' / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(root / 'resources' / name, destination)
    shutil.copytree(root / 'resources/licenses', resources / 'resources/licenses')
    install_controller_prompts(root,resources)
    for name in ('LICENSE', 'LICENSE.md', 'LICENSE.txt', 'COPYING'):
        source = root / name
        if source.is_file():
            shutil.copy2(source, resources / name)
    if engine:
        for arch in arches:
            command('lipo', engine, '-verify_arch', arch)
        shutil.copy2(engine, macos / 'halo2_native')
        shutil.copytree(shaders, resources / 'shaders')
    loading = None
    if loading_converter is not None:
        from package_loading_converter import stage
        loading = stage(resources, loading_converter, vorbis_sdk, root, arches)
    (resources / 'launcher-build.json').write_text(json.dumps(dict(
        schema='H2_MACOS_LAUNCHER_BUILD_V1', architectures=arches,
        launcher_input_sha256=digest(launcher), python_sdk_input_sha256=digest(sdk / 'manifest.json'),
        python=manifest['python'], pillow=manifest['pillow'], certifi=manifest['certifi'],
        steam_sdk_input_sha256=digest(steam_sdk / 'manifest.json') if steam_sdk is not None else None,
        loading_converter=loading,
        bundled_native_variants=variants,
        engine_included=bool(engine), playable_engine_verified=False,
        installation_requirement='User-owned Halo MCC installation; import remains subject to existing admission checks',
        bundled_game_media=False), indent=2) + '\n')
    sealers = ()
    if loading is not None:
        from package_loading_converter import write_manifest
        sealers = (lambda: write_manifest(resources, arches),)
    bundle(app, arches, libraries, licenses, resource_sealers=sealers)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--launcher', type=Path, required=True)
    parser.add_argument('--python-sdk', type=Path, required=True)
    parser.add_argument('--app', type=Path, required=True)
    parser.add_argument('--arch', action='append', choices=['arm64', 'x86_64'], required=True)
    parser.add_argument('--library', type=Path, action='append', default=[])
    parser.add_argument('--license', action='append', default=[], metavar='NAME=PATH')
    parser.add_argument('--engine', type=Path)
    parser.add_argument('--shaders', type=Path)
    parser.add_argument('--steam-sdk', type=Path)
    parser.add_argument('--loading-converter', type=Path)
    parser.add_argument('--vorbis-sdk', type=Path)
    args = parser.parse_args()
    if sys.platform != 'darwin':
        parser.error('Launcher packaging requires macOS SDK tools')
    package(args.root, args.launcher, args.python_sdk, args.app, args.arch, args.library,
        [(v.split('=', 1)[0], Path(v.split('=', 1)[1])) for v in args.license], args.engine, args.shaders, args.steam_sdk,
        args.loading_converter, args.vorbis_sdk)


if __name__ == '__main__':
    main()
