"""Check the exact selected terminal delta against the frozen camera renderer.

Dry-run all three production and four test changes without changing the active
source. Compile the complete selected machine module and its three real CPU
consumer tests against the camera cohort's pinned libraries. This is migration
compatibility, not a new engine, MCC mission or publication qualification.
Run through run_test.py; run_tests.py owns the private compiler tree.
"""
if not __debug__:
    raise RuntimeError('Terminal compatibility validation requires enabled Python assertion guards')

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys


def pin(path):
    with path.open('rb') as stream:
        return dict(bytes=path.stat().st_size, sha256=hashlib.file_digest(stream, 'sha256').hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--recovery', type=Path, required=True)
    parser.add_argument('--selected-reference', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.output.resolve().is_relative_to(owner)
    args.output.mkdir(parents=True, exist_ok=False)
    sys.path.insert(0, str(root / 'tools'))
    from build_launcher_engine import publication_identity, source_hashes
    recovery = json.loads(args.recovery.read_text())
    source = Path(recovery['source']).resolve(strict=True)
    build = source.parent / 'build'
    inventory = source_hashes(source)
    assert hashlib.sha256(json.dumps(inventory, sort_keys=True).encode()).hexdigest() == recovery['production_inventory_sha256']
    reference = json.loads(args.selected_reference.read_text())
    assert reference['passed'] and len(reference['changed_files']) == 7
    current = json.loads((root / 'gameplay-runtime-selection.json').read_text())
    assert current['runtime'] == reference['runtime']
    assert all(list(publication_identity(root / name)) == value for name, value in reference['identities'].items())
    recipe_path = root / reference['reconstruction']
    recipe = json.loads(recipe_path.read_text())
    patch = Path(recipe['patch'])
    assert pin(patch)['sha256'] == recipe['patch_sha256']
    before = {row['path']: row['before_sha256'] for row in recipe['changed_files']}
    project = owner / 'project'
    project.mkdir()
    staged = project / 'delta'
    staged.mkdir()
    originals = {}
    for row in reference['changed_files']:
        name = row['path']
        original = source / name
        originals[name] = pin(original)
        assert originals[name]['sha256'] == before[name]
        destination = staged / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(original, destination)
    result = subprocess.run(['patch', '--batch', '--fuzz=0', '-p1', '-i', str(patch)],
                            cwd=staged, capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    after = {}
    future_inventory = inventory.copy()
    for row in reference['changed_files']:
        name = row['path']
        after[name] = pin(staged / name)
        assert after[name]['sha256'] == row['sha256']
        if row['kind'] == 'production':
            future_inventory[name] = row['sha256']
    # These two public files are an explicitly paired implementation/header
    # change. All other public/private ABI inputs remain the frozen cohort's.
    header = project / 'include/halo2/source_machine_scene.hpp'
    header.parent.mkdir(parents=True)
    shutil.copyfile(staged / 'include/halo2/source_machine_scene.hpp', header)
    shutil.copyfile(staged / 'src/gameplay/objects/source_machine_scene.cpp', project / 'machine.cpp')
    # Also compile-check the complete selected production caller with the
    # camera engine's actual flags. Do not substitute the newer dirty Root
    # caller or silently discard a selected feature to make the merge pass.
    flags = build / 'CMakeFiles/halo2_systems.dir/flags.make'
    flag_pin = pin(flags)
    values = {}
    for line in flags.read_text().splitlines():
        if ' = ' in line:
            key, value = line.split(' = ', 1)
            values[key] = value
    syntax = [os.environ.get('CXX', 'c++'), '-I' + str(project / 'include')]
    for key in ('CXX_DEFINES', 'CXX_INCLUDES', 'CXX_FLAGS'):
        syntax += shlex.split(values[key])
    syntax += ['-fsyntax-only', str(staged / 'src/gameplay/multiplayer/multiplayer_runtime.cpp')]
    with (args.output / 'selected-caller-syntax.log').open('w') as log:
        checked = subprocess.run(syntax, stdout=log, stderr=subprocess.STDOUT)
    if checked.returncode:
        failure = dict(scope=__doc__, passed=False, status='selected_caller_compile_failure',
            command=syntax, exit_code=checked.returncode, active_source_mutated=False,
            owned_scratch=str(owner), retained_large_artifacts=[])
        (args.output / 'report.json').write_text(json.dumps(failure, indent=2) + '\n')
        return checked.returncode
    (project / 'src').symlink_to(source / 'src', target_is_directory=True)
    (project / 'tests').mkdir()
    (project / 'tests/fixtures').symlink_to(source / 'tests/fixtures', target_is_directory=True)
    tests = ['source_machine_scene', 'source_machine_portal', 'source_object_lod_scene']
    for name in tests:
        shutil.copyfile(staged / ('tests/' + name + '_tests.cpp'), project / ('tests/' + name + '_tests.cpp'))
    for name in ['assets', 'resources', 'analysis']:
        (project / name).symlink_to(root / name, target_is_directory=True)
    link = build / 'CMakeFiles/source_machine_scene_tests.dir/link.txt'
    link_pin = pin(link)
    words = shlex.split(link.read_text())
    words = words[words.index('-o') + 2:]
    links, libraries = [], {}
    for word in words:
        if word.endswith('.a') or ('.so' in word and not word.startswith('-')):
            path = Path(word)
            path = (path if path.is_absolute() else build / path).resolve(strict=True)
            libraries[str(path)] = pin(path)
            links.append(str(path))
        else:
            assert word.startswith('-')
            links.append(word)
    assert str(build / 'libhalo2_renderer.a') in libraries
    cmake = '''cmake_minimum_required(VERSION 3.25)
project(WorkshopSelectedTerminal LANGUAGES CXX)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 REQUIRED)
include(''' + json.dumps(str(source / 'cmake/TestStorage.cmake')) + ''')
add_library(selected_machine OBJECT machine.cpp)
target_compile_features(selected_machine PRIVATE cxx_std_20)
target_link_libraries(selected_machine PRIVATE SDL2::SDL2 nlohmann_json::nlohmann_json)
target_compile_options(selected_machine PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_include_directories(selected_machine PRIVATE include ''' + json.dumps(str(source / 'include')) + ' ' + json.dumps(str(source / 'src')) + ''')
'''
    for name in tests:
        cmake += 'add_executable(' + name + ' tests/' + name + '_tests.cpp)\n'
        cmake += 'target_compile_features(' + name + ' PRIVATE cxx_std_20)\n'
        cmake += 'target_include_directories(' + name + ' PRIVATE include ' + json.dumps(str(source / 'include')) + ' ' + json.dumps(str(source / 'src')) + ')\n'
        cmake += 'target_link_libraries(' + name + ' PRIVATE selected_machine nlohmann_json::nlohmann_json SDL2::SDL2\n' + '\n'.join(json.dumps(v) for v in links) + ')\n'
        cmake += 'add_test(NAME selected_terminal_' + name + ' COMMAND ' + name + ')\n'
        cmake += 'set_tests_properties(selected_terminal_' + name + ' PROPERTIES TIMEOUT 180 WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")\n'
    (project / 'CMakeLists.txt').write_text(cmake)
    command = [sys.executable, '-u', '-B', str(root / 'tools/run_tests.py'),
        '--source', str(project), '--scratch-root', str(owner / 'compiler'),
        '--output', str(args.output / 'validation'), '--jobs', '2', '--test-jobs', '1',
        '--filter', '^selected_terminal_', '--retain-report', 'Testing/Temporary/LastTest.log']
    for name in tests:
        command += ['--target', name]
    preparation = dict(scope=__doc__, status='prepared_not_validated', passed=False,
        active_source=str(source), camera_inventory_sha256=recovery['production_inventory_sha256'],
        future_inventory_sha256=hashlib.sha256(json.dumps(future_inventory, sort_keys=True).encode()).hexdigest(),
        exact_patch_applied_to_private_copy=True, originals=originals, selected_delta=after,
        complete_selected_multiplayer_caller_typechecked=True, caller_syntax_command=syntax,
        selected_runtime=reference['runtime'], borrowed_libraries=libraries, command=command,
        active_source_mutated=False, complete_mod_support=False,
        owned_scratch=str(owner), retained_large_artifacts=[])
    report_path = args.output / 'report.json'
    report_path.write_text(json.dumps(preparation, indent=2) + '\n')
    run = subprocess.run(command)
    unchanged = source_hashes(source) == inventory and pin(link) == link_pin and pin(flags) == flag_pin
    unchanged &= all(pin(Path(name)) == value for name, value in libraries.items())
    unchanged &= all(pin(source / name) == value for name, value in originals.items())
    unchanged &= all(list(publication_identity(root / name)) == value for name, value in reference['identities'].items())
    preparation.update(status='finished', passed=run.returncode == 0 and unchanged,
                       exit_code=run.returncode, all_source_library_and_selection_inputs_unchanged=unchanged)
    report_path.write_text(json.dumps(preparation, indent=2) + '\n')
    print(json.dumps(dict(passed=preparation['passed'], exit_code=run.returncode, inputs_unchanged=unchanged)))
    return 0 if preparation['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
