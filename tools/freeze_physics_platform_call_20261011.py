#!/usr/bin/env python3
"""Freeze the native platform/world join over the authenticated semantic lock cohort."""
import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/integration/physics-engine-20261010'
PARENT_PIN = 'f97efedd92fb7bd661eccb7e512fe9a131c1f3c396e684392669e762d681a968'
APPEND = '\ninclude(cmake/PhysicsPlatformCall.cmake)\n'
OWNED = (
    'src/physics_world_runtime_generated.cpp',
    'include/halo2/physics_platform_call.hpp', 'src/physics_platform_call.cpp',
    'src/physics_platform_call_bindings.hpp', 'tests/physics_platform_call_tests.cpp',
    'cmake/PhysicsPlatformCall.cmake',
    'tools/research_physics_world_platform_runtime_20261011.py',
    'tools/generate_physics_world_platform_runtime_20261011.py',
    'tools/research_physics_platform_call_bindings_20261011.py',
    'tools/oracle_physics_world_platform_bridge_20261011.py',
    'tools/freeze_physics_platform_call_20261011.py',
    'tools/validate_physics_platform_call_20261011.py',
    'docs/PHYSICS_WORLD_NATIVE_PLATFORM_JOIN_20261011.md',
)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parent_path = BASE / 'source-manifest-v12.json'
    assert sha(parent_path) == PARENT_PIN
    parent = json.loads(parent_path.read_text())
    source = BASE / 'source-v13'
    path = BASE / 'source-manifest-v13.json'
    expected_cmake = (BASE / 'source-v12/CMakeLists.txt').read_text() + APPEND
    assert (ROOT / 'CMakeLists.txt').read_text().count('include(cmake/PhysicsPlatformCall.cmake)') == 1
    if path.exists():
        report = json.loads(path.read_text())
        assert report['parent_manifest_sha256'] == PARENT_PIN and report['owned_overlays'] == list(OWNED)
        for relative, pin in report['files'].items():
            assert sha(source / relative) == pin, relative
        for relative in OWNED:
            assert sha(ROOT / relative) == report['files'][relative], relative
        assert (source / 'CMakeLists.txt').read_text() == expected_cmake
        print(json.dumps(dict(files=len(report['files']), sha256=sha(path), audit='passed')))
        return
    assert not source.exists(), 'Preserve an existing or incomplete source freeze'
    for relative, pin in parent['files'].items():
        assert sha(BASE / 'source-v12' / relative) == pin, relative
    pins = {relative: sha(ROOT / relative) for relative in OWNED}
    source.mkdir()
    files = {}
    for relative in sorted(set(parent['files']) | set(OWNED)):
        target = source / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        if relative == 'CMakeLists.txt':
            target.write_text(expected_cmake)
        else:
            origin = ROOT / relative if relative in OWNED else BASE / 'source-v12' / relative
            shutil.copyfile(origin, target)
            assert sha(target) == (pins[relative] if relative in OWNED else parent['files'][relative]), relative
        files[relative] = sha(target)
    for relative, pin in pins.items():
        assert sha(ROOT / relative) == pin, relative
    (source / 'analysis').symlink_to(ROOT / 'analysis', target_is_directory=True)
    report = dict(schema=1, parent_manifest_sha256=PARENT_PIN, owned_overlays=list(OWNED), files=files,
        top_level_cmake_transform=dict(parent_sha256=parent['files']['CMakeLists.txt'], appended_text=APPEND),
        scope='Immutable v12 plus listed native platform/world join overlays; CMake derives only from the frozen parent and the explicit append, not other shared-checkout edits. Private validation tree inherits original test resources; not a redistributable asset-free package or live gameplay proof.')
    path.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(files=len(files), sha256=sha(path))))


if __name__ == '__main__':
    main()
