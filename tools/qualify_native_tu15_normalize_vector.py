#!/usr/bin/env python3
"""Freeze the normalizer core and run focused disposable CMake qualification."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--storage-only',action='store_true');a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'source';project.mkdir();a.output.mkdir(parents=True,exist_ok=True)
    shutil.copytree(ROOT/'include',project/'include')
    names=['src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp','tests/native_tu15_normalize_vector_tests.cpp',
        'tests/native_campaign_ai_vector_math_tu15_tests.cpp','tests/native_tu15_light_up_vector_tests.cpp',
        'src/rendering/effects/native_tu15_light_up_vector.cpp','src/physics/runtime/native_tu15_normalize_vector_bridge.hpp',
        'src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp',
        'src/physics/runtime/physics_instruction_remap_tu15.hpp',
        'src/physics/runtime/physics_solver_internal.hpp','src/physics/runtime/physics_step_internal.hpp',
        'tools/validate_native_tu15_normalize_vector.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp',
        'include/halo2/native_tu15_normalize_vector.hpp','include/halo2/detail/native_tu15_arithmetic_environment.hpp',
        'include/halo2/physics_contact_runtime.hpp','include/halo2/physics_contact_record.hpp',
        'include/halo2/source_runtime_memory.hpp','include/halo2/source_runtime_write_protection.hpp',
        'tests/native_tu15_normalize_storage_tests.cpp','src/runtime/source_runtime_memory.cpp',
        'src/runtime/source_runtime_write_protection.inc','src/runtime/source_runtime_snapshot_bytes.cpp','src/assets/cache_file_snapshot.cpp',
        'cmake/NativeTu15NormalizeVector.cmake','cmake/SnapshotStorage.cmake']
    pins={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()for n in names}
    for name in names:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    (project/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(NativeNormalizeTransport LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(Threads REQUIRED)
include("{ROOT}/cmake/TestStorage.cmake")
add_executable(normalize tests/native_tu15_normalize_vector_tests.cpp src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp)
add_executable(ai_vector tests/native_campaign_ai_vector_math_tu15_tests.cpp src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp)
add_executable(light_vector tests/native_tu15_light_up_vector_tests.cpp src/rendering/effects/native_tu15_light_up_vector.cpp src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp)
include(cmake/SnapshotStorage.cmake)
set(HALO2_RENDERER_ORACLE_PYTHON "{ROOT}/.tools/python/bin/python")
set(HALO2_TEST_ASSET_SOURCE_ROOT "{ROOT}")
set(HALO2_TU15_GAMEPLAY_XBE "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
include(cmake/NativeTu15NormalizeVector.cmake)
foreach(target IN ITEMS normalize ai_vector light_vector)
  target_include_directories(${{target}} PRIVATE include src)
  target_compile_features(${{target}} PRIVATE cxx_std_20)
  target_compile_options(${{target}} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
  target_link_libraries(${{target}} PRIVATE nlohmann_json::nlohmann_json)
  if(HALO2_SANITIZERS)
    target_compile_options(${{target}} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${{target}} PRIVATE -fsanitize=address,undefined)
  endif()
endforeach()
add_test(NAME normalize_physical COMMAND "{ROOT}/.tools/python/bin/python" "{ROOT}/tools/validate_native_tu15_normalize_vector.py" --native $<TARGET_FILE:normalize> --report "${{CMAKE_CURRENT_BINARY_DIR}}/normalize.json" --tu15 "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe" --mp11 "{ROOT}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03")
add_test(NAME ai_vector_fixture COMMAND "${{Python3_EXECUTABLE}}" "{ROOT}/tools/test_native_campaign_screen_lifecycle_tu15.py" --probe $<TARGET_FILE:ai_vector> --fixture "{ROOT}/tests/fixtures/native_campaign_ai_vector_math_tu15.json.gz")
add_test(NAME light_vector_physical COMMAND "{ROOT}/.tools/python/bin/python" "{ROOT}/tools/validate_native_tu15_light_up_vector.py" --native $<TARGET_FILE:light_vector> --report "${{CMAKE_CURRENT_BINARY_DIR}}/light.json" --xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
set_tests_properties(normalize_physical light_vector_physical PROPERTIES TIMEOUT 180)
''')
    results=[]
    for kind,compiler,sanitize in [('gcc','g++',False),('clang-sanitizers','clang++',True)]:
        targets=['native_tu15_normalize_storage_tests']if a.storage_only else['normalize','ai_vector','light_vector','native_tu15_normalize_storage_tests']
        selected='^original_tu15_native_normalize_real_storage$'if a.storage_only else'^(normalize_physical|ai_vector_fixture|light_vector_physical|original_tu15_native_normalize_real_storage)$'
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
            '--output',str(a.output/kind),'--jobs','2','--test-jobs','1',
            '--filter',selected,'--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
            '--retain-report','normalize.json','--retain-report','light.json','--retain-report','Testing/Temporary/LastTest.log']
        for target in targets:command+=['--target',target]
        if sanitize:command+=['--sanitizers']
        run=subprocess.run(command);results.append(dict(kind=kind,returncode=run.returncode,command=command))
        if run.returncode:break
    unchanged=all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==pin for n,pin in pins.items())
    passed=len(results)==2 and all(r['returncode']==0 for r in results) and unchanged
    (a.output/'validation.json').write_text(json.dumps(dict(passed=passed,results=results,source_sha256=pins,
        source_unchanged=unchanged,storage_only=a.storage_only,owned_scratch=str(scratch),large_artifacts_retained=[]),indent=2)+'\n')
    return int(not passed)


if __name__=='__main__':raise SystemExit(main())
