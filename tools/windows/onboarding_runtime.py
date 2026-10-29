"""Collect the native Python/Tk setup runtime and the shared onboarding sources.

The existing MinGW SDK stays immutable. Pillow's pinned extra dependencies use
their own reusable input prefix; developer headers and archives never enter the
launcher delivery. User MCC imports and reference executable banks are excluded.
"""
from __future__ import annotations

import json
from pathlib import Path

from prepare_sdk import prepare


def prepare_inputs(root, sdk):
    root, sdk = Path(root), Path(sdk)
    lock = root / 'tools/windows/onboarding-lock.json'
    data = json.loads(lock.read_text())
    receipt = json.loads((sdk / 'halo2-sdk.json').read_text())
    if receipt.get('lock_sha256') != data['base_sdk_lock_sha256']:
        raise RuntimeError('Onboarding Python requires the pinned compatible Windows SDK')
    prefix = sdk.parent / 'onboarding-python'
    prepare(root, prefix, lock_path=lock)
    return prefix


def _files(directory, *, exclude=()):
    for path in sorted(directory.rglob('*')):
        if not path.is_file() or any(part in exclude for part in path.relative_to(directory).parts):
            continue
        if path.is_symlink():
            raise RuntimeError('Onboarding input must be a direct regular file: ' + str(path))
        yield path


def collect(root, sdk, extra):
    """Return explicit package-relative source owners and interpreter metadata."""
    root, sdk, extra = (Path(path) for path in (root, sdk, extra))
    lock = json.loads((Path(__file__).with_name('onboarding-lock.json')).read_text())
    version = lock['python']
    stdlib = sdk / 'lib' / ('python' + version)
    pillow = extra / 'lib' / ('python' + version) / 'site-packages'
    required = [root / 'tools/setup_assets.py', root / 'tools/setup_assets_ui.py',
                root / 'tools/native_renderer_shaders.py', sdk / 'bin/python.exe',
                stdlib / 'encodings/__init__.py', stdlib / 'tkinter/__init__.py',
                pillow / 'PIL/__init__.py', sdk / 'lib/tcl8.6/init.tcl',
                sdk / 'lib/tk8.6/tk.tcl']
    for path in required:
        if path.is_symlink() or not path.is_file():
            raise RuntimeError('Missing native onboarding resource: ' + str(path))
    if not list((stdlib / 'lib-dynload').glob('_tkinter*.pyd')):
        raise RuntimeError('The Windows Python runtime lacks its native Tk extension')
    inputs = {'python/python.exe': sdk / 'bin/python.exe'}
    for source in _files(stdlib, exclude={'__pycache__', 'site-packages', 'test', 'tests',
                                        'idlelib', 'ensurepip', 'turtledemo'}):
        if source.suffix.lower() in {'.a', '.lib'} or any(part.startswith('config-') for part in source.relative_to(stdlib).parts):
            continue
        inputs['python/lib/python' + version + '/' + source.relative_to(stdlib).as_posix()] = source
    for source in _files(pillow, exclude={'__pycache__', 'tests'}):
        if source.relative_to(pillow).parts[0] == 'PIL' or source.relative_to(pillow).parts[0].startswith('pillow-'):
            inputs['python/lib/python' + version + '/site-packages/' + source.relative_to(pillow).as_posix()] = source
    for name in ('tcl8.6', 'tk8.6', 'tcl8'):
        directory = sdk / 'lib' / name
        for source in _files(directory):
            inputs['python/lib/' + name + '/' + source.relative_to(directory).as_posix()] = source
    for source in sorted((root / 'tools').glob('*.py')):
        if not source.is_symlink():
            inputs['tools/' + source.name] = source
    for name in ('__init__.py', 'atomic_files.py', 'steam_setup.py', 'loading_converter.py'):
        source = root / 'tools/macos' / name
        if source.is_file() and not source.is_symlink():
            inputs['tools/macos/' + name] = source
    source = root / 'tools/steam/content-profile.json'
    if source.is_file() and not source.is_symlink():
        inputs['tools/steam/content-profile.json'] = source
    for source in _files(root / 'tools/data', exclude={'__pycache__', '.git'}):
        inputs['tools/data/' + source.relative_to(root / 'tools/data').as_posix()] = source
    for source in sorted((root / 'tools/reference').glob('*.tsv')):
        if not source.is_symlink():
            inputs['tools/reference/' + source.name] = source
    for name in ('launcher.svg', 'frontend/mcc-native-layout.json'):
        source = root / 'resources' / name
        if not source.is_file() or source.is_symlink():
            raise RuntimeError('Missing shared launcher artwork/layout: ' + str(source))
        inputs['resources/' + name] = source
    for source in _files(root / 'resources/licenses'):
        inputs['resources/licenses/' + source.relative_to(root / 'resources/licenses').as_posix()] = source
    for name in ('COPYING', 'PROVENANCE.json', 'README.md', 'packed_codebooks_aoTuV_603.bin'):
        source = root / 'external/ww2ogg' / name
        if source.is_file() and not source.is_symlink():
            inputs['external/ww2ogg/' + name] = source
    for name in ('LICENSE', 'LICENSE.md', 'LICENSE.txt', 'COPYING'):
        source = root / name
        if source.is_file() and not source.is_symlink():
            inputs[name] = source
    for source in _files(extra / 'share/licenses'):
        inputs['licenses/onboarding/' + source.relative_to(extra / 'share/licenses').as_posix()] = source
    metadata = {'python': version, 'pillow_packages': json.loads((extra / 'halo2-sdk.json').read_text())['packages'],
                'interpreter': 'python/python.exe', 'setup': 'tools/setup_assets.py',
                'shared_ui': 'tools/setup_assets_ui.py',
                'scope': 'Shared Linux onboarding flow with native Windows Python, Tk and Pillow'}
    return inputs, metadata
