#!/usr/bin/env python3
"""Freeze and audit the original vehicle ABI component and bounded evidence.

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
E = ROOT / 'analysis/multiplayer/sandbox-source/vehicle-runtime-20261011'
OWNED = (
    'include/halo2/physics_vehicle_runtime.hpp',
    'src/physics/vehicles/physics_vehicle_runtime_internal.hpp',
    'src/physics/vehicles/physics_vehicle_runtime_generated.cpp',
    'tests/physics_vehicle_runtime_tests.cpp',
    'cmake/PhysicsVehicleRuntime.cmake',
    'tools/research_vehicle_runtime_20261011.py',
    'tools/generate_vehicle_runtime_20261011.py',
    'tools/oracle_vehicle_runtime_20261011.py',
    'tools/validate_vehicle_runtime_20261011.py',
    'docs/PHYSICS_VEHICLE_ABI_RUNTIME_PORT_20261011.md',
)
TOP = '''cmake_minimum_required(VERSION 3.20)
project(halo2_physics_vehicle_runtime_cohort LANGUAGES CXX)
include(CTest)
option(HALO2_SANITIZERS "Instrument the complete component" OFF)
find_package(nlohmann_json CONFIG REQUIRED)
include(cmake/PhysicsVehicleRuntime.cmake)
'''
REGISTRATION = 'include(cmake/PhysicsVehicleRuntime.cmake)'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def support_headers():
    result = set()
    pending = [ROOT / path for path in OWNED if Path(path).suffix in ('.cpp', '.hpp')]
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
    support = support_headers()
    files = {relative: sha(ROOT / relative) for relative in list(OWNED) + support}
    destination.mkdir(parents=True)
    for relative, pin in files.items():
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / relative, target)
        assert sha(target) == pin == sha(ROOT / relative)
    (destination / 'CMakeLists.txt').write_text(TOP)
    (destination / 'analysis').symlink_to(ROOT / 'analysis', target_is_directory=True)
    manifest = dict(schema=1, owned=list(OWNED), support=support, files=files,
        standalone_top_sha256=sha(destination / 'CMakeLists.txt'),
        production_registration=REGISTRATION, analysis_inputs_copied=False)
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

    correspondence = E / 'correspondence-verified/report.json'
    assert sha(correspondence) == sha(E / 'correspondence-repeat/report.json')
    paired = json.loads(correspondence.read_text())
    assert paired['extension_tool_sha256'] == sha(ROOT / 'tools/research_vehicle_runtime_20261011.py')
    assert paired['parent_sha256'] == sha(E.parent / 'vehicle-control-20261011/correspondence-verified/report.json')
    assert paired['complete_bodies'] == 10 and len(paired['instruction_map']) == 1526
    for row in paired['routines']:
        for label in ('disc', 'later'):
            listing = row[label + '_listing']
            for directory in ('correspondence-verified', 'correspondence-repeat'):
                assert sha(E / directory / 'instructions' / listing['file']) == listing['sha256']

    generated = E / 'generated-verified/manifest.json'
    assert sha(generated) == sha(E / 'generated-repeat/manifest.json')
    generator = json.loads(generated.read_text())
    assert generator['generator_sha256'] == sha(ROOT / 'tools/generate_vehicle_runtime_20261011.py')
    assert generator['correspondence_sha256'] == sha(correspondence)
    assert generator['generated_cpp_sha256'] == sha(ROOT / 'src/physics/vehicles/physics_vehicle_runtime_generated.cpp')
    assert generator['generated_cpp_sha256'] == sha(E / 'generated-repeat/physics_vehicle_runtime_generated.cpp')
    assert set(generator['lowered_instructions']) == set(paired['instruction_map'])
    assert len(generator['complete_native_entries']) == 10
    assert not generator['production_opcode_decoder'] and not generator['production_original_code_execution']

    original = E / 'original-verified/report.json'
    assert sha(original) == sha(E / 'original-repeat/report.json')
    report = json.loads(original.read_text())
    assert report['tool_sha256'] == sha(ROOT / 'tools/oracle_vehicle_runtime_20261011.py')
    assert report['correspondence_sha256'] == sha(correspondence)
    assert report['reader_sha256'] == sha(ROOT / 'tools/inspect_multiplayer.py')
    assert report['segments_helper_sha256'] == sha(ROOT / 'tools/export_mp_physics_contact_lifecycle.py')
    assert report['declared_inputs'] == 1582 and report['authored_definitions'] == 48
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
        assert len(fixture['cases']) == info['cases'] == 1582
        assert len({case['id'] for case in fixture['cases']}) == 1582
        assert sum(case['complete'] for case in fixture['cases']) == info['complete'] == 1033
        assert sum(len(case['instructions']) for case in fixture['cases']) == info['instructions'] == 351662
        assert sum(len(case['writes']) for case in fixture['cases']) == info['ordered_writes'] == 43321
        assert sum(len(bytes.fromhex(bank['hex'])) for case in fixture['cases']
                   for bank in case['final_banks']) == info['full_state_bytes'] == 265655
        seen = {p for case in fixture['cases'] for p in case['instructions']}
        assert len(seen) == info['covered_instructions'] == 832
        coverage[image] = {}
        for row in paired['routines']:
            lines = (correspondence.parent / 'instructions' / row['disc_listing']['file']).read_text().splitlines()
            covered = sum(int(line.split()[0], 16) in seen for line in lines)
            coverage[image][row['disc_1_1_entry']] = dict(compiled=row['instructions'], exercised=covered)
        assert all(coverage[image][entry]['compiled'] == coverage[image][entry]['exercised']
                   for entry in ('000f06b0', '000f5930', '00208d80', '00208e70', '00326af0'))
        assert len(fixture['definitions']) == 48
        for definition in fixture['definitions']:
            map_path = ROOT / definition['map']
            if map_path not in map_pins:
                map_pins[map_path] = sha(map_path)
            assert map_pins[map_path] == definition['map_sha256']
            with map_path.open('rb') as handle:
                handle.seek(definition['file_offset'])
                raw = handle.read(0x300)
            assert raw.hex() == definition['prefix_hex']
            assert hashlib.sha256(raw).hexdigest() == definition['prefix_sha256']

    logs = {}
    binaries = {}
    for name in ('release', 'sanitizer', 'production'):
        for stage in ('configure', 'build', 'test'):
            path = E / f'{name}-{stage}.log'
            logs[path.name] = sha(path)
        output = (E / f'{name}-test.log').read_text()
        for image in ('disc', 'u5'):
            assert re.search(r'original_physics_vehicle_runtime_' + image + r'\s+\.+\s+Passed', output)
            assert output.count('PASS original physics vehicle runtime: 1582 cases, 351662 instructions, 43321 ordered writes, 265655 full-state bytes') == 2
        assert '100% tests passed' in output and 'Skipped' not in output
        if name == 'production':
            assert re.search(r'original_source_vehicle_control\s+\.+\s+Passed', output)
            directory = ROOT / 'build-vehicle-control-20261011'
        else:
            directory = E / f'build-{name}'
        binaries[name] = sha(directory / 'physics_vehicle_runtime_tests')
        ninja = (directory / 'build.ninja').read_text()
        block = ninja.split('CMakeFiles/halo2_physics_vehicle_runtime.dir/src/physics_vehicle_runtime_generated.cpp.o:')[1].split('\n\n')[0]
        for flag in ('-fno-fast-math', '-ffp-contract=off', '-frounding-math'):
            assert flag in block
        if name == 'sanitizer':
            assert '-fsanitize=address,undefined' in block
    result = dict(schema=1, validator_sha256=sha(__file__), source_manifest_sha256=sha(manifest_path),
        correspondence_sha256=sha(correspondence), generated_manifest_sha256=sha(generated),
        original_report_sha256=sha(original), compiled_paired_bodies=10, compiled_instructions=1526,
        exercised_instructions=832, coverage=coverage, original_cases_per_image=1582,
        complete_cases_per_image=1033, required_owner_prefix_cases_per_image=549,
        native_comparisons_per_build=3164, native_instructions_per_build=703324,
        ordered_writes_per_build=86642, full_state_bytes_per_build=531310,
        original_correspondence_and_generation_repeats_identical=True,
        production_tests_passed=True, frozen_release_tests_passed=True,
        frozen_asan_ubsan_leak_tests_passed=True, logs=logs, binaries=binaries,
        instruction_patches=[], original_code_executed_by_production=False,
        live_vehicle_driving_integrated=False, full_physics_parity=False,
        limits=['Declared raw bodies/controls, not source-created live vehicle scenarios',
                'Missing object, vehicle and CRT power owners preserve their actual prefixes',
                '832 exercised instructions do not prove all 1526 compiled instructions',
                'Masked FP exceptions; exception/status flags and hardware equivalence unproven',
                'GCC/Clang x86 backend; Windows/macOS and Xbox hardware unverified',
                'Live player/vehicle/object, all-map, split-screen and LAN integration remains'])
    path = E / 'validation.json'
    path.write_text(json.dumps(result, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(native_comparisons_per_build=3164, exercised_instructions=832,
                          validation_sha256=sha(path))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--freeze', action='store_true')
    args = parser.parse_args()
    freeze() if args.freeze else validate()
