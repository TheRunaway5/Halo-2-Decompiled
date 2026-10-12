#!/usr/bin/env python3
"""Freeze and audit the composed vehicle/object update and bounded evidence.

The standalone cohort uses the production CMake module and exact transitive
headers. Shared checkout edits outside that cohort are excluded. Original
execution remains a test-only oracle; no live driving parity is inferred.
"""
import argparse
import hashlib
import json
import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
E = ROOT / 'analysis/multiplayer/sandbox-source/vehicle-motion-20261011'
OWNED = (
    'include/halo2/physics_vehicle_motion.hpp',
    'src/physics_vehicle_motion_internal.hpp',
    'src/physics_vehicle_motion_generated.cpp',
    'tests/physics_vehicle_motion_tests.cpp',
    'cmake/PhysicsVehicleMotion.cmake',
    'tools/research_vehicle_motion_20261011.py',
    'tools/generate_vehicle_motion_20261011.py',
    'tools/oracle_vehicle_motion_20261011.py',
    'tools/validate_vehicle_motion_20261011.py',
    'docs/PHYSICS_VEHICLE_MOTION_PORT_20261011.md',
)
TOP = '''cmake_minimum_required(VERSION 3.20)
project(halo2_physics_vehicle_motion_cohort LANGUAGES CXX)
include(CTest)
option(HALO2_SANITIZERS "Instrument the complete component" OFF)
find_package(nlohmann_json CONFIG REQUIRED)
include(cmake/PhysicsVehicleRuntime.cmake)
include(cmake/PhysicsVehicleUpdate.cmake)
include(cmake/PhysicsVehicleMotion.cmake)
'''
REGISTRATION = 'include(cmake/PhysicsVehicleMotion.cmake)'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


EXPLICIT_SUPPORT = ('src/physics_vehicle_runtime_generated.cpp',
    'tests/physics_vehicle_runtime_tests.cpp', 'cmake/PhysicsVehicleRuntime.cmake',
    'src/physics_vehicle_update_generated.cpp', 'tests/physics_vehicle_update_tests.cpp', 'cmake/PhysicsVehicleUpdate.cmake',
    'tests/physics_world_runtime_tests.cpp')
PARENT = ROOT / 'analysis/multiplayer/sandbox-source/vehicle-update-20261011'
PARENT_MANIFEST_PIN = 'f90f1f92fecac38d3ec6279c0c9945da18a851d914033c66db6fae89a246f6c2'


def verify_parent():
    assert sha(PARENT / 'source-manifest-v1.json') == PARENT_MANIFEST_PIN
    parent = json.loads((PARENT / 'source-manifest-v1.json').read_text())
    independent_world='src/physics_world_runtime_generated.cpp'
    for relative,pin in parent['files'].items():
        assert sha(PARENT/'source-v1'/relative)==pin,relative
        # The independently evolving world implementation is not linked by
        # this standalone vehicle cohort. Preserve it and test its root target.
        if relative!=independent_world:assert pin==sha(ROOT/relative),relative
    return dict(frozen_parent_files_unchanged=len(parent['files']),
        current_parent_component_files_unchanged=len(parent['files'])-1,
        independent_world_source=independent_world, prior_world_source_sha256=parent['files'][independent_world])


def support_headers():
    result = set(EXPLICIT_SUPPORT)
    pending = [ROOT / path for path in OWNED + EXPLICIT_SUPPORT if Path(path).suffix in ('.cpp', '.hpp')]
    visited = set(pending)
    while pending:
        path = pending.pop()
        for include in re.findall(r'^#include "([^"]+)"', path.read_text(), re.M):
            candidates = [path.parent / include, ROOT / 'include' / include, ROOT / 'src' / include]
            matches = [p.resolve() for p in candidates if p.is_file()]
            assert matches, (path, include)
            target = matches[0]
            relative = str(target.relative_to(ROOT))
            if relative not in OWNED:
                result.add(relative)
            if target not in visited:
                visited.add(target)
                pending.append(target)
    return sorted(result)


def freeze():
    destination = E / 'source-v1'
    assert not destination.exists()
    assert REGISTRATION in (ROOT / 'CMakeLists.txt').read_text()
    overlay = verify_parent()
    support = support_headers()
    files = {relative: sha(ROOT / relative) for relative in list(OWNED) + support}
    destination.mkdir(parents=True)
    for relative, pin in files.items():
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / relative, target)
        assert sha(target) == pin == sha(ROOT / relative)
    (destination / 'CMakeLists.txt').write_text(TOP)
    (destination / 'analysis').symlink_to(ROOT / 'analysis', target_is_directory=True)
    manifest = dict(schema=1, owned=list(OWNED), support=support, files=files,
        standalone_top_sha256=sha(destination / 'CMakeLists.txt'),
        production_registration=REGISTRATION, analysis_inputs_copied=False,
        retained_parent_manifest_sha256=PARENT_MANIFEST_PIN, retained_parent_sources=overlay,
        root_world_compatibility_sources_recorded=True)
    path = E / 'source-manifest-v1.json'
    path.write_text(json.dumps(manifest, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(source_manifest_sha256=sha(path), frozen_files=len(files))))


def validate():
    source = E / 'source-v1'
    manifest_path = E / 'source-manifest-v1.json'
    manifest = json.loads(manifest_path.read_text())
    assert manifest['owned'] == list(OWNED) and manifest['support'] == support_headers()
    for relative, pin in manifest['files'].items():
        assert sha(source / relative) == pin == sha(ROOT / relative), relative
    assert sha(source / 'CMakeLists.txt') == manifest['standalone_top_sha256']
    assert (source / 'CMakeLists.txt').read_text() == TOP
    assert (source / 'analysis').resolve() == ROOT / 'analysis'
    assert manifest['production_registration'] == REGISTRATION
    assert REGISTRATION in (ROOT / 'CMakeLists.txt').read_text()
    assert manifest['retained_parent_manifest_sha256'] == PARENT_MANIFEST_PIN
    assert manifest['retained_parent_sources'] == verify_parent()

    correspondence = E / 'correspondence-verified/report.json'
    assert sha(correspondence) == sha(E / 'correspondence-repeat/report.json')
    paired = json.loads(correspondence.read_text())
    assert paired['extension_tool_sha256'] == sha(ROOT / 'tools/research_vehicle_motion_20261011.py')
    assert paired['parent_sha256'] == sha(PARENT / 'correspondence-verified/report.json')
    assert paired['complete_bodies'] == 80 and len(paired['instruction_map']) == 10613
    instruction_mnemonics={}
    for row in paired['routines']:
        for line in (correspondence.parent/'instructions'/row['disc_listing']['file']).read_text().splitlines():
            fields=line.split();instruction_mnemonics[int(fields[0],16)]=fields[2]
        for label in ('disc', 'later'):
            listing = row[label + '_listing']
            for directory in ('correspondence-verified', 'correspondence-repeat'):
                assert sha(E / directory / 'instructions' / listing['file']) == listing['sha256']

    generated = E / 'generated-verified/manifest.json'
    assert sha(generated) == sha(E / 'generated-repeat/manifest.json')
    generator = json.loads(generated.read_text())
    assert generator['generator_sha256'] == sha(ROOT / 'tools/generate_vehicle_motion_20261011.py')
    assert generator['correspondence_sha256'] == sha(correspondence)
    assert generator['generated_cpp_sha256'] == sha(ROOT / 'src/physics_vehicle_motion_generated.cpp')
    assert generator['generated_cpp_sha256'] == sha(E / 'generated-repeat/physics_vehicle_motion_generated.cpp')
    assert len(generator['lowered_instructions']) == 8019
    assert set(generator['lowered_instructions']) < set(paired['instruction_map'])
    assert len(generator['complete_native_entries']) == 48
    assert len(generator['reused_parent_native_entries']) == 32
    assert {int(p, 16) for p in generator['complete_native_entries']} == set(paired['additional_native_entries'])
    assert not generator['production_opcode_decoder'] and not generator['production_original_code_execution']

    original = E / 'original-verified/report.json'
    assert sha(original) == sha(E / 'original-repeat/report.json')
    report = json.loads(original.read_text())
    assert report['tool_sha256'] == sha(ROOT / 'tools/oracle_vehicle_motion_20261011.py')
    assert report['correspondence_sha256'] == sha(correspondence)
    assert report['update_controls_tool_sha256']==sha(ROOT/'tools/oracle_vehicle_update_20261011.py')
    assert report['parent_controls_tool_sha256'] == sha(ROOT / 'tools/oracle_vehicle_runtime_20261011.py')
    assert report['reader_sha256'] == sha(ROOT / 'tools/inspect_multiplayer.py')
    assert report['segments_helper_sha256'] == sha(ROOT / 'tools/export_mp_physics_contact_lifecycle.py')
    assert report['declared_inputs'] == 3957 and report['authored_definitions'] == 48
    assert not report['instruction_patches'] and not report['native_expected_data_used']
    coverage = {}
    map_pins = {}
    for image in ('disc', 'u5'):
        path = E / 'original-verified' / (image + '.json')
        info = report['fixtures'][image]
        assert sha(path) == info['fixture_sha256'] == sha(E / 'original-repeat' / (image + '.json'))
        fixture = json.loads(path.read_text())
        assert fixture['tool_sha256'] == report['tool_sha256']
        assert fixture['correspondence_sha256'] == sha(correspondence)
        assert not fixture['instruction_patches'] and not fixture['native_expected_data_used']
        assert len(fixture['cases']) == info['cases'] == 3957
        assert len({case['id'] for case in fixture['cases']}) == 3957
        for case in fixture['cases']:
            mask=0x7d00
            for site in case['instructions']:
                mnemonic=instruction_mnemonics[site]
                if mnemonic in ('fcom','fcomp','fcompp','fucompp'):mask=0x7d00
                elif mnemonic in ('fsin','fcos'):mask=0x3c00
                elif mnemonic.startswith('f') and mnemonic not in ('fnstsw','fnstcw'):mask=0x3800
            assert case['x87_defined_status_mask']==mask
            setups=[e for e in case['writes'] if not e['instruction']]
            if setups:
                call=case['subsequent_call'];assert call is not None
                assert [e['address']-case['entry_abi']['gp'][4] for e in setups]==[int(x) for x in sorted(call['stack'])]
                assert [int.from_bytes(bytes.fromhex(e['written']),'little') for e in setups]==[call['stack'][x] for x in sorted(call['stack'])]

        assert sum(case['complete'] for case in fixture['cases']) == info['complete'] == 2437
        assert sum(len(case['instructions']) for case in fixture['cases']) == info['instructions'] == 2779263
        assert sum(len(case['writes']) for case in fixture['cases']) == info['ordered_writes'] == 451797
        assert sum(len(bytes.fromhex(bank['hex'])) for case in fixture['cases']
                   for bank in case['final_banks']) == info['full_state_bytes'] == 1400600
        seen = {p for case in fixture['cases'] for p in case['instructions']}
        assert len(seen) == info['covered_instructions'] == 6447
        coverage[image] = {}
        for row in paired['routines']:
            lines = (correspondence.parent / 'instructions' / row['disc_listing']['file']).read_text().splitlines()
            covered = sum(int(line.split()[0], 16) in seen for line in lines)
            coverage[image][row['disc_1_1_entry']] = dict(compiled=row['instructions'], exercised=covered)
        assert all(coverage[image][entry]['compiled']==coverage[image][entry]['exercised']
                   for entry in ('00030bd0','001d4510','0031f9e0','00321070'))
        builders=[c for c in fixture['cases'] if c['entry']==0x2095a0 and not c.get('subsequent_call')]
        assert len(builders)==576 and all(c['complete'] for c in builders)
        staged=[c for c in fixture['cases'] if c.get('subsequent_call')]
        assert len(staged)==432
        for entry in (0xf4650,0xf5e40,0x2090a0):
            chosen=[c for c in staged if c['subsequent_call']['entry']==entry]
            assert chosen and all(c['complete'] for c in chosen)
            assert all(0x2095a0 in c['instructions'] and entry in c['instructions'] for c in chosen)
        assert len(fixture['definitions']) == 48
        for definition in fixture['definitions']:
            map_path = ROOT / definition['map']
            if map_path not in map_pins:
                map_pins[map_path] = sha(map_path)
            assert map_pins[map_path] == definition['map_sha256']
            with map_path.open('rb') as handle:
                handle.seek(definition['file_offset'])
                raw = handle.read(len(bytes.fromhex(definition['prefix_hex'])))
            assert raw.hex() == definition['prefix_hex']
            assert hashlib.sha256(raw).hexdigest() == definition['prefix_sha256']
            for tag in definition['dependent_tags']:
                with map_path.open('rb') as handle:handle.seek(tag['file_offset']);raw=handle.read(len(bytes.fromhex(tag['raw_hex'])))
                assert raw.hex()==tag['raw_hex'] and hashlib.sha256(raw).hexdigest()==tag['sha256']

    logs = {}
    binaries = {}
    for name in ('release', 'sanitizer', 'production'):
        for stage in ('configure', 'build', 'test'):
            path = E / f'{name}-{stage}.log'
            logs[path.name] = sha(path)
        output = (E / f'{name}-test.log').read_text()
        for image in ('disc', 'u5'):
            assert re.search(r'original_physics_vehicle_motion_' + image + r'\s+\.+\s+Passed', output)
            assert re.search(r'original_physics_vehicle_update_' + image + r'\s+\.+\s+Passed', output)
            assert re.search(r'original_physics_vehicle_runtime_' + image + r'\s+\.+\s+Passed', output)
        assert output.count('PASS original physics vehicle motion: 3957 cases, 2779263 instructions, 451797 ordered writes, 1400600 full-state bytes') == 2
        assert output.count('PASS original physics vehicle update: 2505 cases, 748940 instructions, 70683 ordered writes, 395004 full-state bytes') == 2
        assert output.count('PASS original physics vehicle runtime: 1582 cases, 351662 instructions, 43321 ordered writes, 265655 full-state bytes') == 2
        assert '100% tests passed' in output and 'Skipped' not in output
        if name == 'production':
            assert re.search(r'original_source_vehicle_control\s+\.+\s+Passed', output)
            assert len(re.findall(r'Test\s+#\d+: original_physics_world_\S+\s+\.+\s+Passed', output)) == 12
            directory = ROOT / 'build-vehicle-control-20261011'
        else:
            directory = E / f'build-{name}'
        binaries[name] = sha(directory / 'physics_vehicle_motion_tests')
        ninja = (directory / 'build.ninja').read_text()
        block = ninja.split('CMakeFiles/halo2_physics_vehicle_motion.dir/src/physics_vehicle_motion_generated.cpp.o:')[1].split('\n\n')[0]
        for flag in ('-fno-fast-math', '-ffp-contract=off', '-frounding-math'):
            assert flag in block
        if name == 'sanitizer':
            assert '-fsanitize=address,undefined' in block
            parent_block = ninja.split('CMakeFiles/halo2_physics_vehicle_runtime.dir/src/physics_vehicle_runtime_generated.cpp.o:')[1].split('\n\n')[0]
            assert '-fsanitize=address,undefined' in parent_block
    result = dict(schema=1, validator_sha256=sha(__file__), source_manifest_sha256=sha(manifest_path),
        correspondence_sha256=sha(correspondence), generated_manifest_sha256=sha(generated),
        original_report_sha256=sha(original), compiled_paired_bodies=80, compiled_instructions=10613, added_native_bodies=48, added_native_instructions=8019,
        exercised_instructions=6447, coverage=coverage, original_cases_per_image=3957,
        complete_cases_per_image=2437, required_owner_prefix_cases_per_image=1520,
        native_comparisons_per_build=7914, native_instructions_per_build=5558526,
        ordered_writes_per_build=903594, full_state_bytes_per_build=2801200,
        original_correspondence_and_generation_repeats_identical=True,
        production_tests_passed=True, frozen_release_tests_passed=True,
        frozen_asan_ubsan_leak_tests_passed=True, logs=logs, binaries=binaries,
        retained_parent_comparisons_per_build=8174, production_world_compatibility_tests_passed=12,
        retained_parent_manifest_sha256=PARENT_MANIFEST_PIN,
        retained_parent_sources=manifest['retained_parent_sources'],
        production_world_binary_sha256=sha(ROOT / 'build-vehicle-control-20261011/physics_world_runtime_tests'),
        production_world_source_sha256=sha(ROOT/'src/physics_world_runtime_generated.cpp'),
        original_state_builder_complete_cases_per_image=576, state_builder_then_motion_cases_per_image=432,
        x87_undefined_status_bits_excluded_by_architecture=True, instruction_patches=[], original_code_executed_by_production=False,
        live_vehicle_driving_integrated=False, full_physics_parity=False,
        limits=['Declared rigid-body, object, node-pose and environment inputs; original state construction, not live driving',
                'Missing animation, solver, CRT remainder and CRT power owners preserve actual prefixes',
                '6447 exercised instructions do not prove all 10613 compiled instructions',
                'Masked FP exceptions; exception/status flags and hardware equivalence unproven',
                'GCC/Clang x86 backend; Windows/macOS and Xbox hardware unverified',
                'Live player/vehicle/object, all-map, split-screen and LAN integration remains'])
    path = E / 'validation.json'
    path.write_text(json.dumps(result, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(native_comparisons_per_build=7914, exercised_instructions=6447,
                          validation_sha256=sha(path))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--freeze', action='store_true')
    args = parser.parse_args()
    freeze() if args.freeze else validate()
