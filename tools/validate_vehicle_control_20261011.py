#!/usr/bin/env python3
"""Freeze and verify only the owned vehicle scalar component and its evidence.

Unrelated shared-checkout edits are excluded. The same production CMake module
is reused in the standalone cohort; ignored analysis inputs are linked rather
than copied into the source snapshot. No live driving parity is inferred.
"""
import argparse
import hashlib
import json
import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
E = ROOT / 'analysis/multiplayer/sandbox-source/vehicle-control-20261011'
OWNED = (
    'src/gameplay/vehicles/source_vehicle_control.cpp', 'include/halo2/source_vehicle_control.hpp',
    'tests/source_vehicle_control_tests.cpp', 'cmake/VehicleControl.cmake',
    'tools/research_vehicle_control_20261011.py',
    'tools/oracle_vehicle_control_20261011.py',
    'tools/validate_vehicle_control_20261011.py',
    'docs/PHYSICS_PLAYERS_VEHICLES_OBJECTS_GOAL.md',
)
SUPPORT = (
    'include/halo2/physics_body_factory.hpp', 'include/halo2/physics_body_state.hpp',
    'include/halo2/physics_contact_lifecycle.hpp', 'include/halo2/physics_contact_record.hpp',
)
TOP = '''cmake_minimum_required(VERSION 3.20)
project(halo2_vehicle_control_cohort LANGUAGES CXX)
include(CTest)
option(HALO2_SANITIZERS "Instrument the complete component" OFF)
find_package(nlohmann_json CONFIG REQUIRED)
include(cmake/VehicleControl.cmake)
'''


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def freeze():
    destination = E / 'source-v1'
    assert not destination.exists()
    assert 'include(cmake/VehicleControl.cmake)' in (ROOT / 'CMakeLists.txt').read_text()
    files = {relative: sha(ROOT / relative) for relative in OWNED + SUPPORT}
    destination.mkdir(parents=True)
    for relative, pin in files.items():
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / relative, target)
        assert sha(target) == pin == sha(ROOT / relative)
    (destination / 'CMakeLists.txt').write_text(TOP)
    (destination / 'analysis').symlink_to(ROOT / 'analysis', target_is_directory=True)
    manifest = dict(schema=1, owned=list(OWNED), support=list(SUPPORT), files=files,
                    standalone_top_sha256=sha(destination / 'CMakeLists.txt'),
                    production_registration='include(cmake/VehicleControl.cmake)',
                    analysis_inputs_copied=False)
    path = E / 'source-manifest-v1.json'
    path.write_text(json.dumps(manifest, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(source_manifest_sha256=sha(path), frozen_files=len(files))))


def validate():
    source = E / 'source-v1'
    manifest_path = E / 'source-manifest-v1.json'
    manifest = json.loads(manifest_path.read_text())
    assert manifest['owned'] == list(OWNED) and manifest['support'] == list(SUPPORT)
    for relative, pin in manifest['files'].items():
        assert sha(source / relative) == pin == sha(ROOT / relative), relative
    assert sha(source / 'CMakeLists.txt') == manifest['standalone_top_sha256']
    assert (source / 'CMakeLists.txt').read_text() == TOP
    assert (source / 'analysis').resolve() == ROOT / 'analysis'
    assert manifest['production_registration'] in (ROOT / 'CMakeLists.txt').read_text()
    correspondence = E / 'correspondence-verified/report.json'
    assert sha(correspondence) == sha(E / 'correspondence-repeat/report.json')
    paired = json.loads(correspondence.read_text())
    assert paired['tool_sha256'] == sha(ROOT / 'tools/research_vehicle_control_20261011.py')
    assert paired['complete_paired_bodies'] == 6 and paired['native_instruction_count'] == 94
    for row in paired['routines']:
        for label in ('old', 'later'):
            listing = row[label + '_listing']
            assert sha(correspondence.parent / 'instructions' / listing['file']) == listing['sha256']
    original = E / 'original-verified/report.json'
    assert sha(original) == sha(E / 'original-repeat/report.json')
    report = json.loads(original.read_text())
    assert report['tool_sha256'] == sha(ROOT / 'tools/oracle_vehicle_control_20261011.py')
    assert report['correspondence_sha256'] == sha(correspondence)
    assert report['fixture_sha256'] == sha(E / 'original-verified/fixture.json') == sha(E / 'original-repeat/fixture.json')
    assert report['cases_per_image'] == 33696 and report['covered_native_instructions'] == 94
    assert report['ordered_stores_per_image'] == 59412 and report['full_output_bytes_per_image'] == 2156544
    assert not report['source_instruction_patches'] and not report['native_expected_data_used']
    logs = {}
    binaries = {}
    for name in ('release', 'sanitizer'):
        for stage in ('configure', 'build', 'test'):
            path = E / f'{name}-{stage}.log'
            logs[path.name] = sha(path)
        output = (E / f'{name}-test.log').read_text()
        assert re.search(r'original_source_vehicle_control\s+\.+\s+Passed', output)
        assert '100% tests passed' in output and 'Skipped' not in output
        directory = E / f'build-{name}'
        binaries[name] = sha(directory / 'source_vehicle_control_tests')
        ninja = (directory / 'build.ninja').read_text()
        block = ninja.split('CMakeFiles/halo2_vehicle_control.dir/src/source_vehicle_control.cpp.o:')[1].split('\n\n')[0]
        for flag in ('-fno-fast-math', '-ffp-contract=off', '-frounding-math'):
            assert flag in block
        if name == 'sanitizer':
            assert '-fsanitize=address,undefined' in block
    production = (E / 'production-test.log').read_text()
    assert '100% tests passed' in production and 'Skipped' not in production
    for stage in ('configure', 'build', 'test'):
        logs['production-' + stage] = sha(E / f'production-{stage}.log')
    result = dict(schema=1, validator_sha256=sha(__file__), source_manifest_sha256=sha(manifest_path),
        correspondence_sha256=sha(correspondence), original_report_sha256=sha(original),
        complete_paired_bodies=6, native_bodies=2, native_instructions=94,
        original_cases_per_image=33696, native_comparisons_per_build=67392,
        ordered_stores_per_build=118824, full_output_bytes_per_build=4313088,
        original_and_correspondence_repeats_identical=True,
        production_root_test_passed=True, frozen_release_test_passed=True,
        frozen_asan_ubsan_leak_test_passed=True, logs=logs, binaries=binaries,
        instruction_patches=[], original_code_executed_by_production=False,
        live_vehicle_driving_integrated=False, full_physics_parity=False,
        limits=['Declared scalar inputs, not driving scenarios',
                'Semantic output/store/result contract excludes caller scratch/registers/flags/x87 stack',
                'GCC/Clang x86 masked-exception backend; native Windows/macOS and Xbox hardware unverified',
                'Complete driving controllers, motion/body force delivery, seats and actual contacts remain',
                'Whole player/vehicle/object physics and composed all-map split-screen/LAN validation remain'])
    path = E / 'validation.json'
    path.write_text(json.dumps(result, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(native_comparisons_per_build=67392, ordered_stores_per_build=118824,
                          validation_sha256=sha(path))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--freeze', action='store_true')
    args = parser.parse_args()
    freeze() if args.freeze else validate()
