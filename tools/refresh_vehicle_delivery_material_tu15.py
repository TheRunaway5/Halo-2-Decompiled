#!/usr/bin/env python3
"""Join the hardware-qualified native material owner to the vehicle delivery tree."""
import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path
import re

from build_launcher_engine import source_hashes
from native_damage_material_kernel_tu15 import install_native_material_lookup, strip_native_material_lookup
from native_difficulty_parameter_kernel_tu15 import strip_native_difficulty_routes
from weapon_test_cache import sha

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/vehicles-tu15-goal-20261023'
MATERIAL = ROOT / 'analysis/destruction-goal-20261022'


def restore_production_cmake(args, owner, source, validation):
    prepared = json.loads((args.manifest.parent / 'prepared.json').read_text())
    log = (args.validation.parent / 'tests.log').read_text()
    if (not re.search(r'100% tests passed(?:, 0 tests failed)? out of 9', log) or
            len(re.findall(r'Test\s+#\d+:.*?\bPassed\b', log)) != 9):
        raise ValueError('Joined delivery requires all nine declared checks to pass without skips')
    cases = {}
    for row in validation['retained_small_reports']:
        if row['status'] != 'retained': raise ValueError('A joined source report was not retained')
        path = Path(row['retained_path'])
        if sha(path) != row['sha256']: raise ValueError('A joined source report changed')
        data = json.loads(path.read_text())
        if not data['passed']: raise ValueError('A joined native scenario failed')
        name = path.stem.removeprefix('vehicle-difficulty-')
        if name == 'campaign_guntower_no_damage':
            if not data['original_no_damage_information_gate_verified']:
                raise ValueError('The original no-information rejection was not verified')
        elif not (data['native_material_firing_calls'] > 0 and
                  data['native_difficulty_firing_activity']['parameter_calls'] > 0):
            raise ValueError('A joined firing interval missed its native kernels')
        cases[name] = dict(report=str(path), sha256=row['sha256'],
            native_material_firing_calls=data.get('native_material_firing_calls'),
            native_difficulty_firing_activity=data.get('native_difficulty_firing_activity'))
    if set(cases) != {'ghost', 'wraith_minigun', 'campaign_flood_clump', 'campaign_guntower_no_damage'}:
        raise ValueError('The joined native cohort differs from its declared scope')
    path = source / 'CMakeLists.txt'; current = path.read_bytes()
    canonical = current[:prepared['canonical_cmake_bytes']]
    if hashlib.sha256(canonical).hexdigest() != prepared['canonical_cmake_sha256']:
        raise ValueError('Canonical CMake prefix changed during native validation')
    if not current[len(canonical):].startswith(b'\n# VEHICLE_DIFFICULTY_NATIVE_QUALIFICATION\n'):
        raise ValueError('Private native registrations have an undeclared boundary')
    args.output.mkdir(parents=True, exist_ok=False)
    binding = dict(source_manifest_sha256=sha(args.manifest), source_unchanged_after_run=True,
        expected_checks=9, actual_retained_scenarios=cases, runtime_opcode_execution=False,
        base_35_case_validation_sha256=prepared['base_validation_sha256'], full_vehicle_parity=False)
    validation['joined_native_material_source_binding'] = binding
    args.validation.write_text(json.dumps(validation, indent=2) + '\n')
    before = sha(path); path.write_bytes(canonical)
    manifest = source_hashes(source)
    (args.output / 'source-hashes.json').write_text(json.dumps(manifest, indent=2) + '\n')
    report = dict(owner=str(owner), passed=True, scope='Restore only declared test registrations after joined native validation',
        validation_sha256=sha(args.validation), changes=[dict(path='CMakeLists.txt', before=before, after=sha(path))],
        source_binding=binding, production_bodies_changed=False, launcher_rebuilt=False,
        full_vehicle_parity=False, tool_sha256=sha(Path(__file__)))
    (args.output / 'prepared.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(status='canonical', report=str(args.output / 'prepared.json'))), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--validation', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--restore-only', action='store_true',
        help='Restore the authenticated canonical CMake prefix after joined live validation')
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True); source = owner / 'source'
    if source.is_symlink() or source_hashes(source) != json.loads(args.manifest.read_text()):
        raise ValueError('Current qualified delivery source changed')
    validation = json.loads(args.validation.read_text())
    if not validation['passed'] or Path(validation['build']) != owner / 'build':
        raise ValueError('Current native difficulty cohort must pass before the next admission')
    for command in json.loads((BASE / 'session-v1/session.json').read_text())['commands']:
        if 'finished' not in command and not any('\n' not in arg and Path(arg).name == Path(__file__).name for arg in command['argv']):
            raise ValueError('Another source/build command is still running')
    if args.restore_only:
        return restore_production_cmake(args, owner, source, validation)
    qualified_path = MATERIAL / 'material-native-frozen-owner-v1/report.json'
    qualified = json.loads(qualified_path.read_text())
    generation_path = MATERIAL / 'material-native-route-generation-v2.json'
    generation = json.loads(generation_path.read_text())
    pins = json.loads((qualified_path.parent / 'source-hashes.json').read_text())
    negative_path = MATERIAL / 'material-native-frozen-negative-v1/report.json'
    negative = json.loads(negative_path.read_text())
    negative_pins = json.loads((negative_path.parent / 'source-hashes.json').read_text())
    if (not qualified['passed'] or qualified['native_differences'] != 0 or qualified['hardware_cases'] != 77 or
            not qualified['whole_owner_component_ABI_and_banks_qualified'] or not qualified['source_unchanged'] or
            not all(row['passed'] and row['differences'] == 0 for row in qualified['native_owner_routes']) or
            not generation['passed'] or not generation['prior_module_exactly_preserved']):
        raise ValueError('Material owner has no passing unchanged-original hardware qualification')
    kernel_name = 'src/gameplay/objects/source_damage_material_native_tu15_internal.hpp'
    kernel = (ROOT / kernel_name).read_text(); product = 'detail::damage_product(modifier,accumulated)'
    if (not negative['passed'] or not negative['swapped_product_rejected'] or
            not all(row['product_mutation_rejected'] for row in negative['native_owner_routes']) or
            negative['original_owner_reference']['sha256'] != qualified['original_owner_reference']['sha256'] or
            kernel.count(product) != 1 or hashlib.sha256(kernel.replace(product,
                'detail::damage_product(accumulated,modifier)').encode()).hexdigest() != negative_pins[kernel_name]):
        raise ValueError('Current material kernel has no matching rejected operand-reversal control')
    changes = {}
    for name in ('include/halo2/detail/damage_product_native.hpp',
                 'include/halo2/source_damage_material_native_tu15.hpp',
                 'src/gameplay/objects/source_damage_material_native_tu15_internal.hpp'):
        if sha(ROOT / name) != pins[name]: raise ValueError('Qualified material source changed: ' + name)
        changes[name] = (ROOT / name).read_bytes()
    name = 'src/gameplay/units/source_damage_delivery_runtime_generated.cpp'
    before = (source / name).read_text(); after = install_native_material_lookup(before)
    if (strip_native_material_lookup(after) != before or
            hashlib.sha256(strip_native_difficulty_routes(after).encode()).hexdigest() != generation['generated_cpp_sha256']):
        raise ValueError('Joined module differs from the two independently qualified source preimages')
    changes[name] = after.encode()
    name = 'tools/native_damage_material_kernel_tu15.py'; changes[name] = (ROOT / name).read_bytes()
    name = 'tools/fix_damage_delivery_observation_tu15.py'; text = (source / name).read_text()
    for anchor, addition in (
        ('def canonical(source):\n', '    from native_damage_material_kernel_tu15 import strip_native_material_lookup\n    source=strip_native_material_lookup(source)\n'),
        ('    output=install_native_model_lookup(output)\n', '    from native_damage_material_kernel_tu15 import install_native_material_lookup\n    output=install_native_material_lookup(output)\n')):
        if text.count(anchor) != 1: raise ValueError('Frozen generator material hook changed')
        text = text.replace(anchor, anchor + addition, 1)
    changes[name] = text.encode()
    name = 'tests/destruction_runtime_tu15_tests.cpp'; text = (source / name).read_text()
    for before, after in (
        ('#include "halo2/source_damage_model_native_tu15.hpp"\n', '#include "halo2/source_damage_model_native_tu15.hpp"\n#include "halo2/source_damage_material_native_tu15.hpp"\n'),
        ('  SourceDamageModelNativeActivity native_model_before=source_damage_model_native_activity();\n',
         '  SourceDamageModelNativeActivity native_model_before=source_damage_model_native_activity();\n  SourceDamageMaterialNativeActivity native_material_before=source_damage_material_native_activity();\n'),
        ('    std::ofstream output(path);output<<recorded.dump(2)<<\'\\n\';\n',
         '    const auto material=source_damage_material_native_activity();\n'
         '    recorded["native_damage_material_activity"]={{"calls",material.calls-native_material_before.calls},\n'
         '        {"groups_found",material.groups_found-native_material_before.groups_found},\n'
         '        {"modifiers_found",material.modifiers_found-native_material_before.modifiers_found}};\n'
         '    std::ofstream output(path);output<<recorded.dump(2)<<\'\\n\';\n'),
        ('  bool pending_qualified=false;\n', '  bool pending_qualified=false;\n  const auto native_material_firing_before=source_damage_material_native_activity();\n'),
        ('  const auto native_difficulty_firing_after=source_difficulty_parameter_native_activity();\n',
         '  const auto native_material_firing_after=source_damage_material_native_activity();\n'
         '  report.data["native_material_firing_calls"]=native_material_firing_after.calls-native_material_firing_before.calls;\n'
         '  const auto native_difficulty_firing_after=source_difficulty_parameter_native_activity();\n'),
        ('  report.data["passed"]=true;\n',
         '  if(std::getenv("HALO2_DESTRUCTION_REQUIRE_NATIVE_MATERIAL") && name!="campaign_guntower_no_damage")\n'
         '    check(source_damage_material_native_activity().calls>native_material_firing_before.calls,\n'
         '        "Actual firing interval executed no native material-damage lookup");\n'
         '  report.data["passed"]=true;\n')):
        if text.count(before) != 1: raise ValueError('Frozen material diagnostic anchor changed')
        text = text.replace(before, after, 1)
    changes[name] = text.encode()
    name = 'CMakeLists.txt'; text = (source / name).read_text()
    needle = 'HALO2_DESTRUCTION_REQUIRE_NATIVE_DIFFICULTY=1"'
    prepared = json.loads((args.manifest.parent / 'prepared.json').read_text())
    if text.count(needle) != len(prepared['destruction_cases']):
        raise ValueError('Frozen native cohort environment changed')
    changes[name] = text.replace(needle, 'HALO2_DESTRUCTION_REQUIRE_NATIVE_DIFFICULTY=1;HALO2_DESTRUCTION_REQUIRE_NATIVE_MATERIAL=1"').encode()
    args.output.mkdir(parents=True, exist_ok=False); rows = []
    for name, data in changes.items():
        path = source / name; before = path.read_bytes() if path.exists() else b''
        before_pin = sha(path) if path.exists() else None
        path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(data)
        rows.append(dict(path=name, before=before_pin, after=sha(path)))
        (args.output / (name.replace('/', '__') + '.patch')).write_text(''.join(difflib.unified_diff(
            before.decode().splitlines(True), data.decode().splitlines(True), fromfile='before/' + name, tofile='after/' + name, n=2)))
    (args.output / 'source-hashes.json').write_text(json.dumps(source_hashes(source), indent=2) + '\n')
    report = dict(owner=str(owner), changes=rows, validation_not_run=True,
        base_manifest_sha256=sha(args.manifest), base_validation_sha256=sha(args.validation),
        material_qualification_sha256=sha(qualified_path), material_generation_sha256=sha(generation_path),
        material_negative_control_sha256=sha(negative_path),
        preserved_source_preimages=True, canonical_cmake_sha256=prepared['canonical_cmake_sha256'],
        canonical_cmake_bytes=prepared['canonical_cmake_bytes'], destruction_cases=prepared['destruction_cases'],
        material_qualification_limits=qualified['scope'], full_vehicle_parity=False, tool_sha256=sha(Path(__file__)))
    (args.output / 'prepared.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(status='prepared', files=len(rows), report=str(args.output / 'prepared.json'))), flush=True)


if __name__ == '__main__':
    main()
