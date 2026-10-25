#!/usr/bin/env python3
"""Compare production native vehicle rate kernels with untouched offline originals.

Run through tools/run_test.py using the repository's oracle-capable Python.
Raw oracle banks and all compiler outputs remain under HALO2_TEST_SCRATCH.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

import oracle_vehicle_runtime_20261011 as oracle


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--negative-control', action='store_true',
                        help='Require the comparison to reject a private native arithmetic mutation')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    source = owner / 'native-rates-source'
    source.mkdir(exist_ok=False)
    args.output.mkdir(parents=True, exist_ok=False)
    paths = list((root / 'include').rglob('*.hpp')) + list((root / 'src').rglob('*.hpp'))
    paths += [root / 'src/physics/vehicles/physics_vehicle_runtime_generated.cpp',
              root / 'tests/physics_vehicle_runtime_tests.cpp']
    pins = {}
    for path in paths:
        name = path.relative_to(root)
        target = source / name
        target.parent.mkdir(parents=True, exist_ok=True)
        before = sha(path)
        shutil.copyfile(path, target)
        if sha(target) != before or sha(path) != before:
            raise RuntimeError('Native rate source changed while freezing: ' + str(name))
        pins[str(name)] = before
    save(args.output / 'source-hashes.json', pins)
    mutation = None
    if args.negative_control:
        native = source / 'src/physics/vehicles/source_vehicle_rate_native_tu15_internal.hpp'
        text = native.read_text()
        needle = 's.xmm[0]=_mm_add_ss(s.xmm[0],deceleration);'
        if text.count(needle) != 1:
            raise RuntimeError('Requalify the isolated native rate negative control')
        before = sha(native)
        native.write_text(text.replace(needle, 's.xmm[0]=_mm_sub_ss(s.xmm[0],deceleration);'))
        mutation = dict(path=str(native.relative_to(source)), before=before, after=sha(native),
                        scope='Private arithmetic mutation only; original oracle and instruction path unchanged')
        pins[str(native.relative_to(source))] = sha(native)
    paired_path = root / 'analysis/multiplayer/sandbox-source/vehicle-runtime-20261011/correspondence-verified/report.json'
    paired = json.loads(paired_path.read_text())
    records = oracle.definitions()
    rows = oracle.controls(records)
    # Add original scalar callers across every declared FP precision/rounding
    # and non-empty x87 stack placement. No expected native output is supplied.
    import itertools
    scalar_rows = [row for row in rows if row.get('scalar_control')]
    for entry, precision, rounding, top, depth in itertools.product(
            (0x208d80, 0x208e70), (0, 0x200, 0x300), range(4), range(8), (0, 1, 3)):
        row = dict(scalar_rows[0])
        row.update(entry=entry, direction=-1., turn=.25, target=-.125,
                   fp_control=0x7f | precision | (rounding << 10),
                   mxcsr=0x1f80 | (rounding << 13), top=top, depth=depth)
        rows.append(row)
    original = owner / 'native-rates-original'
    original.mkdir()
    stats = {}
    for name, latest, path in (
            ('disc', False, root / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),
            ('u5', True, oracle.NEW_DEFAULT)):
        print('Offline untouched original vehicle rate comparison:', name, flush=True)
        cases = oracle.execute(path, paired, latest, records, rows)
        one_address=0x46a0d0 if latest else 0x465030
        image=oracle.Xbe(path)
        fixture = dict(schema=1, image='1.5' if latest else '1.1', cases=cases,
                       native_cursor_immutable_inputs=[dict(address=one_address,hex=image.read(one_address,4).hex())],
                       native_cursor_site_bindings=[dict(key=int(p,16),value=q if latest else int(p,16))
                           for p,q in sorted(paired['instruction_map'].items())
                           if 0x208d80<=int(p,16)<0x208ed0])
        output = original / (name + '.json')
        output.write_text(json.dumps(fixture, separators=(',', ':')) + '\n')
        stats[name] = dict(original_sha256=sha(path), fixture_sha256=sha(output),
                           cases=len(cases), complete=sum(c['complete'] for c in cases),
                           ordered_stores=sum(len(c['writes']) for c in cases),
                           full_bank_bytes=sum(len(bytes.fromhex(b['hex'])) for c in cases for b in c['final_banks']))
    report = dict(passed=False, scratch_owner=str(owner), original=stats,
                  correspondence_sha256=sha(paired_path),
                  oracle_sha256=sha(Path(oracle.__file__)), tool_sha256=sha(Path(__file__)),
                  authored_inputs=records, instruction_patches=[],
                  native_expected_data_used=False, in_game_or_full_vehicle_parity=False,
                  large_artifacts_retained=[], negative_control=mutation)
    save(args.output / 'report.json', report)
    cmake = '''cmake_minimum_required(VERSION 3.20)
project(Halo2VehicleRateNative LANGUAGES CXX)
enable_testing()
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_executable(physics_vehicle_runtime_tests tests/physics_vehicle_runtime_tests.cpp
 src/physics/vehicles/physics_vehicle_runtime_generated.cpp)
target_include_directories(physics_vehicle_runtime_tests PRIVATE include src)
target_compile_features(physics_vehicle_runtime_tests PRIVATE cxx_std_20)
target_compile_options(physics_vehicle_runtime_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(physics_vehicle_runtime_tests PRIVATE nlohmann_json::nlohmann_json)
'''
    for name in stats:
        cmake += (f'add_test(NAME native_vehicle_rates_{name} COMMAND "${{Python3_EXECUTABLE}}" '
                  f'"{root / "tools/run_test.py"}" -- $<TARGET_FILE:physics_vehicle_runtime_tests> '
                  f'"{original / (name + ".json")}")\n')
    (source / 'CMakeLists.txt').write_text(cmake)
    command = [sys.executable, '-B', str(root / 'tools/run_tests.py'), '--build-worker',
               '--source', str(source), '--output', str(args.output / 'validation'),
               '--target', 'physics_vehicle_runtime_tests', '--filter', '^native_vehicle_rates_',
               '--jobs', str(args.jobs), '--test-jobs', '1',
               '--retain-report', 'Testing/Temporary/LastTest.log']
    result = subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    if args.negative_control:
        validation = json.loads((args.output / 'validation/report.json').read_text())
        tests = (args.output / 'validation/tests.log').read_text() if (args.output / 'validation/tests.log').exists() else ''
        report['comparison_rejected_mutation'] = result.returncode != 0 and all(
            step['exit_code'] == 0 for step in validation['steps'] if step['name'] != 'tests') and (
                'native kernel changed stores or full memory' in tests)
        report['passed'] = report['comparison_rejected_mutation']
    else:
        report['passed'] = result.returncode == 0
    report['validation_return_code'] = result.returncode
    report['frozen_source_unchanged'] = all(sha(source / n) == h for n, h in pins.items())
    save(args.output / 'report.json', report)
    print(json.dumps(dict(passed=report['passed'], report=str(args.output / 'report.json'))), flush=True)
    raise SystemExit(0 if report['passed'] and report['frozen_source_unchanged'] else 1)


if __name__ == '__main__':
    main()
