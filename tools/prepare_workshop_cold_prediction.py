"""Stage native cold prediction with a complete coherent Cache provider cohort.

Copy headers, prep tools and exact libraries from the active immutable resource
cohort once its first linked consumer is complete. Compile only the new complete
native prediction/cache/I/O consumer; never rebuild the full engine or mix Cache
ABIs. --session keeps this owned small compiler project available for diagnosis.
Run through run_test.py; all copies and prepared maps remain under its owner.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import sys
import time


def pin(path):
    with path.open('rb') as stream:
        return dict(bytes=path.stat().st_size, sha256=hashlib.file_digest(stream, 'sha256').hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--provider', type=Path, required=True)
    parser.add_argument('--base-cache', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--session', action='store_true')
    args = parser.parse_args()
    if args.session and not sys.stdin.isatty():
        raise ValueError('--session requires an interactive terminal for the finite coordinator')
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.output.resolve().is_relative_to(owner)
    args.output.mkdir(parents=True, exist_ok=False)
    provider_pin = pin(args.provider)
    provider = json.loads(args.provider.read_text())
    assert provider['copied_source_matches_complete_pre_copy_inventory']
    source = Path(provider['source']).resolve(strict=True)
    assert source.is_relative_to(root / '.local/test-scratch')
    parent = source.parent
    builds = list((parent / 'compiler').glob('halo2-test-*/build'))
    assert len(builds) == 1
    build = builds[0]
    project = owner / 'project'
    project.mkdir()
    source_pins = {}
    # Stage all public/private headers from the SAME source as Cache's build.
    # No current Root Cache or sound-plan header enters this provider cohort.
    for folder in ('include', 'src'):
        for path in sorted((source / folder).rglob('*')):
            if not path.is_file() or (folder == 'src' and path.suffix not in ('.hpp', '.h', '.def', '.inc')):
                continue
            name = path.relative_to(source).as_posix()
            destination = project / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            source_pins[name] = pin(path)
            shutil.copyfile(path, destination)
            assert pin(path) == pin(destination) == source_pins[name]
    sources = ['src/campaign/scripting/source_workshop_script_builtins_tu15_generated.cpp',
               'src/audio/native_tu15_sound_kernels_resources.cpp',
               'src/audio/source_sound_cache_io_tu15.cpp',
               'src/core/native_tu15_data_array_allocate_specific.cpp',
               'src/core/native_tu15_data_array_allocate.cpp',
               'src/core/native_tu15_data_array_delete.cpp',
               'src/core/native_tu15_page_allocator.cpp']
    for name in sources:
        destination = project / name
        source_pins[name] = pin(source / name)
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source / name, destination)
        assert pin(source / name) == pin(destination) == source_pins[name]
    shutil.copytree(source / 'tools', project / 'tools',
                    ignore=shutil.ignore_patterns('__pycache__', '.git', 'target'))
    (project / 'cmake').mkdir()
    shutil.copyfile(source / 'cmake/TestStorage.cmake', project / 'cmake/TestStorage.cmake')
    shutil.copytree(source / 'data', project / 'data')
    # The new consumer adds no Cache ABI; it composes the explicitly copied
    # native owners using the existing public resource interfaces.
    root_pins = {}
    for name in ['tests/native_workshop_prediction_delivery_tests.cpp',
                 'tests/workshop_prediction_cold_sound.hpp',
                 'tools/validate_workshop_prediction_content.py']:
        destination = project / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        root_pins[name] = pin(root / name)
        shutil.copyfile(root / name, destination)
        assert pin(root / name) == pin(destination) == root_pins[name]
    (args.output / 'provider-source-files.json').write_text(json.dumps(source_pins, indent=2) + '\n')
    report = dict(scope=__doc__, status='waiting_for_provider_link', passed=False,
        provider=str(args.provider), provider_record_before_link=provider_pin,
        provider_source_inventory_sha256=provider['source_inventory_sha256'],
        provider_source=str(source), project=str(project), Root_consumer_sources=root_pins,
        owned_scratch=str(owner), retained_large_artifacts=[])
    report_path = args.output / 'preparation.json'
    report_path.write_text(json.dumps(report, indent=2) + '\n')
    # Building the next requested target proves that the first native consumer
    # linked successfully. An archive pathname alone can appear while ar writes.
    sentinel = build / 'CMakeFiles/workshop_prediction_raw_bitmap_tests.dir/tests/workshop_prediction_raw_bitmap_tests.cpp.o'
    deadline = time.monotonic() + 1800
    while not (sentinel.is_file() and (build / 'workshop_prediction_delivery_tests').is_file()):
        current = json.loads(args.provider.read_text())
        assert current['status'] == 'running' and source.is_dir(), 'Provider terminated before a complete link'
        assert time.monotonic() < deadline, 'Provider link wait exceeded its declared finite window'
        time.sleep(1)
    assert (build / 'workshop_prediction_delivery_tests').is_file()
    link = build / 'CMakeFiles/workshop_prediction_delivery_tests.dir/link.txt'
    link_pin = pin(link)
    words = shlex.split(link.read_text())
    words = words[words.index('-o') + 2:]
    libraries, links = {}, []
    (project / 'libraries').mkdir()
    for word in words:
        if word.endswith('.a') or ('.so' in word and not word.startswith('-')):
            original = Path(word)
            original = (original if original.is_absolute() else build / original).resolve(strict=True)
            original_pin = pin(original)
            if original.suffix == '.a':
                destination = project / 'libraries' / original.name
                shutil.copyfile(original, destination)
                assert pin(original) == pin(destination) == original_pin
                links.append(str(destination))
            else:
                links.append(str(original))
            libraries[str(original)] = original_pin
        else:
            assert word.startswith('-')
            links.append(word)
    assert str(build / 'libhalo2_cache.a') in libraries
    assert pin(link) == link_pin
    assert all(pin(project / name) == value for name, value in source_pins.items())
    assert all(pin(project / name) == value for name, value in root_pins.items())
    cmake = '''cmake_minimum_required(VERSION 3.25)
project(WorkshopColdPrediction LANGUAGES CXX)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 REQUIRED)
include(cmake/TestStorage.cmake)
add_executable(cold_prediction tests/native_workshop_prediction_delivery_tests.cpp
''' + '\n'.join(sources) + ''')
target_compile_features(cold_prediction PRIVATE cxx_std_20)
target_compile_definitions(cold_prediction PRIVATE HALO2_WORKSHOP_PREDICTION_COLD_CACHE=1)
target_compile_options(cold_prediction PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_include_directories(cold_prediction PRIVATE include src tests)
target_link_libraries(cold_prediction PRIVATE nlohmann_json::nlohmann_json SDL2::SDL2
''' + '\n'.join(json.dumps(word) for word in links) + ''')
add_test(NAME native_E3_cold_prediction COMMAND ${Python3_EXECUTABLE} -B
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_workshop_prediction_content.py
    --test-executable $<TARGET_FILE:cold_prediction>
    --base-cache ''' + json.dumps(str(args.base_cache.resolve(strict=True))) + '''
    --output "${CMAKE_CURRENT_BINARY_DIR}/cold-prediction.json")
set_tests_properties(native_E3_cold_prediction PROPERTIES TIMEOUT 900)
'''
    (project / 'CMakeLists.txt').write_text(cmake)
    report.update(status='prepared_not_validated', libraries=libraries,
        provider_link=link_pin, copied_native_headers_and_sources=len(source_pins),
        native_HS_executed=False, actual_World_qualified=False, declaration_admission_changed=False,
        complete_mod_support=False)
    report_path.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(status=report['status'], project=str(project), libraries=len(libraries))), flush=True)
    if args.session:
        os.execv(sys.executable, [sys.executable, '-u', '-B', str(root / 'tools/macos/test_session.py'),
                                '--output', str(args.output / 'session')])


if __name__ == '__main__':
    main()
