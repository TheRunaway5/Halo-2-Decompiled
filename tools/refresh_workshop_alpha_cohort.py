"""Extend the declared material overlay without replacing unrelated source.

Run directly in the existing run_test.py command coordinator. Preserve all
previous overlays and reconstruct the exact selected sealed source inventory
before and after this delta. Assets, selections and runtimes stay unchanged.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path

from build_launcher_engine import source_hashes


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def inventory(pins):
    return hashlib.sha256(json.dumps(pins, sort_keys=True).encode()).hexdigest()


def replace(text, before, after):
    assert text.count(before) == 1 and after not in text
    return text.replace(before, after)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--preparation', type=Path, required=True)
    parser.add_argument('--component', type=Path, required=True)
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
    before = source_hashes(source)
    assert inventory(before) == prep['production_inventory_sha256']
    expected = json.loads((root / base['selected_runtime']).parent.joinpath('source-hashes.json').read_text())

    def restore(pins, changes):
        value = dict(pins)
        for change in changes:
            assert digest(source / change['path']) == change['sha256']
            if change['production']:
                if change['before'] is None:
                    value.pop(change['path'])
                else:
                    value[change['path']] = change['before']
        assert value == expected and inventory(value) == base['production_inventory_sha256']

    restore(before, parent['changes'])
    component = json.loads(args.component.read_text())
    assert component['passed'] and len(component['results']) == 3 and not Path(component['owned_scratch']).exists()
    for result in component['results']:
        path = args.component.parent / (result['compiler'] + ('-sanitizers' if result['sanitizers'] else '')) / 'evidence/alpha-environment.json'
        report = json.loads(path.read_text())
        assert report['passed'] and report['authored_rows'] == 88 and report['original_paired_rows'] == 68
        assert report['native_template_variants'] == 38 and report['synthetic_alias_owners'] == 1
        assert report['cases'] == 110 and report['complete_banks_checked'] == 1568
        assert report['guard_cases'] == 1201 and report['nondefault_transform_controls'] == 1
    assert all(digest(Path(path)) == pin for path, pin in component['source_pins'].items())
    journal = json.loads((args.preparation.parent / 'session/session.json').read_text())
    builder = [row for row in journal['commands'] if 'tests/test_build_launcher_engine.py' in row['argv']]
    assert builder and builder[-1]['exit_code'] == 0
    selections = {name: (root / name).read_bytes() for name in ('gameplay-runtime-selection.json', 'frontend-runtime-selection.json')}
    runtime = root / prep['selected_runtime']
    assert digest(runtime) == prep['runtime_sha256']
    copied = [
        'include/halo2/mcc_native_alpha_environment.hpp', 'src/rendering/mcc_native_alpha_environment.cpp',
        'src/rendering/mcc_native_alpha_environment_profiles.inc', 'cmake/MccNativeAlphaEnvironment.cmake',
        'tools/data/mcc-native-alpha-environment-profiles.json', 'tools/generate_mcc_native_alpha_environment.py',
        'tools/mcc_alpha_environment_projection.py', 'tools/export_mcc_native_alpha_environment_fixture.py',
        'tools/freeze_mcc_alpha_environment_fixture.py', 'tools/validate_mcc_alpha_environment.py',
        'tools/audit_shader_vertex_masks_tu15.py', 'tools/audit_mcc_alpha_environment_roles.py',
        'tests/mcc_native_alpha_environment_tests.cpp', 'tests/mcc_native_alpha_environment_tests.py',
        'tests/fixtures/mcc-native-alpha-environment-v1.json', 'tests/fixtures/mcc-native-alpha-environment-v1.json.zlib']
    planned = {}
    for name in copied:
        assert not (source / name).exists()
        planned[name] = (root / name).read_bytes()
    name = 'CMakeLists.txt'
    text = (source / name).read_text()
    anchor = 'target_sources(halo2_cache PRIVATE src/rendering/mcc_native_tiling_specular.cpp)\n'
    text = replace(text, anchor, anchor + 'target_sources(halo2_cache PRIVATE src/rendering/mcc_native_alpha_environment.cpp)\n')
    anchor = '    include(cmake/MccNativeTilingSpecular.cmake)\n'
    planned[name] = replace(text, anchor, anchor + '    include(cmake/MccNativeAlphaEnvironment.cmake)\n').encode()
    name = 'src/rendering/mcc_native_shaders.cpp'
    text = (source / name).read_text()
    anchor = '#include "halo2/mcc_native_tiling_specular.hpp"\n'
    text = replace(text, anchor, anchor + '#include "halo2/mcc_native_alpha_environment.hpp"\n')
    root_loader = (root / name).read_text()
    start = '            if(!projected)projected=project_mcc_alpha_environment('
    end = '            if(!projected)projected=project_mcc_world_default_roles('
    assert root_loader.count(start) == root_loader.count(end) == 1
    block = root_loader[root_loader.index(start):root_loader.index(end)]
    assert 'retained_tag_allocations_' in block and block.count('alias.address') == 1
    planned[name] = replace(text, end, block + end).encode()
    name = 'tools/build_launcher_engine.py'
    text = (source / name).read_text()
    anchor = "                   '--target', 'mcc_native_tiling_specular_probe',\n"
    text = replace(text, anchor, anchor + "                   '--target', 'mcc_native_alpha_environment_probe',\n")
    anchor = "                   'mcc_native_tiling_specular|mcc_native_tiling_specular_generator|'\n"
    text = replace(text, anchor, anchor + "                   'mcc_native_alpha_environment|mcc_native_alpha_environment_generator|'\n")
    anchor = "                   '--retain-report', 'tiling.json',\n"
    planned[name] = replace(text, anchor, anchor + "                   '--retain-report', 'alpha-environment.json',\n").encode()
    name = 'tests/test_build_launcher_engine.py'
    text = (source / name).read_text()
    anchor = "                     'mcc_native_tiling_specular','mcc_native_tiling_specular_generator']:\n"
    text = replace(text, anchor, "                     'mcc_native_tiling_specular','mcc_native_tiling_specular_generator',\n"
        "                     'mcc_native_alpha_environment','mcc_native_alpha_environment_generator']:\n")
    anchor = "        self.assertIn('tiling.json',commands[1])\n"
    planned[name] = replace(text, anchor, anchor + "        self.assertIn('mcc_native_alpha_environment_probe',commands[1])\n"
        "        self.assertIn('alpha-environment.json',commands[1])\n").encode()
    root_pins = {name: digest(root / name) for name in planned}
    combined = {row['path']: dict(row) for row in parent['changes']}
    delta = []
    for name, raw in planned.items():
        target = source / name
        assert target.parent.resolve().is_relative_to(source) and not target.is_symlink()
        row = dict(path=name, before=digest(target) if target.exists() else None,
                   sha256=hashlib.sha256(raw).hexdigest(), bytes=len(raw))
        delta.append(row)
        original_before = combined[name]['before'] if name in combined else row['before']
        combined[name] = dict(row, before=original_before)
    args.output.with_suffix('.planned.json').write_text(json.dumps(dict(scope=__doc__, source=str(source),
        parent_preparation=str(args.preparation), parent_inventory_sha256=inventory(before), delta=delta), indent=2) + '\n')
    for name, raw in planned.items():
        target = source / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(raw)
    after = source_hashes(source)
    changes = list(combined.values())
    for row in changes:
        row['production'] = row['path'] in after
    restore(after, changes)
    assert {name for name in before.keys() | after.keys() if before.get(name) != after.get(name)} == {
        row['path'] for row in delta if row['path'] in after}
    assert all(digest(root / name) == pin for name, pin in root_pins.items())
    assert all((root / name).read_bytes() == raw for name, raw in selections.items())
    assert digest(runtime) == prep['runtime_sha256']
    report = dict(scope=__doc__, passed=True, source=str(source), changes=changes, delta=delta,
        parent_preparation=str(args.preparation), parent_preparation_sha256=digest(args.preparation),
        parent_refresh=str(parent_path), parent_refresh_sha256=digest(parent_path),
        base_preparation=str(base_path), base_preparation_sha256=digest(base_path),
        production_inventory_before=base['production_inventory_sha256'], parent_production_inventory_sha256=inventory(before),
        production_inventory_sha256=inventory(after), production_files=len(after),
        component=str(args.component), component_sha256=digest(args.component), builder_control_command=builder[-1],
        helper_sha256=digest(Path(__file__)), Root_files_unchanged=True, unrelated_Root_loader_changes_preserved=True,
        selected_runtime_unchanged=True, source_published=False, owned_scratch=str(owner), retained_large_artifacts=[])
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    fresh = dict(prep, status='declared_alpha_environment_overlay_not_engine_qualified', sealed_inventory_verified=False,
        base_sealed_inventory_verified=True, overlay_inventory_verified=True, overlays=changes,
        production_inventory_sha256=inventory(after), production_files=len(after),
        source_refresh_receipt=str(args.output), source_refresh_sha256=digest(args.output))
    args.new_preparation.write_text(json.dumps(fresh, indent=2) + '\n')
    print(json.dumps(dict(passed=True, delta_files=len(delta), combined_overlay_files=len(changes),
        production_files=len(after), production_inventory_sha256=inventory(after), selected_runtime_unchanged=True)))


if __name__ == '__main__':
    main()
