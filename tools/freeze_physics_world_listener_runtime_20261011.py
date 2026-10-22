#!/usr/bin/env python3
"""Freeze only the owned contact-listener extension over immutable source-v8."""
import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/integration/physics-engine-20261010'
PARENT_PIN = '0b87dffca6fbe9e5ab72dbf6edd6e887b214cac873f514c9b809804c40532312'
OWNED = (
    'cmake/PhysicsWorldRuntime.cmake',
    'src/physics/runtime/physics_world_runtime_generated.cpp',
    'tools/research_physics_world_listener_runtime_20261011.py',
    'tools/generate_physics_world_listener_runtime_20261011.py',
    'tools/oracle_physics_world_listener_runtime_20261011.py',
    'tools/original_physics_file_hash.py',
    'tools/oracle_physics_world_listener_rebound_20261011.py',
    'tools/freeze_physics_world_listener_runtime_20261011.py',
    'tools/validate_physics_world_listener_runtime_20261011.py',
    'docs/PHYSICS_WORLD_LISTENER_ABI_RUNTIME_PORT_20261011.md',
)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parent = BASE / 'source-manifest-v8.json'
    assert sha(parent) == PARENT_PIN
    previous = json.loads(parent.read_text())
    source = BASE / 'source-v9'
    destination = BASE / 'source-manifest-v9.json'
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
        assert sha(BASE / 'source-v8' / relative) == pin, relative
    for relative in OWNED:
        assert (ROOT / relative).is_file(), relative
    owned_pins = {relative: sha(ROOT / relative) for relative in OWNED}
    files = {}
    source.mkdir()
    for relative in sorted(set(previous['files']) | set(OWNED)):
        origin = ROOT / relative if relative in OWNED else BASE / 'source-v8' / relative
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
    # Fixtures stay external to the code snapshot, as in all earlier freezes.
    (source / 'analysis').symlink_to(ROOT / 'analysis', target_is_directory=True)
    report = dict(schema=1, parent_manifest_sha256=PARENT_PIN,
        owned_overlays=list(OWNED), files=files,
        scope='Immutable v8 source plus the listed contact-listener overlays; original assets and analysis data excluded. Component validation is not live gameplay proof.')
    destination.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(files=len(files), sha256=sha(destination))))


if __name__ == '__main__':
    main()
