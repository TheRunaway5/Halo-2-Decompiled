"""Qualify native tiling data conversion in owned compiler/sanitizer builds.

Uses the required literal MCC/original fixture and unchanged original packing
results. Selected runtimes, frozen engine source and assets stay unchanged.
This is component execution and complete guard coverage, not E3 GPU acceptance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--preparation', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--compiler', action='append', choices=['g++', 'clang++'])
    parser.add_argument('--sanitizers', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.output.resolve().is_relative_to(owner)
    preparation = json.loads(args.preparation.read_text())
    source = Path(preparation['source']).resolve(strict=True)
    from build_launcher_engine import source_hashes
    inventory = source_hashes(source)
    assert hashlib.sha256(json.dumps(inventory, sort_keys=True).encode()).hexdigest() == preparation['production_inventory_sha256']
    selectors = {name: (root / name).read_bytes() for name in ('gameplay-runtime-selection.json', 'frontend-runtime-selection.json')}
    runtime = root / preparation['selected_runtime']
    assert digest(runtime) == preparation['runtime_sha256']
    args.output.mkdir(parents=True, exist_ok=True)
    base_files = ['src/assets/cache.cpp', 'cmake/TestStorage.cmake', 'tools/run_test.py']
    base_files += [path.relative_to(source).as_posix() for path in (source / 'include').rglob('*') if path.is_file()]
    overlays = ['src/rendering/mcc_native_tiling_specular.cpp', 'src/rendering/mcc_native_tiling_specular_profiles.inc',
        'include/halo2/mcc_native_tiling_specular.hpp', 'tools/generate_mcc_native_tiling_specular.py',
        'tools/data/mcc-native-tiling-specular-profiles.json', 'tests/mcc_native_tiling_specular_tests.cpp',
        'tests/mcc_native_tiling_specular_tests.py', 'tests/fixtures/mcc-native-tiling-specular-v1.json',
        'tests/fixtures/mcc-native-tiling-specular-v1.json.zlib']
    project = owner / 'project'
    pins = {}
    for origin, names in ((source, base_files), (root, overlays)):
        for name in names:
            target = project / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(origin / name, target)
            pins[str(origin / name)] = digest(origin / name)
    (project / 'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.25)
project(MccTilingSpecular LANGUAGES CXX)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
include(cmake/TestStorage.cmake)
add_executable(tiling_tests tests/mcc_native_tiling_specular_tests.cpp
 src/rendering/mcc_native_tiling_specular.cpp src/assets/cache.cpp)
target_include_directories(tiling_tests PRIVATE include src)
target_compile_features(tiling_tests PRIVATE cxx_std_20)
target_compile_definitions(tiling_tests PRIVATE HALO2_CACHE_BYTES_ONLY=1)
target_link_libraries(tiling_tests PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
if(HALO2_SANITIZERS)
 target_compile_options(tiling_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
 target_link_options(tiling_tests PRIVATE -fsanitize=address,undefined)
endif()
add_test(NAME mcc_native_tiling_specular COMMAND ${Python3_EXECUTABLE} -B
 "${CMAKE_CURRENT_SOURCE_DIR}/tests/mcc_native_tiling_specular_tests.py"
 --repo-root "${CMAKE_CURRENT_SOURCE_DIR}" --test-executable $<TARGET_FILE:tiling_tests>
 --report-output "${CMAKE_CURRENT_BINARY_DIR}/tiling.json")
set_tests_properties(mcc_native_tiling_specular PROPERTIES TIMEOUT 180)
add_test(NAME mcc_native_tiling_specular_generator COMMAND ${Python3_EXECUTABLE} -B
 "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_native_tiling_specular.py" --check)
''')
    matrix = [(compiler, args.sanitizers) for compiler in args.compiler] if args.compiler else [('g++', False), ('clang++', False), ('clang++', True)]
    results = []
    for compiler, sanitized in matrix:
        name = compiler + ('-sanitizers' if sanitized else '')
        command = [sys.executable, '-B', str(root / 'tools/run_tests.py'), '--source', str(project),
            '--scratch-root', str(owner / 'builds'), '--output', str(args.output / name), '--target', 'tiling_tests',
            '--filter', '^mcc_native_tiling_specular(_generator)?$', '--jobs', '2', '--test-jobs', '1',
            '--cmake-arg=-DCMAKE_CXX_COMPILER=' + compiler,
            '--retain-report', 'tiling.json', '--retain-report', 'Testing/Temporary/LastTest.log']
        if sanitized:
            command.append('--sanitizers')
        completed = subprocess.run(command)
        results.append(dict(compiler=compiler, sanitizers=sanitized, exit_code=completed.returncode))
        if completed.returncode:
            break
    assert source_hashes(source) == inventory
    assert all(digest(Path(path)) == pin for path, pin in pins.items())
    assert all((root / name).read_bytes() == raw for name, raw in selectors.items())
    assert digest(runtime) == preparation['runtime_sha256']
    report = dict(scope=__doc__, passed=all(item['exit_code'] == 0 for item in results), results=results,
        source_pins=pins, helper_sha256=digest(Path(__file__)), selected_runtime_unchanged=True,
        frozen_source_unchanged=True, source_overlays_applied=False, owned_scratch=str(owner), retained_large_artifacts=[])
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
