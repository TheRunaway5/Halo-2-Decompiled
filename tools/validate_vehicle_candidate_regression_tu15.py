#!/usr/bin/env python3
"""Recheck the full declared native vehicle cohort in a qualified owned build.

Only private test registrations and pinned fixture inputs are added. Restore
the exact incoming CMake bytes on success, failure or cancellation; the outer
run_test.py owner removes all compiler products.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from test_destruction_current_cohort import CASES
from weapon_test_cache import sha

ROOT = Path(__file__).resolve().parents[1]
PRIMARY = ('ghost', 'banshee', 'scorpion', 'wraith', 'warthog-mg',
           'warthog-gauss', 'spectre', 'h_turret_ap', 'c_turret_ap')
OWNERSHIP = ('native_tu15_world_memory_ownership', 'native_tu15_physics_branch_ownership',
             'original_tu15_contrail_snapshot_inputs', 'original_tu15_light_fade_curves',
             'original_vehicle_bv_agent_tu15')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('prepared', 'validation', 'cases', 'output', 'reference-xbe'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--weapon-case', action='append', choices=PRIMARY,
                        help='Run only these firing failures before repeating the complete cohort')
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source = owner / 'source'
    prepared = json.loads(args.prepared.read_text())
    validation = json.loads(args.validation.read_text())
    if (Path(prepared['owner']) != owner or not validation['passed'] or
            validation['vehicle_choice_source_binding']['preparation_sha256'] != sha(args.prepared) or
            not validation['vehicle_choice_source_binding']['source_unchanged_after_run']):
        raise ValueError('Use the same qualified native source/build owner')
    pins = prepared['source_hashes']
    if any(sha(source / name) != pin for name, pin in pins.items()):
        raise ValueError('Qualified source changed before the broader regression')
    original_pin = sha(args.reference_xbe)
    if original_pin != '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd':
        raise ValueError('Original read-only data reference differs')
    cases = json.loads(args.cases.read_text())
    if cases['schema'] != 'H2_VEHICLE_VARIANT_CONSTRUCTOR_CASES_V1' or len(cases['cases']) != 327:
        raise ValueError('The complete 327-context input manifest is required')
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    maps = Path(prepared['maps_directory'])
    cmake = source / 'CMakeLists.txt'
    before = cmake.read_bytes()
    lines = ['\n# VEHICLE_CANDIDATE_REGRESSION_TU15\n',
             'if(NOT TARGET destruction_runtime_tu15_tests)\n',
             'add_executable(destruction_runtime_tu15_tests tests/destruction_runtime_tu15_tests.cpp)\n',
             'target_link_libraries(destruction_runtime_tu15_tests PRIVATE halo2_systems halo2_authority_codec OpenSSL::Crypto)\n',
             'endif()\n']
    reports = []
    inputs = source / 'tests/fixtures/vehicle-variants-tu15'
    inputs.mkdir(parents=True, exist_ok=True)

    def register(name, target, arguments, environment=''):
        quoted = ' '.join('"' + str(value) + '"' for value in arguments)
        lines.append(f'add_test(NAME {name} COMMAND "${{Python3_EXECUTABLE}}" '
                     f'"${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- $<TARGET_FILE:{target}> {quoted})\n')
        lines.append(f'set_tests_properties({name} PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE'
                     + (f' ENVIRONMENT "{environment}"' if environment else '') + ')\n')

    for case in cases['cases']:
        original = args.cases.parent / case['path']
        if sha(original) != case['sha256']:
            raise ValueError('Variant input changed')
        destination = inputs / case['path']
        if destination.exists() and sha(destination) != case['sha256']:
            raise ValueError('Existing private variant input differs')
        if not destination.exists():
            shutil.copyfile(original, destination)
        report = 'vehicle-variant-' + case['case_id'] + '.json'
        reports.append(report)
        register('vehicle_candidate_variant_' + case['case_id'], 'vehicle_variants_world_tu15_tests',
                 [maps / case['map'], maps, destination, '${CMAKE_CURRENT_BINARY_DIR}/' + report])
    for case, map_name in CASES.items():
        report = 'vehicle-destruction-' + case + '.json'
        reports.append(report)
        register('vehicle_candidate_destruction_' + case, 'destruction_runtime_tu15_tests',
                 [maps / (map_name + '.map'), maps, case, '${CMAKE_CURRENT_BINARY_DIR}/' + report],
                 'HALO2_DESTRUCTION_CONTINUATION=1;HALO2_DESTRUCTION_FIXED_CLOCK=1;'
                 'HALO2_DESTRUCTION_REQUIRE_NATIVE_DIFFICULTY=1;HALO2_DESTRUCTION_REQUIRE_NATIVE_MATERIAL=1')
    for kind, names in (('weapon', PRIMARY), ('secondary', ('banshee', 'scorpion')), ('boost', ('ghost',))):
        for name in names:
            report = f'vehicle-{kind}-{name}.json'
            reports.append(report)
            environment = f'HALO2_TEST_VEHICLE_WEAPON_HUD_CASE={name};HALO2_TEST_TURRET_SOUND_TRACE=1;'
            if kind == 'boost':
                environment += 'HALO2_TEST_GHOST_BOOST=1;'
            else:
                environment += 'HALO2_TEST_VEHICLE_WEAPON_FIRE=1;'
                if kind == 'secondary':
                    environment += 'HALO2_TEST_VEHICLE_WEAPON_SECONDARY=1;'
                else:
                    environment += 'HALO2_TEST_TURRET_TARGET_DAMAGE=1;'
            environment += 'HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=${CMAKE_CURRENT_BINARY_DIR}/' + report
            register(f'vehicle_candidate_{kind}_{name}', 'vehicle_weapon_hud_world_tests',
                     [maps / 'coagulation.map', maps, 'native-mcc'], environment)
    selected_reports = ['vehicle-weapon-' + name + '.json' for name in args.weapon_case] if args.weapon_case else reports
    selected_filter = '^(' + '|'.join('vehicle_candidate_weapon_' + re.escape(name) for name in args.weapon_case) + ')$' if args.weapon_case else '^(vehicle_candidate_|' + '|'.join(name + '$' for name in OWNERSHIP) + ')'
    command = [sys.executable, '-B', str(ROOT / 'tools/run_tests.py'), '--build-worker',
               '--source', str(source), '--output', str(args.output), '--jobs', '2', '--test-jobs', '1',
               '--filter', selected_filter,
               '--cmake-arg=-DCMAKE_CXX_FLAGS_RELEASE=-O2 -DNDEBUG',
               '--cmake-arg=-DFETCHCONTENT_SOURCE_DIR_HALO2_DATACHANNEL=' + prepared['peer_copy'],
               '--cmake-arg=-DHALO2_TU15_CONTRAIL_XBE=' + str(args.reference_xbe.resolve())]
    for target in ('destruction_runtime_tu15_tests', 'vehicle_weapon_hud_world_tests',
                   'vehicle_variants_world_tu15_tests', 'native_tu15_world_memory_tests',
                   'native_tu15_physics_branch_ownership_tests', 'source_contrail_snapshot_tests',
                   'light_fade_curves_tu15_tests', 'vehicle_bv_agent_tu15_tests'):
        command += ['--target', target]
    for report in selected_reports + ([] if args.weapon_case else ['vehicle-bv-agent-original.json']) + ['Testing/Temporary/LastTest.log']:
        command += ['--retain-report', report]
    binding = dict(owner=str(owner), expected_cases=len(args.weapon_case) if args.weapon_case else len(reports) + len(OWNERSHIP),
                   complete_declared_cohort=not bool(args.weapon_case),
                   prepared_sha256=sha(args.prepared), validation_sha256=sha(args.validation),
                   cases_sha256=sha(args.cases), tool_sha256=sha(Path(__file__)),
                   native_runtime_opcode_execution=False, full_vehicle_parity=False,
                   original_reference_sha256=original_pin, production_bodies_changed=False)
    (args.output / 'source-binding.json').write_text(json.dumps(binding, indent=2) + '\n')
    try:
        cmake.write_bytes(before + ''.join(lines).encode())
        test_pins = dict(pins, **{'CMakeLists.txt': sha(cmake)})
        print(json.dumps(dict(status='running', cases=binding['expected_cases'])), flush=True)
        result = subprocess.run(command, env=dict(os.environ, CARGO_BUILD_JOBS='2'),
                                stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT).returncode
        binding['source_unchanged_after_run'] = all(sha(source / name) == pin for name, pin in test_pins.items())
    finally:
        cmake.write_bytes(before)
        binding['incoming_CMake_restored'] = cmake.read_bytes() == before
        (args.output / 'source-binding.json').write_text(json.dumps(binding, indent=2) + '\n')
    path = args.output / 'report.json'
    report = json.loads(path.read_text())
    log = (args.output / 'tests.log').read_text() if (args.output / 'tests.log').exists() else ''
    passed = re.search(r'^100% tests passed(?:, 0 tests failed)? out of (\d+)\s*$', log, re.MULTILINE)
    binding['all_declared_cases_passed_without_skips'] = bool(
        passed and int(passed[1]) == binding['expected_cases'] and 'Skipped' not in log)
    retained={row['path']:row for row in report.get('retained_small_reports',[])}
    expected_reports=selected_reports+([] if args.weapon_case else ['vehicle-bv-agent-original.json'])
    rejected=[]
    for name in expected_reports:
        row=retained.get(name)
        if not row or row['status']!='retained':
            rejected.append(name);continue
        retained_path=Path(row['retained_path'])
        if sha(retained_path)!=row['sha256'] or json.loads(retained_path.read_text()).get('passed')is not True:
            rejected.append(name)
    binding['every_declared_source_report_retained_and_passed']=not rejected
    binding['rejected_source_reports']=rejected
    report['candidate_vehicle_regression'] = binding
    report['passed'] &= result == 0 and binding['source_unchanged_after_run'] and binding['all_declared_cases_passed_without_skips'] and not rejected
    path.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], report=str(path))), flush=True)
    return 0 if report['passed'] else result or 1


if __name__ == '__main__':
    raise SystemExit(main())
