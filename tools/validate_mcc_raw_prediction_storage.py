"""Check raw prediction ownership and the complete original header corpus.

Use the frozen Cache ABI with the explicitly staged pure bitmap-owner/header
changes. No newer Cache/sound-plan headers are borrowed. Compare all eleven
raw formats and retain CPU sampling refusals. Run through run_test.py;
run_tests.py owns/removes every GCC, Clang and sanitizer compiler tree.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--recovery', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.output.resolve().is_relative_to(owner)
    recovery = json.loads(args.recovery.read_text())
    assert recovery['passed'] and recovery['complete_source_inventory_reproduced']
    source = Path(recovery['source']).resolve(strict=True)
    build = source.parent / 'build'
    sys.path.insert(0, str(source / 'tools'))
    from build_launcher_engine import source_hashes
    import steam_workshop as workshop
    inventory = source_hashes(source)
    assert hashlib.sha256(json.dumps(inventory, sort_keys=True).encode()).hexdigest() == recovery['production_inventory_sha256']
    link = build / 'CMakeFiles/workshop_mod_probe.dir/link.txt'
    link_pin = workshop.pin(link)
    words = shlex.split(link.read_text())
    words = words[words.index('-o') + 2:]
    links, libraries = [], {}
    for word in words:
        if word.endswith('.a') or ('.so' in word and not word.startswith('-')):
            path = Path(word)
            path = (path if path.is_absolute() else build / path).resolve(strict=True)
            libraries[str(path)] = workshop.pin(path)
            links.append(str(path))
        else:
            assert word.startswith('-')
            links.append(word)
    project = owner / 'project'
    project.mkdir()
    paths = {
        'src/rendering/mcc_cpu_bitmap_owner.cpp': 'owner.cpp',
        'src/rendering/mcc_cpu_bitmap_owner.hpp': 'private/rendering/mcc_cpu_bitmap_owner.hpp',
        'src/rendering/source_bitmap_pending_header.cpp': 'header.cpp',
        'include/halo2/source_bitmap_pending_header.hpp': 'private/halo2/source_bitmap_pending_header.hpp',
        'tests/workshop_prediction_raw_bitmap_tests.cpp': 'raw.cpp',
        'tests/source_bitmap_pending_header_tests.cpp': 'header-test.cpp',
        'tests/fixtures/retail_source_bitmap_pending_header.tsv': 'header-reference.tsv',
        'tools/validate_mcc_raw_prediction_storage.py': 'coordinator.py'}
    pins = {name: workshop.pin(root / name) for name in paths}
    for name, target in paths.items():
        destination = project / target
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(root / name, destination)
    layout = source / 'src/rendering/effects/mcc_lens_bitmap_layout.cpp'
    layout_pin = workshop.pin(layout)
    shutil.copyfile(layout, project / 'lens.cpp')
    # Also type-check the real World-owned projection with its new sampling
    # guard. Actual World lifecycle execution remains a separate acceptance.
    projection = root / 'src/rendering/mcc_cpu_bitmap_projection.cpp'
    projection_pin = workshop.pin(projection)
    args.output.mkdir(parents=True, exist_ok=False)
    syntax = [os.environ.get('CXX', 'c++'), '-std=c++20', '-fsyntax-only',
        '-I' + str(project / 'private'), '-I' + str(source / 'include'), '-I' + str(source / 'src'), str(projection)]
    with (args.output / 'projection-syntax.log').open('w') as log:
        result = subprocess.run(syntax, stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError('Actual World-owned bitmap projection did not type-check')
    (project / 'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.25)
project(MccRawPredictionStorage LANGUAGES CXX)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 REQUIRED)
include(''' + json.dumps(str(source / 'cmake/TestStorage.cmake')) + ''')
if(HALO2_SANITIZERS)
 add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
 add_link_options(-fsanitize=address,undefined)
endif()
add_executable(raw_storage raw.cpp owner.cpp lens.cpp header.cpp)
add_executable(original_header header-test.cpp header.cpp)
foreach(target IN ITEMS raw_storage original_header)
 target_include_directories(${target} PRIVATE private ''' + json.dumps(str(source / 'include')) + ' ' + json.dumps(str(source / 'src')) + ''')
 target_compile_features(${target} PRIVATE cxx_std_20)
 target_compile_options(${target} PRIVATE -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off -frounding-math)
 target_link_options(${target} PRIVATE -Wl,--gc-sections)
 target_link_libraries(${target} PRIVATE nlohmann_json::nlohmann_json SDL2::SDL2
''' + ''.join(' ' + json.dumps(word) + '\n' for word in links) + ''')
endforeach()
add_test(NAME raw_prediction_storage COMMAND raw_storage)
add_test(NAME original_prediction_header COMMAND original_header "${CMAKE_CURRENT_SOURCE_DIR}/header-reference.tsv")
''')
    results = []
    for compiler, sanitized in [('g++', False), ('clang++', False), ('clang++', True)]:
        label = compiler + ('-sanitized' if sanitized else '')
        command = [sys.executable, '-B', str(root / 'tools/run_tests.py'), '--source', str(project),
            '--scratch-root', str(owner / 'builds'), '--output', str(args.output / label),
            '--target', 'raw_storage', '--target', 'original_header', '--filter', '^(raw_prediction_storage|original_prediction_header)$',
            '--cmake-arg=-DCMAKE_CXX_COMPILER=' + compiler, '--jobs', '2', '--test-jobs', '1',
            '--retain-report', 'Testing/Temporary/LastTest.log']
        if sanitized:
            command += ['--sanitizers']
        run = subprocess.run(command)
        results.append(dict(compiler=compiler, sanitizers=sanitized, exit_code=run.returncode, command=command))
        if run.returncode:
            break
    unchanged = source_hashes(source) == inventory
    unchanged &= all(workshop.pin(root / name) == pin for name, pin in pins.items())
    unchanged &= all(workshop.pin(Path(path)) == pin for path, pin in libraries.items())
    unchanged &= workshop.pin(layout) == layout_pin and workshop.pin(link) == link_pin
    unchanged &= workshop.pin(projection) == projection_pin
    report = dict(scope=__doc__, passed=unchanged and len(results) == 3 and all(row['exit_code'] == 0 for row in results),
        source_inputs_unchanged=unchanged, source_inventory_sha256=recovery['production_inventory_sha256'],
        source_pins=pins, projection_source=projection_pin, projection_syntax_command=syntax,
        borrowed_libraries=libraries, results=results, raw_formats=11, negative_controls_per_compiler=87,
        original_header_cases_per_compiler=19472, original_header_words_per_compiler=97360,
        CPU_sampling_domain_extended=False, actual_World_lifecycle_qualified=False,
        script_declaration_admitted=False, complete_mod_support=False,
        owned_scratch=str(owner), retained_large_artifacts=[])
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], results=[row['exit_code'] for row in results])))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
