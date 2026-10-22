#!/usr/bin/env python3
"""Freeze the original outer game physics frame over immutable v18."""
import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/integration/physics-engine-20261010'
PARENT_PIN = '3a6db1913e6d881b8d81c2852643e7541ee8d7bd042ff808d71e725bea0d5ae0'
APPEND = '\ninclude(cmake/PhysicsGameFrame.cmake)\n'
OWNED = (
    'src/physics/runtime/physics_world_runtime_generated.cpp',
    'include/halo2/physics_game_frame_runtime.hpp',
    'src/physics/physics_game_frame_runtime.cpp',
    'tests/physics_game_frame_runtime_tests.cpp',
    'cmake/PhysicsGameFrame.cmake',
    'tools/research_physics_game_frame_runtime_20261011.py',
    'tools/capture_physics_game_frame_20261011.py',
    'tools/generate_physics_game_frame_runtime_20261011.py',
    'tools/oracle_physics_game_frame_checkpoint_20261011.py',
    'tools/oracle_physics_game_frame_hardware_20261011.py',
    'tools/physics_game_frame_hardware_trace_20261011.cpp',
    'tools/original_i386_game_frame_bridge_20261011.py',
    'tools/prepare_physics_game_frame_fp_cases_20261011.py',
    'tools/freeze_physics_game_frame_runtime_20261011.py',
    'tools/validate_physics_game_frame_runtime_20261011.py',
    'docs/PHYSICS_GAME_FRAME_RUNTIME_20261011.md',
)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parent_path = BASE / 'source-manifest-v18.json'
    assert sha(parent_path) == PARENT_PIN
    parent = json.loads(parent_path.read_text())
    source = BASE / 'source-v19'
    path = BASE / 'source-manifest-v19.json'
    expected_cmake = (BASE / 'source-v18/CMakeLists.txt').read_text() + APPEND
    assert (ROOT / 'CMakeLists.txt').read_text().count('include(cmake/PhysicsGameFrame.cmake)') == 1
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
    assert not source.exists(), 'Preserve prior or incomplete freeze'
    for relative, pin in parent['files'].items():
        assert sha(BASE / 'source-v18' / relative) == pin, relative
    pins = {relative: sha(ROOT / relative) for relative in OWNED}
    source.mkdir()
    files = {}
    for relative in sorted(set(parent['files']) | set(OWNED)):
        target = source / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        if relative == 'CMakeLists.txt':
            target.write_text(expected_cmake)
        else:
            origin = ROOT / relative if relative in OWNED else BASE / 'source-v18' / relative
            shutil.copyfile(origin, target)
            assert sha(target) == (pins[relative] if relative in OWNED else parent['files'][relative]), relative
        files[relative] = sha(target)
    for relative, pin in pins.items():
        assert sha(ROOT / relative) == pin, relative
    (source / 'analysis').symlink_to(ROOT / 'analysis', target_is_directory=True)
    report = dict(schema=1, parent_manifest_sha256=PARENT_PIN, owned_overlays=list(OWNED), files=files,
        top_level_cmake_transform=dict(parent_sha256=parent['files']['CMakeLists.txt'], appended_text=APPEND),
        scope='Immutable v18 plus listed complete outer game-frame, public entry and host original-CPU oracle overlays. Parent headers remain unchanged; current shared-checkout headers and startup/vehicle work are not silently adopted. Private analysis resources stay external to the source snapshot. This is not a fresh world, live match or redistributable asset-free package.')
    path.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(files=len(files), sha256=sha(path))))


if __name__ == '__main__':
    main()
