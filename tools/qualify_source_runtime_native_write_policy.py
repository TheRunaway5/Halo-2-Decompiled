#!/usr/bin/env python3
"""Disposable native-write admission and shared-lock reentrancy qualification."""
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
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=scratch/'source';source.mkdir()
    a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=True)
    shutil.copytree(ROOT/'include',source/'include')
    names=['src/runtime/source_runtime_memory.cpp','src/runtime/source_runtime_write_protection.inc',
        'src/runtime/source_execution_inputs.cpp','src/runtime/source_runtime_snapshot_bytes.cpp',
        'src/assets/cache_file_snapshot.cpp','tests/source_runtime_policy_hold_tests.cpp',
        'tests/source_runtime_write_dispatch_reentrancy_tests.cpp','include/halo2/source_runtime_write_protection.hpp',
        'include/halo2/physics_contact_record.hpp','include/halo2/source_runtime_memory.hpp',
        'tests/source_runtime_memory_tests.cpp','src/gameplay/objects/object_body_heap.cpp']
    pins={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()for n in names}
    for n in names:
        target=source/n;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/n,target)
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeWritePolicy LANGUAGES CXX)
include(CTest)
find_package(Threads REQUIRED)
add_library(storage STATIC src/runtime/source_runtime_snapshot_bytes.cpp src/assets/cache_file_snapshot.cpp)
target_include_directories(storage PUBLIC include)
target_compile_features(storage PUBLIC cxx_std_20)
add_executable(policy tests/source_runtime_policy_hold_tests.cpp src/runtime/source_runtime_memory.cpp src/runtime/source_execution_inputs.cpp)
add_executable(reentrancy tests/source_runtime_write_dispatch_reentrancy_tests.cpp)
add_executable(memory tests/source_runtime_memory_tests.cpp src/runtime/source_runtime_memory.cpp src/gameplay/objects/object_body_heap.cpp)
foreach(target IN ITEMS storage policy reentrancy memory)
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_include_directories(${target} PRIVATE include src)
    if(HALO2_SANITIZERS)
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endforeach()
target_link_libraries(policy PRIVATE storage Threads::Threads)
target_link_libraries(reentrancy PRIVATE storage Threads::Threads)
target_link_libraries(memory PRIVATE storage Threads::Threads)
add_test(NAME source_runtime_policy_publication COMMAND policy)
add_test(NAME source_runtime_native_write_recursive_refusal COMMAND reentrancy)
add_test(NAME source_runtime_retained_memory COMMAND memory)
add_test(NAME source_runtime_retained_memory_reference COMMAND memory)
set_tests_properties(source_runtime_retained_memory_reference PROPERTIES ENVIRONMENT "HALO2_REFERENCE_SMALL_MEMORY_TRANSFERS=1")
set_tests_properties(source_runtime_policy_publication PROPERTIES TIMEOUT 20)
set_tests_properties(source_runtime_native_write_recursive_refusal PROPERTIES TIMEOUT 3)
'''
    (source/'CMakeLists.txt').write_text(cmake)
    results=[]
    def run(kind,compiler,sanitize,mutant=False):
        argv=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'builds'),
            '--output',str(a.output/kind),'--jobs','2','--test-jobs','1',
            '--target','reentrancy','--filter','^source_runtime_native_write_recursive_refusal$'if mutant else'^source_runtime_(policy_publication|native_write_recursive_refusal|retained_memory|retained_memory_reference)$',
            '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
        if not mutant:argv+=['--target','policy','--target','memory']
        if sanitize:argv+=['--sanitizers']
        proc=subprocess.run(argv);results.append(dict(kind=kind,command=argv,exit_code=proc.returncode))
        return proc.returncode
    passed=run('gcc','g++',False)==0
    if passed:passed=run('clang-sanitizers','clang++',True)==0
    mutation_detected=False
    if passed:
        inc=source/'src/runtime/source_runtime_write_protection.inc';text=inc.read_text()
        start=text.index('    // A writer can send this call through the shared-lock path')
        end=text.index('    dispatch_write(registry,memory,address,bytes,observe,slot.owner);',start)
        inc.write_text(text[:start]+text[end:])
        code=run('missing-slow-dispatch-depth','g++',False,True)
        log=(a.output/'missing-slow-dispatch-depth/tests.log').read_text()
        mutation_detected=code!=0 and 'Timeout' in log
    unchanged=all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==v for n,v in pins.items())
    passed=passed and mutation_detected and unchanged
    (a.output/'validation.json').write_text(json.dumps(dict(passed=passed,results=results,
        missing_slow_dispatch_depth_detected=mutation_detected,source_sha256=pins,source_unchanged=unchanged,
        owned_scratch=str(scratch),large_artifacts_retained=[]),indent=2)+'\n')
    return int(not passed)


if __name__=='__main__':raise SystemExit(main())
