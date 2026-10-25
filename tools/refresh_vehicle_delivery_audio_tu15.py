#!/usr/bin/env python3
"""Admit the independently recovered scenery audio fix into an owned delivery tree."""
import argparse
import difflib
import json
import os
from pathlib import Path

from build_launcher_engine import source_hashes
from weapon_test_cache import sha

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source = owner / 'source'
    if source.is_symlink() or not source.is_dir():
        raise ValueError('A private owned delivery source is required')
    manifest = json.loads(args.manifest.read_text())
    if source_hashes(source) != manifest:
        raise ValueError('Canonical delivery source changed after qualification')
    authority = ROOT / 'analysis/combat-presentation-audit-20261022/repair-scenery-audio'
    receipt = authority / 'scenery-angle-native/receipt.json'
    qualified = json.loads(receipt.read_text())
    if not qualified['passed'] or qualified['exit_code'] != 0:
        raise ValueError('Independent native qualification did not pass')
    changes = {}
    for name, pins in qualified['changes'].items():
        if sha(source / name) != pins['before'] or sha(ROOT / name) != pins['after']:
            raise ValueError('Qualified audio preimage or result changed: ' + name)
        changes[name] = (ROOT / name).read_bytes()
    fixture = 'tests/fixtures/scenery_audio_angle_tu15.json'
    if sha(ROOT / fixture) != qualified['fixture_sha256']:
        raise ValueError('Independent original angle fixture changed')
    generator = 'tools/generate_object_loop_audio_tu15.py'
    if sha(ROOT / generator) != '727cb92709de2621af402bdf00dbb06091d1efa960868db6b1efc0f1598354df':
        raise ValueError('Qualified generator changed')
    for name in (generator, fixture, 'tests/scenery_audio_angle_tu15_tests.cpp'):
        changes[name] = (ROOT / name).read_bytes()
    # Add just the qualified regression registration, not the drifting root CMake.
    root_cmake = (ROOT / 'CMakeLists.txt').read_text()
    start = '    add_executable(scenery_audio_angle_tu15_tests '
    end = '    add_executable(source_object_audio_views_tu15_tests '
    if root_cmake.count(start) != 1 or root_cmake.count(end) != 1:
        raise ValueError('Qualified CMake registration anchors changed')
    block = root_cmake[root_cmake.index(start):root_cmake.index(end)]
    cmake = (source / 'CMakeLists.txt').read_text()
    if start in cmake or cmake.count(end) != 1:
        raise ValueError('Delivery CMake preimage changed')
    changes['CMakeLists.txt'] = cmake.replace(end, block + end, 1).encode()
    args.output.mkdir(parents=True, exist_ok=False)
    records = []
    for name, data in changes.items():
        target = source / name
        before = target.read_bytes() if target.exists() else b''
        before_pin = sha(target) if target.exists() else None
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        records.append(dict(path=name, before=before_pin, after=sha(target)))
        (args.output / (name.replace('/', '__') + '.patch')).write_text(''.join(
            difflib.unified_diff(before.decode().splitlines(True), data.decode().splitlines(True),
                                 fromfile='before/' + name, tofile='after/' + name)))
    updated = source_hashes(source)
    declared = {name for name in changes if name in updated}
    actual = {name for name in manifest.keys() | updated.keys() if manifest.get(name) != updated.get(name)}
    if actual != declared:
        raise ValueError('An undeclared production source changed during scoped refresh')
    (args.output / 'source-hashes.json').write_text(json.dumps(updated, indent=2) + '\n')
    report = dict(owner=str(owner), changes=records, base_manifest_sha256=sha(args.manifest),
                  qualification_receipt_sha256=sha(receipt), tool_sha256=sha(Path(__file__)),
                  source_result_supplied=False, full_vehicle_parity=False,
                  validation_not_run=True, production_changed_paths=sorted(actual))
    (args.output / 'prepared.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(status='prepared', files=len(records), report=str(args.output / 'prepared.json'))))


if __name__ == '__main__':
    main()
