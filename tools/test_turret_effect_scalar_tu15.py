#!/usr/bin/env python3
"""Run the production turret effect converter and event-RNG regression.

This is a component check. MCC Cache and ordinary turret firing are separate.
Use run_test.py; compile only these owners through the official CMake runner.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from weapon_test_cache import sha

ROOT = Path(__file__).resolve().parents[1]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        ap.error('Use tools/run_test.py')
    owner = Path(os.environ['HALO2_TEST_SCRATCH'])
    source = owner / 'component-source'
    source.mkdir()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    inputs = {}
    shutil.copytree(ROOT / 'include', source / 'include')
    for path in (source / 'include').rglob('*'):
        if path.is_file():
            inputs[str(path.relative_to(source))] = sha(path)
    paths = ['src/assets/cache.cpp', 'src/assets/mcc/mcc_gameplay_scalar.cpp',
             'src/assets/mcc/mcc_gameplay_scalar_damage_profiles.inc',
             'src/assets/mcc/mcc_destruction_effect_scalar_profiles.inc',
             'tests/mcc_destruction_effect_scalar_tests.cpp',
             'tools/mcc-turret-effect-scalar-profiles.json']
    for name in paths:
        target = source / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / name, target)
        inputs[name] = sha(target)
    (source / 'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.25)
project(turret_effect_component LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json REQUIRED)
add_executable(turret_effect_tests tests/mcc_destruction_effect_scalar_tests.cpp
    src/assets/mcc/mcc_gameplay_scalar.cpp src/assets/cache.cpp)
target_compile_features(turret_effect_tests PRIVATE cxx_std_20)
target_compile_options(turret_effect_tests PRIVATE -ffunction-sections -fdata-sections)
target_link_options(turret_effect_tests PRIVATE -Wl,--gc-sections)
target_include_directories(turret_effect_tests PRIVATE include src)
target_link_libraries(turret_effect_tests PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME turret_effect_scalar COMMAND turret_effect_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/mcc-turret-effect-scalar-profiles.json")
''')
    command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--source', str(source),
               '--target', 'turret_effect_tests', '--filter', '^turret_effect_scalar$',
               '--jobs', '2', '--output', str(args.output / 'validation')]
    report = {'passed': False, 'source_sha256': inputs, 'command': command,
              'scratch': str(owner), 'cache_or_runtime_acceptance': False,
              'large_artifacts_retained': []}
    code = subprocess.run(command, check=False).returncode
    report.update(passed=code == 0, returncode=code)
    (args.output / 'component.json').write_text(json.dumps(report, indent=2) + '\n')
    return code


if __name__ == '__main__':
    raise SystemExit(main())
