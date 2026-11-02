"""Admit only the camera declaration after complete actual E3 GPU proof.

Extend the current declared source cohort, preserving its original sealed-base
before-hashes and every earlier material overlay. Run directly in the active
run_test.py coordinator; never use a checkout or installed runtime as scratch.
The subsequent production-profile GPU run must generate no private declaration.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path

from build_launcher_engine import source_hashes
from generate_mcc_script_layout import generated


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory(pins):
    return hashlib.sha256(json.dumps(pins, sort_keys=True).encode()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--preparation', type=Path, required=True)
    parser.add_argument('--GPU-proof', dest='gpu', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--new-preparation', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source = owner / 'source'
    prep = json.loads(args.preparation.read_text())
    assert Path(prep['owned_scratch']) == owner and Path(prep['source']) == source
    assert prep['overlay_inventory_verified'] and prep['base_sealed_inventory_verified']
    parent_path = Path(prep['source_refresh_receipt'])
    assert digest(parent_path) == prep['source_refresh_sha256']
    parent = json.loads(parent_path.read_text())
    assert parent['passed'] and parent['changes'] == prep['overlays']
    base_path = Path(parent['base_preparation'])
    assert digest(base_path) == parent['base_preparation_sha256']
    base = json.loads(base_path.read_text())
    assert base['sealed_inventory_verified'] and not base['overlays']
    expected_base = json.loads((root / prep['selected_runtime']).parent.joinpath('source-hashes.json').read_text())

    def restore(pins, changes):
        restored = dict(pins)
        for row in changes:
            assert digest(source / row['path']) == row['sha256']
            if row['production']:
                if row['before'] is None:
                    restored.pop(row['path'])
                else:
                    restored[row['path']] = row['before']
        assert restored == expected_base and inventory(restored) == base['production_inventory_sha256']

    before = source_hashes(source)
    assert inventory(before) == prep['production_inventory_sha256']
    restore(before, parent['changes'])
    gpu = json.loads(args.gpu.read_text())
    assert gpu['passed'] and gpu['GPU_requested'] and not Path(gpu['owned_scratch']).exists()
    assert gpu['source_inventory_sha256'] == prep['production_inventory_sha256']
    for name in ('earthcity_e3', 'earthcity_e3_potato'):
        row = json.loads((args.gpu.parent / 'evidence' / (name + '.json')).read_text())
        view = row['GPU']
        assert row['passed'] and row['HS_calls'] == 110 and row['frames'] == 330 and len(row['calls']) == 55
        assert row['decoded_scenario_restored'] and row['complete_script_requirements_retained'] == 22
        assert view['passed'] and view['camera_control_pairs'] == 55 and len(view['frames']) == 110
        assert view['resident_BSP_draws'] and view['dynamic_BSP_draws'] and view['verified_BSP_vertex_bytes']
        assert view['authored_empty_BSP_controls'] == 3 and view['raw_captures_removed']
    assert all(digest(root / name) == pin for name, pin in gpu['GPU_test_sources'].items())
    test = root / 'tests/native_workshop_e3_camera_hs_tests.cpp'
    restored_message = test.read_text().replace('E3 camera consumer requires its authenticated admitted declaration',
                                               'E3 camera consumer requires its explicit private profile')
    assert hashlib.sha256(restored_message.encode()).hexdigest() == gpu['test_sha256']
    selections = {name: (root / name).read_bytes() for name in
                  ('gameplay-runtime-selection.json', 'frontend-runtime-selection.json')}
    assert digest(root / prep['selected_runtime']) == prep['runtime_sha256']
    profile_name = 'tools/mcc-script-layout.json'
    profile = json.loads((source / profile_name).read_text())
    assert len(profile['builtins']) == 411 and not any(row['mcc_opcode'] == 454 for row in profile['builtins'])
    admitted = dict(gpu['authenticated_mcc'], **gpu['authenticated_xbox'])
    assert admitted['mcc_opcode'] == 454 and admitted['xbox_opcode'] == 452
    assert admitted['arguments'] == admitted['xbox_arguments'] == [36, 11]
    assert admitted['return_type'] == admitted['xbox_return_type'] == 4 and admitted['xbox_evaluator'] == '0x2b27b0'
    proposed = copy.deepcopy(profile)
    proposed['builtins'].append(admitted)
    proposed['builtins'].sort(key=lambda row: row['mcc_opcode'])
    assert json.loads((root / profile_name).read_text()) == proposed
    assert (root / 'src/campaign/scripting/mcc_script_layout.inc').read_text() == generated(proposed)
    names = [profile_name, 'src/campaign/scripting/mcc_script_layout.inc',
        'tools/validate_workshop_e3_camera_hs.py', 'tests/native_workshop_e3_camera_hs_tests.cpp',
        'tests/native_workshop_e3_camera_gpu.cpp', 'tests/native_workshop_e3_camera_gpu.hpp']
    planned = {name: (root / name).read_bytes() for name in names}
    combined = {row['path']: dict(row) for row in parent['changes']}
    delta = []
    for name, raw in planned.items():
        target = source / name
        assert target.parent.resolve().is_relative_to(source) and not target.is_symlink()
        row = dict(path=name, before=digest(target) if target.exists() else None,
                   sha256=hashlib.sha256(raw).hexdigest(), bytes=len(raw))
        delta.append(row)
        combined[name] = dict(row, before=combined[name]['before'] if name in combined else row['before'])
    args.output.with_suffix('.planned.json').write_text(json.dumps(dict(source=str(source), delta=delta), indent=2) + '\n')
    for name, raw in planned.items():
        target = source / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(raw)
    after = source_hashes(source)
    changes = list(combined.values())
    for row in changes:
        row['production'] = row['path'] in after
    restore(after, changes)
    assert all((root / name).read_bytes() == raw for name, raw in planned.items())
    assert all((root / name).read_bytes() == raw for name, raw in selections.items())
    assert digest(root / prep['selected_runtime']) == prep['runtime_sha256']
    report = dict(scope=__doc__, passed=True, source=str(source), changes=changes, delta=delta,
        base_preparation=str(base_path), base_preparation_sha256=digest(base_path),
        parent_preparation=str(args.preparation), parent_preparation_sha256=digest(args.preparation),
        parent_refresh=str(parent_path), parent_refresh_sha256=digest(parent_path),
        production_inventory_before=base['production_inventory_sha256'], parent_production_inventory_sha256=inventory(before),
        production_inventory_sha256=inventory(after), production_files=len(after),
        component=str(args.gpu), component_sha256=digest(args.gpu),
        sole_admitted_MCC_opcode=454, production_profile_rows=412,
        helper_sha256=digest(Path(__file__)), selected_runtime_unchanged=True,
        unrelated_Root_loader_changes_preserved=True, source_published=False, owned_scratch=str(owner), retained_large_artifacts=[])
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    fresh = dict(prep, status='declared_camera_admission_not_production_engine_qualified',
        overlays=changes, production_inventory_sha256=inventory(after), production_files=len(after),
        source_refresh_receipt=str(args.output), source_refresh_sha256=digest(args.output))
    args.new_preparation.write_text(json.dumps(fresh, indent=2) + '\n')
    print(json.dumps(dict(passed=True, delta_files=len(delta), combined_overlay_files=len(changes),
                         production_files=len(after), production_inventory_sha256=inventory(after))))


if __name__ == '__main__':
    main()
