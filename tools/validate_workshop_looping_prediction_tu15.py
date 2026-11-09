"""Compare native looping prediction/cache touches with unchanged TU1.5.

An additive private generation preserves every existing Workshop body and
adds607's matched evaluator plus18D1D0/128A20. The effect test executes both
complete effect bodies and an explicitly pinned production21F260 cache kernel;
no dependency returns are supplied. Cold allocation, MCC projection and actual
HS composition are not accepted here. Production source/admission is unchanged.
Run through run_test.py; run_tests.py owns every compiler tree.
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
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--frozen-source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--diagnostic', action='store_true',
                        help='Only the real GCC component; not complete compiler/control qualification')
    parser.add_argument('--native-cache-source', type=Path,
                        help='Explicit current Root cache kernel; default is the frozen cache kernel')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source = args.frozen_source.resolve(strict=True)
    assert source.name == 'source' and source.is_relative_to(root / '.local/test-scratch')
    assert not args.output.resolve().is_relative_to(owner)
    args.output.mkdir(parents=True, exist_ok=False)
    project = owner / 'project'
    project.mkdir()
    shutil.copytree(source / 'include', project / 'include')
    shutil.copytree(source / 'src/physics/runtime', project / 'src/physics/runtime')
    borrowed = ['src/audio/native_tu15_sound_kernels_resources.cpp', 'src/audio/native_tu15_sound_kernel.hpp',
                'src/campaign/scripting/source_workshop_script_builtins_tu15_generated.cpp',
                'tools/run_test.py', 'cmake/TestStorage.cmake']
    frozen_pins = {name: digest(source / name) for name in borrowed}
    for directory in ('include', 'src/physics/runtime'):
        frozen_pins.update({p.relative_to(source).as_posix(): digest(p)
                            for p in (source / directory).rglob('*') if p.is_file()})
    for name in borrowed:
        target = project / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source / name, target)
    native_cache_source = (args.native_cache_source or source / borrowed[0]).resolve(strict=True)
    assert native_cache_source in ((root / borrowed[0]).resolve(), (source / borrowed[0]).resolve())
    native_cache_pin = digest(native_cache_source)
    shutil.copyfile(native_cache_source, project / borrowed[0])
    core_sources, root_dependency_pins = [], {}
    if native_cache_source == (root / borrowed[0]).resolve():
        # The current cache kernel calls these actual native core owners.
        # Include their complete source cohort; never stub their link symbols.
        for stem in ('native_tu15_data_array_allocate_specific', 'native_tu15_data_array_allocate',
                     'native_tu15_data_array_delete', 'native_tu15_page_allocator'):
            core_sources.append('src/core/' + stem + '.cpp')
            for name in ('include/halo2/' + stem + '.hpp', core_sources[-1]):
                root_dependency_pins[name] = digest(root / name)
                target = project / name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(root / name, target)
    helpers = ['tests/workshop_looping_prediction_tests.cpp', 'tests/workshop_unit_controls_tests.py',
               'tools/audit_workshop_looping_prediction_tu15.py',
               'tools/validate_workshop_looping_prediction_tu15.py',
               'tools/generate_workshop_script_builtins_tu15.py']
    pins = {name: digest(root / name) for name in helpers}
    for name in helpers[:2]:
        target = project / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(root / name, target)
    from audit_workshop_compatibility import Pe64, xbox_catalog
    from oracle_smg_firing_1_1 import Xbe
    from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
    pe = Pe64(Path.home() / '.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll')
    declaration = pe.builtin(607)
    assert declaration['name'] == 'sound_looping_predict' and declaration['return_type'] == 4
    assert declaration['arguments'] == [35]
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    candidates = [row for row in xbox_catalog(image)['sound_looping_predict']
                  if row['xbox_return_type'] == 4 and row['xbox_arguments'] == [35]]
    assert len(candidates) == 1 and candidates[0]['xbox_opcode'] == 605
    assert candidates[0]['xbox_evaluator'] == '0x2b3e10'
    audit = owner / 'audit.json'
    audit.write_text(json.dumps(dict(xbox_tu15_sha256=NEW_PIN,
        declaration_candidates=[dict(**declaration, matches=candidates)]), indent=2) + '\n')
    python = root / '.tools/python/bin/python3'
    generated = project / 'prediction.cpp'
    generation = owner / 'generation'
    generate = [str(python), '-B', str(root / 'tools/generate_workshop_script_builtins_tu15.py'),
        '--audit-report', str(audit), '--output', str(generated), '--directory', str(generation),
        '--preserve-module', str(source / borrowed[2]), '--names', 'sound_looping_predict',
        '--owned-callees', '0x18d1d0', '0x128a20']
    subprocess.run(generate, check=True)
    fixture = project / 'tests/fixtures/workshop_looping_prediction.json.zlib'
    oracle = [str(python), '-B', str(root / 'tools/audit_workshop_looping_prediction_tu15.py'),
        '--report', str(args.output / 'original.json'), '--fixture', str(fixture)]
    subprocess.run(oracle, check=True)
    text = generated.read_text()
    mutations = [('wrong_track', '0x18d1fau', '0x20u', '0x10u'),
                 ('wrong_priority', '0x128a71u', '0x2u', '0x4u'),
                 ('wrong_pitch_stride', '0x128a4eu', '*2u', '*3u'),
                 ('missing_loop', '0x18d20cu', 'if(s.flags.sf!=s.flags.of)goto L18d1f4;', ';')]
    variants = []
    for name, pc, needle, replacement in mutations:
        lines = [line for line in text.splitlines(keepends=True) if f'instruction({pc})' in line]
        assert len(lines) == 1 and lines[0].count(needle) == 1, (name, lines)
        (project / (name + '.cpp')).write_text(text.replace(lines[0], lines[0].replace(needle, replacement)))
        variants.append(name)
    (project / 'negative.py').write_text('''import subprocess,sys
r=subprocess.run(sys.argv[1:],capture_output=True,text=True)
assert r.returncode!=0 and 'prediction' in r.stderr.lower(),(r.returncode,r.stdout,r.stderr)
print('Expected original-backed rejection:',r.stderr.strip())
''')
    cmake = '''cmake_minimum_required(VERSION 3.20)
project(WorkshopLoopingPrediction LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(cmake/TestStorage.cmake)
if(HALO2_SANITIZERS)
add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
add_link_options(-fsanitize=address,undefined)
endif()
'''
    cmake += f'''add_library(prediction_cache OBJECT src/audio/native_tu15_sound_kernels_resources.cpp {' '.join(core_sources)})
add_library(prediction_consumer OBJECT tests/workshop_looping_prediction_tests.cpp)
foreach(part IN ITEMS prediction_cache prediction_consumer)
 target_include_directories(${{part}} PRIVATE include src)
 target_compile_features(${{part}} PRIVATE cxx_std_20)
 target_compile_options(${{part}} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
 target_link_libraries(${{part}} PRIVATE nlohmann_json::nlohmann_json)
endforeach()
'''
    for name in ['prediction'] + variants:
        cmake += f'''add_executable({name} {name}.cpp $<TARGET_OBJECTS:prediction_cache> $<TARGET_OBJECTS:prediction_consumer>)
target_include_directories({name} PRIVATE include src)
target_compile_features({name} PRIVATE cxx_std_20)
target_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries({name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME looping_prediction_{name} COMMAND '''
        if name != 'prediction':
            cmake += '"${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/negative.py" '
        cmake += f'''"${{Python3_EXECUTABLE}}" -B "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/workshop_unit_controls_tests.py"
 --fixture "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/workshop_looping_prediction.json.zlib"
 --test-executable "$<TARGET_FILE:{name}>")
'''
    (project / 'CMakeLists.txt').write_text(cmake)
    results = []
    modes = [('g++', False)] if args.diagnostic else [('g++', False), ('clang++', False), ('clang++', True)]
    for compiler, sanitized in modes:
        label = compiler + ('-sanitized' if sanitized else '')
        command = [sys.executable, str(root / 'tools/run_tests.py'), '--source', str(project),
            '--scratch-root', str(owner / 'builds'), '--output', str(args.output / label),
            '--filter', '^looping_prediction_', '--cmake-arg=-DCMAKE_CXX_COMPILER=' + compiler,
            '--jobs', '2', '--test-jobs', '1', '--retain-report', 'Testing/Temporary/LastTest.log']
        for name in ['prediction'] + ([] if sanitized or args.diagnostic else variants):
            command += ['--target', name]
        if sanitized or args.diagnostic:
            # Build/run only the real component in the sanitizer cohort.
            command[command.index('^looping_prediction_')] = '^looping_prediction_prediction$'
            if sanitized:
                command += ['--sanitizers']
        run = subprocess.run(command)
        results.append(dict(compiler=compiler, sanitizers=sanitized, command=command, exit_code=run.returncode))
        if run.returncode:
            break
    unchanged = all(digest(root / name) == pin for name, pin in pins.items())
    unchanged &= all(digest(source / name) == pin for name, pin in frozen_pins.items())
    unchanged &= digest(native_cache_source) == native_cache_pin
    unchanged &= all(digest(root / name) == pin for name, pin in root_dependency_pins.items())
    report = dict(scope=__doc__, passed=unchanged and all(row['exit_code'] == 0 for row in results),
        sources_unchanged=unchanged, source_hashes=pins, frozen_source=str(source),
        frozen_file_count=len(frozen_pins),
        frozen_inventory_sha256=hashlib.sha256(json.dumps(frozen_pins, sort_keys=True).encode()).hexdigest(),
        native_cache_kernel_source=str(native_cache_source), native_cache_kernel_sha256=native_cache_pin,
        uses_frozen_cache_kernel=native_cache_source == (source / borrowed[0]).resolve(),
        additional_current_native_dependencies=root_dependency_pins,
        generation_command=generate, generation=json.loads((generation / 'generation.json').read_text()),
        oracle_command=oracle, fixture=json.loads(fixture.with_suffix('').read_text()),
        diagnostic_only=args.diagnostic, negative_controls=[] if args.diagnostic else variants,
        results=results, runtime_enabled=False,
        HS_caller_qualified=False, MCC_projection_qualified=False, cold_streaming_qualified=False,
        published_component=False, owned_scratch=str(owner), retained_large_artifacts=[])
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], results=[row['exit_code'] for row in results])))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
