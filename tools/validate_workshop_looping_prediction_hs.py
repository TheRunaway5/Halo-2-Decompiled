"""Compare complete native HS/prediction/cache composition with original code.

Compile the same complete campaign/AI providers used by the existing standalone
actor-selector test, including every unchanged AI body via the official native
partitioner. Join the explicitly pinned current Workshop and cache/core owners.
No HS routine is reduced, emulated or replaced with a supplied result. The
explicit cached-state fixtures do not qualify World/MCC/cold streaming/mission
execution. Run through run_test.py; run_tests.py owns every compiler tree.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import runpy
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
    project = owner / 'project'
    project.mkdir()
    args.output.mkdir(parents=True, exist_ok=False)
    root_files = {
        'tests/workshop_looping_prediction_hs_tests.cpp': 'test.cpp',
        'src/campaign/scripting/source_workshop_script_builtins_tu15_generated.cpp': 'workshop.cpp',
        'src/audio/native_tu15_sound_kernels_resources.cpp': 'cache.cpp',
        'tests/workshop_unit_controls_tests.py': 'reference.py',
        'tools/oracle_workshop_looping_prediction_hs_tu15.py': 'oracle.py',
        'tools/validate_workshop_looping_prediction_hs.py': 'coordinator.py'}
    for stem in ('native_tu15_data_array_allocate_specific', 'native_tu15_data_array_allocate',
                 'native_tu15_data_array_delete', 'native_tu15_page_allocator'):
        root_files['src/core/' + stem + '.cpp'] = stem + '.cpp'
        root_files['include/halo2/' + stem + '.hpp'] = 'private/halo2/' + stem + '.hpp'
    root_pins = {name: workshop.pin(root / name) for name in root_files}
    for name, target in root_files.items():
        destination = project / target
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(root / name, destination)
    frozen_files = {
        'src/campaign/source_campaign_runtime_tu15_generated.cpp': 'campaign.cpp',
        'src/gameplay/weapons/source_weapon_animation_query_symbols_tu15.cpp': 'weapon-symbols.cpp',
        'src/campaign/scripting/native_mcc_script_builtin.cpp': 'builtin.cpp',
        'src/runtime/source_runtime_memory.cpp': 'memory.cpp'}
    frozen_pins = {name: workshop.pin(source / name) for name in frozen_files}
    for name, target in frozen_files.items():
        shutil.copyfile(source / name, project / target)
    ai = source / 'src/campaign/ai/source_ai_frame_tu15_generated.cpp'
    ai_pin = workshop.pin(ai)
    splitter = source / 'tools/split_native_translation_unit.py'
    split_pin = workshop.pin(splitter)
    split = runpy.run_path(str(splitter))['split'](ai, project / 'ai', 12)
    assert split['body_bytes_unchanged'] and split['source_sha256'] == ai_pin['sha256']
    assert all(row['instruction_body_unchanged'] for row in split['entries'])
    fixture = project / 'hs-original.json.zlib'
    oracle = [str(root / '.tools/python/bin/python3'), '-B',
        str(root / 'tools/oracle_workshop_looping_prediction_hs_tu15.py'),
        '--output', str(fixture), '--report', str(args.output / 'original.json')]
    subprocess.run(oracle, check=True)
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
    ai_sources = ['ai/router.cpp'] + [f'ai/body_{n:03d}.cpp' for n in range(12)]
    sources = ['test.cpp', 'workshop.cpp', 'cache.cpp', *frozen_files.values(), *ai_sources]
    sources += [target for name, target in root_files.items() if name.startswith('src/core/')]
    (project / 'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.25)
project(WorkshopLoopingPredictionHs LANGUAGES CXX)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 REQUIRED)
find_package(Threads REQUIRED)
include(''' + json.dumps(str(source / 'cmake/TestStorage.cmake')) + ''')
add_executable(prediction_hs ''' + ' '.join(sources) + ''')
target_compile_features(prediction_hs PRIVATE cxx_std_20)
target_compile_options(prediction_hs PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_include_directories(prediction_hs PRIVATE private ''' + json.dumps(str(source / 'include')) + ' ' + json.dumps(str(source / 'src')) + ''')
target_link_libraries(prediction_hs PRIVATE nlohmann_json::nlohmann_json SDL2::SDL2 Threads::Threads
''' + ''.join(' ' + json.dumps(word) + '\n' for word in links) + ''')
add_test(NAME native_looping_prediction_HS COMMAND "${Python3_EXECUTABLE}" -B
 "${CMAKE_CURRENT_SOURCE_DIR}/reference.py" --fixture "${CMAKE_CURRENT_SOURCE_DIR}/hs-original.json.zlib"
 --test-executable "$<TARGET_FILE:prediction_hs>")
set_tests_properties(native_looping_prediction_HS PROPERTIES TIMEOUT 180)
''')
    command = [sys.executable, '-B', str(root / 'tools/run_tests.py'), '--source', str(project),
        '--scratch-root', str(owner / 'builds'), '--output', str(args.output / 'validation'),
        '--target', 'prediction_hs', '--filter', '^native_looping_prediction_HS$', '--jobs', '2',
        '--test-jobs', '1', '--retain-report', 'Testing/Temporary/LastTest.log']
    result = subprocess.run(command)
    unchanged = source_hashes(source) == inventory
    unchanged &= all(workshop.pin(root / name) == pin for name, pin in root_pins.items())
    unchanged &= all(workshop.pin(source / name) == pin for name, pin in frozen_pins.items())
    unchanged &= all(workshop.pin(Path(path)) == pin for path, pin in libraries.items())
    unchanged &= workshop.pin(ai) == ai_pin and workshop.pin(splitter) == split_pin and workshop.pin(link) == link_pin
    report = dict(scope=__doc__, passed=result.returncode == 0 and unchanged, exit_code=result.returncode,
        all_sources_and_borrowed_archives_unchanged=unchanged, source_inventory_sha256=recovery['production_inventory_sha256'],
        Root_sources=root_pins, frozen_sources=frozen_pins, complete_AI_source=ai_pin,
        complete_AI_body_count=split['body_count'], complete_AI_body_bytes_preserved=True,
        borrowed_libraries=libraries, oracle_command=oracle, command=command,
        fixture=json.loads(fixture.with_suffix('').read_text()),
        actual_World_qualified=False, MCC_resources_and_cold_streaming_qualified=False,
        declaration_admission_changed=False, complete_mod_support=False,
        owned_scratch=str(owner), retained_large_artifacts=[])
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], exit_code=result.returncode, sources_unchanged=unchanged)))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
