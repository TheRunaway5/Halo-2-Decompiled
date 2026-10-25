#!/usr/bin/env python3
"""Freeze newer qualified delivery inputs without changing either source owner."""
import argparse
import json
import os
from pathlib import Path
import shutil
from weapon_test_cache import sha

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('prepared', 'base-manifest', 'reference-source', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    reference = args.reference_source.resolve(strict=True)
    if reference == owner or not reference.is_relative_to(ROOT / '.local/test-scratch'):
        raise ValueError('Read a distinct managed source snapshot')
    selected = ROOT / 'gameplay-runtime-selection.json'
    selection_pin = sha(selected)
    selection = json.loads(selected.read_text())
    runtime = ROOT / selection['runtime']
    qualification = runtime.parent / 'qualification.json'
    installed_path = runtime.parent / 'source-hashes.json'
    installed = json.loads(installed_path.read_text())
    if not json.loads(qualification.read_text())['passed'] or sha(runtime) != selection['runtime_sha256']:
        raise ValueError('Current delivery is not qualified')
    base = json.loads(args.base_manifest.read_text())['source_files_sha256']
    current = json.loads(args.prepared.read_text())['source_hashes']
    production = lambda name: name == 'CMakeLists.txt' or name.startswith(('src/', 'include/', 'cmake/', 'shaders/', 'rust/'))
    names = [name for name in installed.keys() | base.keys() if production(name) and installed.get(name) != base.get(name)]
    staged = owner / 'qualified-delivery-inputs'
    staged.mkdir()
    changes = []
    for name in sorted(names):
        if current.get(name) == installed.get(name):
            continue
        if current.get(name) != base.get(name) or name not in installed:
            raise ValueError('Qualified delivery needs an explicit merge: ' + name)
        path = reference / name
        if sha(path) != installed[name]:
            raise ValueError('Reference no longer contains selected delivery bytes: ' + name)
        destination = staged / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, destination)
        if sha(path) != installed[name] or sha(destination) != installed[name]:
            raise ValueError('Selected delivery source changed while freezing: ' + name)
        changes.append(dict(path=name, before=current.get(name), after=installed[name], bytes=destination.stat().st_size))
    if sha(selected) != selection_pin:
        raise ValueError('Selection changed during source staging')
    report = dict(status='staged', owner=str(owner), source=str(staged), selection_sha256=selection_pin,
                  selected_runtime=str(runtime), runtime_sha256=sha(runtime),
                  qualification_sha256=sha(qualification), selected_manifest_sha256=sha(installed_path),
                  prepared_sha256=sha(args.prepared), base_manifest_sha256=sha(args.base_manifest),
                  changes=changes, bytes=sum(row['bytes'] for row in changes),
                  borrowed_source_modified=False, full_vehicle_parity=False, tool_sha256=sha(Path(__file__)))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(status='staged', files=len(changes), bytes=report['bytes'], report=str(args.output))), flush=True)


if __name__ == '__main__':
    main()
