#!/usr/bin/env python3
"""Freeze only the owned world event extension over immutable source-v9."""
import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/integration/physics-engine-20261010'
PARENT_PIN = 'f6134ae86ea5cac7e1a752ce8cb394c3148b59ff184686c36e58d466ed59587b'
OWNED = (
    'cmake/PhysicsWorldRuntime.cmake',
    'src/physics_world_runtime_generated.cpp',
    'tools/research_physics_world_event_runtime_20261011.py',
    'tools/generate_physics_world_event_runtime_20261011.py',
    'tools/oracle_physics_world_event_runtime_20261011.py',
    'tools/oracle_physics_world_event_full_20261011.py',
    'tools/oracle_physics_world_event_rebound_20261011.py',
    'tools/capture_physics_world_checkpoint_20261011.py',
    'tools/oracle_physics_world_checkpoint_20261011.py',
    'tools/freeze_physics_world_event_runtime_20261011.py',
    'tools/validate_physics_world_event_runtime_20261011.py',
    'docs/PHYSICS_WORLD_EVENT_ABI_RUNTIME_PORT_20261011.md',
    'docs/PHYSICS_WORLD_SNAPSHOT_ASSET_BOUNDARY_20261011.md',
    '.gitignore',
)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parent = BASE / 'source-manifest-v9.json'
    assert sha(parent) == PARENT_PIN
    previous = json.loads(parent.read_text())
    source = BASE / 'source-v10'
    destination = BASE / 'source-manifest-v10.json'
    if destination.exists():
        report = json.loads(destination.read_text())
        assert report['parent_manifest_sha256'] == PARENT_PIN
        assert report['owned_overlays'] == list(OWNED)
        for relative, pin in report['files'].items():
            assert sha(source / relative) == pin, relative
        for relative in OWNED:
            assert sha(ROOT / relative) == report['files'][relative], relative
        print(json.dumps(dict(files=len(report['files']), sha256=sha(destination), audit='passed')))
        return
    assert not source.exists(), 'Do not overwrite an incomplete or existing freeze'
    for relative, pin in previous['files'].items():
        assert sha(BASE / 'source-v9' / relative) == pin, relative
    for relative in OWNED:
        assert (ROOT / relative).is_file(), relative
    owned_pins = {relative: sha(ROOT / relative) for relative in OWNED}
    files = {}
    source.mkdir()
    for relative in sorted(set(previous['files']) | set(OWNED)):
        origin = ROOT / relative if relative in OWNED else BASE / 'source-v9' / relative
        target = source / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(origin, target)
        files[relative] = sha(target)
        if relative in OWNED:
            assert files[relative] == owned_pins[relative], relative
        else:
            assert files[relative] == previous['files'][relative], relative
    for relative, pin in owned_pins.items():
        assert sha(ROOT / relative) == pin, relative
    # Analysis evidence stays external. Inherited original test resources make
    # this a private validation tree, not a redistributable code-only package.
    (source / 'analysis').symlink_to(ROOT / 'analysis', target_is_directory=True)
    report = dict(schema=1, parent_manifest_sha256=PARENT_PIN,
        owned_overlays=list(OWNED), files=files,
        scope='Immutable v9 plus the listed owned event overlays. Top-level assets and analysis data excluded; inherited original test resources remain in this private validation tree. Not a redistributable code-only package or live gameplay proof.')
    destination.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(files=len(files), sha256=sha(destination))))


if __name__ == '__main__':
    main()
