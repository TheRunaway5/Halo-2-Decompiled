#!/usr/bin/env python3
"""Read-only audit of the r7 preview and its explicit unfinished live gates."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    args = parser.parse_args()
    root = args.root.resolve()
    bank = root / 'analysis/multiplayer/integration/sandbox-preview-r7-20261010'
    manifest = json.loads((bank / 'delivery/manifest.json').read_text())
    failures = []
    counts = {}

    def check(label, base, entries):
        counts[label] = len(entries)
        for name, pin in entries.items():
            path = base / name
            if not path.is_file() or digest(path) != pin:
                failures.append(f'{label}: {name}')

    check('delivery', bank / 'delivery', manifest['delivery_files'])
    check('evidence', root, manifest['evidence'])
    check('external_inputs', root, manifest['external_inputs'])
    check('compiled_shaders', bank / 'build', manifest['compiled_shaders'])
    source_manifest = bank / 'source-manifest.json'
    if digest(source_manifest) != manifest['source_manifest_sha256']:
        failures.append('source manifest')
    check('source', bank / 'source', json.loads(source_manifest.read_text())['files'])
    for check_row in manifest['completed_checks']:
        if check_row['exit_code'] != 0:
            failures.append(f'failed completed check: {check_row["name"]}')
        for name, pin in check_row.get('binaries', {}).items():
            if not (bank / 'build' / name).is_file() or digest(bank / 'build' / name) != pin:
                failures.append(f'changed check binary: {name}')
        if check_row['log'] not in manifest['evidence']:
            failures.append(f'unpinned check: {check_row["name"]}')
    if manifest['live_lifts_complete'] or manifest['live_teleporters_complete'] or manifest['live_vehicles_complete']:
        failures.append('unfinished live owners incorrectly declared complete')
    for gate in manifest['open_live_gates']:
        if gate['exit_code'] != 1 or gate['log'] not in manifest['evidence']:
            failures.append(f'changed open gate: {gate["name"]}')
        if 'RED:' not in (root / gate['log']).read_text():
            failures.append(f'missing live failure signal: {gate["name"]}')
    preserved = json.loads((bank / 'preserved-launchers.json').read_text())
    for name, identity in preserved.items():
        path = root / name
        if str(path.resolve()) != identity['target'] or digest(path) != identity['sha256']:
            failures.append(f'preserved launcher changed: {name}')
    launcher = root / 'Halo2-Sandbox-Preview'
    target = bank / 'delivery/runtime/Halo2-Title-Launcher'
    if not launcher.is_symlink() or launcher.resolve() != target.resolve():
        failures.append('preview launcher binding')
    counts.update(completed_checks=len(manifest['completed_checks']),
                  open_live_gates=len(manifest['open_live_gates']),
                  mismatches=failures)
    print(json.dumps(counts, indent=2, sort_keys=True))
    raise SystemExit(bool(failures))


if __name__ == '__main__':
    main()
