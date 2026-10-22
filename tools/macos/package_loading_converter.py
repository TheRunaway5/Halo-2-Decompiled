"""Stage and seal the native loading converter inside an owned app build."""
from __future__ import annotations

import json
from pathlib import Path
import shutil
import sys

from bundle import command, dependencies, digest, system_library
from build_vorbis import SOURCES

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from macos.loading_converter import BASE, EXECUTABLE, LIBRARIES, LICENSES, SCHEMA, image_architectures
from mcc_loading_presentation import CODEBOOK, CODEBOOK_SHA, CONVERTER_MANIFEST


def stage(resources, executable, sdk, root, arches):
    executable, sdk = executable.resolve(strict=True), sdk.resolve(strict=True)
    manifest_path = sdk / 'manifest.json'
    manifest = json.loads(manifest_path.read_text())
    expected = set(arches)
    if (manifest.get('schema') != 'H2_MACOS_VORBIS_SDK_V1' or
            not expected <= set(manifest.get('architectures', [])) or
            image_architectures(executable, 2) != expected):
        raise RuntimeError('Loading converter inputs do not provide the app CPU owners')
    if (set(manifest.get('libraries', {})) != {'libogg.0.dylib', 'libvorbis.0.dylib',
            'libvorbisenc.2.dylib', 'libvorbisfile.3.dylib'} or
            set(manifest.get('licenses', {})) != {'Ogg.txt', 'Vorbis.txt'} or
            any(manifest.get('sources', {}).get(name, {}).get('version') != version or
                manifest.get('sources', {}).get(name, {}).get('sha256') != sha
                for name, (version, sha) in SOURCES.items())):
        raise RuntimeError('Loading converter requires the pinned Ogg/Vorbis SDK inputs')
    for name, sha in manifest['libraries'].items():
        path = sdk / 'lib' / name
        if path.is_symlink() or digest(path) != sha:
            raise RuntimeError('Vorbis SDK library changed: ' + name)
    for name, sha in manifest['licenses'].items():
        if digest(sdk / 'share/licenses' / name) != sha:
            raise RuntimeError('Vorbis SDK license changed: ' + name)
    if (resources / BASE).exists() or (resources / BASE).is_symlink():
        raise RuntimeError('Loading converter staging requires a new owned directory')
    copies = [(executable, resources / EXECUTABLE)]
    copies += [(sdk / 'lib' / Path(name).name, resources / name) for name in LIBRARIES]
    copies += [(sdk / 'share/licenses' / Path(name).name, resources / name) for name in LICENSES[:2]]
    copies += [(root / 'external/ww2ogg/COPYING', resources / LICENSES[2]),
               (root / CODEBOOK, resources / CODEBOOK)]
    if digest(root / CODEBOOK) != CODEBOOK_SHA:
        raise RuntimeError('Loading converter codebook differs from the retained source')
    for source, destination in copies:
        if source.is_symlink():
            raise RuntimeError('Loading converter staging inputs must be regular files')
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
    (resources / EXECUTABLE).chmod(0o755)
    allowed = {Path(name).name for name in LIBRARIES}
    for name in (EXECUTABLE, *LIBRARIES):
        image = resources / name
        kind = 2 if name == EXECUTABLE else 6
        if image_architectures(image, kind) != expected:
            raise RuntimeError('Loading converter image CPU owners differ: ' + name)
        changes = {}
        for arch in arches:
            for dependency in dependencies(image, arch):
                if system_library(dependency):
                    continue
                basename = Path(dependency).name
                if basename not in allowed:
                    raise RuntimeError('Unexpected loading converter dependency: ' + dependency)
                changes[dependency] = '@loader_path/' + ('../lib/' if kind == 2 else '') + basename
        if kind == 6:
            command('install_name_tool', '-id', '@rpath/' + image.name, image)
        for before, after in changes.items():
            if before != after:
                command('install_name_tool', '-change', before, after, image)
    return dict(executable_input_sha256=digest(executable), sdk_input_sha256=digest(manifest_path),
                architectures=arches, pcm_profile_verified=False)


def write_manifest(resources, arches):
    """Run after Mach-O fixups/signing and before the enclosing app is sealed."""
    names = (EXECUTABLE, *LIBRARIES, *LICENSES, CODEBOOK)
    value = dict(schema=SCHEMA, architectures=list(arches),
                 files={name: dict(bytes=(resources / name).stat().st_size,
                                   sha256=digest(resources / name)) for name in names})
    path = resources / CONVERTER_MANIFEST
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + '\n')
    # Verify the same standard-library admission used by actual setup.
    from macos.loading_converter import dependencies as admit
    admit(resources, path, value)
