#!/usr/bin/env python3
"""Freeze a validator-only correction; retain exact v19 compiled inputs."""
import json
import shutil
from freeze_physics_game_frame_runtime_20261011 import ROOT, BASE, sha

PARENT_PIN = '87abf847fc5688a1e6567fe426b44dc6a1e4427805c798f5363a338411889c94'
CORRECTIONS = ('tools/validate_physics_game_frame_runtime_20261011.py',
               'tools/freeze_physics_game_frame_audit_correction_20261011.py')


def main():
    parent_path = BASE / 'source-manifest-v19.json'
    assert sha(parent_path) == PARENT_PIN
    parent = json.loads(parent_path.read_text())
    source, path = BASE / 'source-v19a', BASE / 'source-manifest-v19a.json'
    if path.exists():
        j = json.loads(path.read_text())
        assert j['parent_manifest_sha256'] == PARENT_PIN and j['metadata_overlays'] == list(CORRECTIONS)
        for relative, pin in j['files'].items():
            assert sha(source / relative) == pin, relative
        for relative in CORRECTIONS:
            assert sha(ROOT / relative) == j['files'][relative], relative
        print(json.dumps(dict(files=len(j['files']), sha256=sha(path), audit='passed')))
        return
    assert not source.exists(), 'Preserve any incomplete correction freeze'
    for relative, pin in parent['files'].items():
        assert sha(BASE / 'source-v19' / relative) == pin, relative
    pins = {relative: sha(ROOT / relative) for relative in CORRECTIONS}
    source.mkdir()
    files = {}
    for relative in sorted(set(parent['files']) | set(CORRECTIONS)):
        origin = ROOT / relative if relative in CORRECTIONS else BASE / 'source-v19' / relative
        target = source / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(origin, target)
        assert sha(target) == (pins[relative] if relative in CORRECTIONS else parent['files'][relative]), relative
        files[relative] = sha(target)
    for relative, pin in pins.items():
        assert sha(ROOT / relative) == pin, relative
    assert all(files[p] == h for p, h in parent['files'].items() if p not in CORRECTIONS)
    (source / 'analysis').symlink_to(ROOT / 'analysis', target_is_directory=True)
    report = dict(schema=1, parent_manifest_sha256=PARENT_PIN, metadata_overlays=list(CORRECTIONS), files=files,
        compiled_inputs_unchanged=True, build_source_manifest_sha256=PARENT_PIN,
        correction='The validator assumed FSbase06400000. Actual authenticated outer-entry FSbase is00AC0000. Runtime, hardware bridge and all31 Release/sanitizer comparisons already used the actual capture. The corrected validator reads the captured field; no C++/header/CMake, fixture, binary or test log changes.',
        scope='Metadata-only v19a descendant. All compiled inputs remain byte-identical to tested immutable v19; its original manifest and failed audit remain preserved.')
    path.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(files=len(files), sha256=sha(path))))


if __name__ == '__main__':
    main()
