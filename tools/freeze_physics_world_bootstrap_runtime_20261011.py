#!/usr/bin/env python3
"""Freeze the owned native initialization extension over immutable source-v10."""
import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/integration/physics-engine-20261010'
PARENT_PIN = 'f117c26f3fd0d5c38e2bb5ba123a17554a6021525a2ba4709c52a4625b6d55b2'
OWNED = (
    'cmake/PhysicsWorldRuntime.cmake',
    'src/physics/runtime/physics_world_runtime_generated.cpp',
    'tools/research_physics_world_bootstrap_runtime_20261011.py',
    'tools/generate_physics_world_bootstrap_runtime_20261011.py',
    'tools/research_physics_world_bootstrap_bindings_20261011.py',
    'tools/capture_physics_world_bootstrap_20261011.py',
    'tools/oracle_physics_world_bootstrap_checkpoint_20261011.py',
    'tools/oracle_physics_world_bootstrap_rebound_20261011.py',
    'tools/freeze_physics_world_bootstrap_runtime_20261011.py',
    'tools/validate_physics_world_bootstrap_runtime_20261011.py',
    'docs/PHYSICS_WORLD_BOOTSTRAP_ABI_RUNTIME_PORT_20261011.md',
)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parent = BASE / 'source-manifest-v10.json'
    assert sha(parent) == PARENT_PIN
    previous = json.loads(parent.read_text())
    source = BASE / 'source-v11'
    destination = BASE / 'source-manifest-v11.json'
    if destination.exists():
        report = json.loads(destination.read_text())
        assert report['parent_manifest_sha256'] == PARENT_PIN and report['owned_overlays'] == list(OWNED)
        for relative, pin in report['files'].items():
            assert sha(source / relative) == pin, relative
        for relative in OWNED:
            assert sha(ROOT / relative) == report['files'][relative], relative
        print(json.dumps(dict(files=len(report['files']), sha256=sha(destination), audit='passed')))
        return
    assert not source.exists(), 'Do not overwrite an existing or incomplete source freeze'
    for relative, pin in previous['files'].items():
        assert sha(BASE / 'source-v10' / relative) == pin, relative
    owned_pins = {relative: sha(ROOT / relative) for relative in OWNED}
    files = {}
    source.mkdir()
    for relative in sorted(set(previous['files']) | set(OWNED)):
        origin = ROOT / relative if relative in OWNED else BASE / 'source-v10' / relative
        target = source / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(origin, target)
        files[relative] = sha(target)
        assert files[relative] == (owned_pins[relative] if relative in OWNED else previous['files'][relative]), relative
    for relative, pin in owned_pins.items():
        assert sha(ROOT / relative) == pin, relative
    (source / 'analysis').symlink_to(ROOT / 'analysis', target_is_directory=True)
    report = dict(schema=1, parent_manifest_sha256=PARENT_PIN, owned_overlays=list(OWNED), files=files,
        scope='Immutable v10 plus listed initialization overlays. Top-level assets and analysis data excluded; inherited original test resources remain in this private validation tree. Not a redistributable code-only package or live gameplay proof.')
    destination.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(files=len(files), sha256=sha(destination))))


if __name__ == '__main__':
    main()
