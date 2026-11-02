"""Qualify complete MCC declaration arity without admitting unowned effects.

Use the authenticated registry's two eight-argument records in a private test
profile. No production declaration is added. Official run_tests.py owns each
compiler tree; the enclosing run_test.py removes the code-only project.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import runpy
import shutil
import subprocess
import sys


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--registry',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.output.resolve().is_relative_to(scratch)
    registry=json.loads(args.registry.read_text())
    assert registry['mcc_sha256']=='de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946'
    assert registry['declaration_count']==918 and registry['maximum_arity']==8
    declarations=[row for row in registry['rows']if len(row['arguments'])==8]
    assert [(row['mcc_opcode'],row['name'])for row in declarations]==[
        (596,'sound_set_tag_parameter_unsafe'),(712,'cinematic_set_environment_map_tint')]
    # Reauthenticate the exact input records against the installed pinned DLL.
    sys.path.insert(0,str(root/'tools'))
    from audit_workshop_compatibility import Pe64
    pe=Pe64(Path.home()/'.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll')
    for row in declarations:
        assert all(row[key]==value for key,value in pe.builtin(row['mcc_opcode']).items())
    project=scratch/'project';project.mkdir()
    paths=['src/campaign/scripting/mcc_script_layout.cpp','src/campaign/mcc_cairo_vanilla_scripts.inc',
        'src/assets/cache.cpp','src/assets/cache_file_snapshot.cpp','src/runtime/source_runtime_snapshot_bytes.cpp',
        'cmake/TestStorage.cmake','cmake/SnapshotStorage.cmake','tools/run_test.py',
        'tools/generate_mcc_script_layout.py','tools/mcc-script-layout.json','tests/mcc_script_arity_tests.cpp']
    paths += [p.relative_to(root).as_posix()for p in (root/'include').rglob('*')if p.is_file()]
    pins={name:hashlib.sha256((root/name).read_bytes()).hexdigest()for name in paths}
    for name in paths:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(root/name,target)
    profile=json.loads((project/'tools/mcc-script-layout.json').read_text())
    original_count=len(profile['builtins'])
    for row in declarations:
        assert not any(old['mcc_opcode']==row['mcc_opcode']for old in profile['builtins'])
        profile['builtins'].append({**row,'xbox_opcode':None,'xbox_flags':0})
    generator=project/'tools/generate_mcc_script_layout.py'
    generated=runpy.run_path(str(generator))['generated'](profile)
    target=project/'src/campaign/scripting/mcc_script_layout.inc';target.write_text(generated)
    reference=dict(mcc_sha256=registry['mcc_sha256'],declarations=declarations)
    (project/'reference.json').write_text(json.dumps(reference,indent=2)+'\n')
    legacy=generator.read_text();assert legacy.count('len(row["arguments"]) > 8')==1
    old_generator=project/'old_generator.py';old_generator.write_text(legacy.replace('len(row["arguments"]) > 8','len(row["arguments"]) > 7'))
    rejected=False
    try:runpy.run_path(str(old_generator))['generated'](profile)
    except ValueError as error:rejected=str(error)=='builtin argument extent exceeds compiled schema'
    assert rejected,'Old generator did not expose the actual registry limitation'
    native=(project/'src/campaign/scripting/mcc_script_layout.cpp').read_text()
    assert native.count('std::array<std::uint16_t,8> arguments;')==1
    (project/'old_metadata.cpp').write_text(native.replace('std::array<std::uint16_t,8> arguments;','std::array<std::uint16_t,7> arguments;'))
    (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.25)
project(MccScriptArity LANGUAGES CXX)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
include(cmake/TestStorage.cmake)
include(cmake/SnapshotStorage.cmake)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
try_compile(old_capacity_accepted SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/old_metadata.cpp"
 CMAKE_FLAGS "-DCMAKE_CXX_STANDARD=20"
 COMPILE_DEFINITIONS "-I${CMAKE_CURRENT_SOURCE_DIR}/include" "-I${CMAKE_CURRENT_SOURCE_DIR}/src"
 OUTPUT_VARIABLE old_capacity_log)
if(old_capacity_accepted OR NOT old_capacity_log MATCHES "too many initializers|excess elements")
 message(FATAL_ERROR "Original registry did not reject the old seven-slot native capacity: ${old_capacity_log}")
endif()
file(WRITE "${CMAKE_BINARY_DIR}/old-capacity-control.log" "${old_capacity_log}")
add_executable(mcc_script_arity_tests tests/mcc_script_arity_tests.cpp
 src/campaign/scripting/mcc_script_layout.cpp src/assets/cache.cpp)
target_include_directories(mcc_script_arity_tests PRIVATE include src)
target_compile_features(mcc_script_arity_tests PRIVATE cxx_std_20)
target_compile_definitions(mcc_script_arity_tests PRIVATE HALO2_CACHE_BYTES_ONLY)
target_compile_options(mcc_script_arity_tests PRIVATE -ffunction-sections -fdata-sections)
target_link_options(mcc_script_arity_tests PRIVATE -Wl,--gc-sections)
target_link_libraries(mcc_script_arity_tests PRIVATE halo2_snapshot_storage nlohmann_json::nlohmann_json)
if(HALO2_SANITIZERS)
 target_compile_options(mcc_script_arity_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
 target_link_options(mcc_script_arity_tests PRIVATE -fsanitize=address,undefined)
endif()
add_test(NAME mcc_script_original_eight_argument_declarations COMMAND mcc_script_arity_tests "${CMAKE_CURRENT_SOURCE_DIR}/reference.json")
''')
    results=[]
    for compiler,sanitizers in [('g++',False),('clang++',False),('clang++',True)]:
        name=compiler+('-sanitizers'if sanitizers else '')
        command=[sys.executable,'-B',str(root/'tools/run_tests.py'),'--source',str(project),
            '--scratch-root',str(scratch/'builds'),'--output',str(args.output/name),
            '--target','mcc_script_arity_tests','--filter','^mcc_script_original_eight_argument_declarations$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
            '--retain-report','Testing/Temporary/LastTest.log','--retain-report','old-capacity-control.log']
        if sanitizers:command.append('--sanitizers')
        result=subprocess.run(command)
        results.append(dict(compiler=compiler,sanitizers=sanitizers,exit_code=result.returncode))
    assert all(hashlib.sha256((root/name).read_bytes()).hexdigest()==pin for name,pin in pins.items())
    report=dict(passed=all(row['exit_code']==0 for row in results),scope=__doc__,
        registry_sha256=hashlib.sha256(args.registry.read_bytes()).hexdigest(),source_hashes=pins,
        production_declarations=original_count,private_test_declarations=len(profile['builtins']),
        declarations=declarations,old_generator_rejected=True,results=results,
        owned_scratch=str(scratch),new_effects_admitted=False,retained_large_artifacts=[])
    args.output.mkdir(parents=True,exist_ok=True)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':
    raise SystemExit(main())
