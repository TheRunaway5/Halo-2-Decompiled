#!/usr/bin/env python3
"""Qualify descriptor-carrying I/O and syntax of its real production consumers.

The original allocated sound-cache call is a retained offline reference.
Full World execution, cache publication, device output and worker scheduling
remain separate acceptance. Run in tools/run_test.py; retain small reports only.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);work=owner/('sound-io-'+args.output.name);work.mkdir(exist_ok=False)
    args.output.mkdir(parents=True,exist_ok=False);project=work/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    pins={ROOT/p.relative_to(project):sha(p)for p in (project/'include').rglob('*')if p.is_file()}
    files=['src/audio/source_sound_cache_io_tu15.cpp','tests/source_sound_cache_io_tu15_tests.cpp',
        'src/physics/physics_live_world_tu15.cpp','tools/replay_original_live_first_person.cpp','src/runtime/mcc_runtime_delivery.cpp',
        'src/audio/mcc_sound_timing_reference.cpp','src/audio/mcc_sound_variation_provider.cpp','src/runtime/source_runtime_resources.cpp']
    pending=[]
    def freeze(p):
        if p in pins:return
        pins[p]=sha(p);target=project/p.relative_to(ROOT);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target);pending.append(p)
    for name in files:freeze(ROOT/name)
    while pending:
        p=pending.pop()
        for name in re.findall(r'^#include "([^"\n]+)"',p.read_text(),re.M):
            if name.startswith('halo2/'):continue
            candidates=[p.parent/name,ROOT/'src'/name,ROOT/name]
            found=next((c.resolve()for c in candidates if c.is_file()),None)
            if found and found.is_relative_to(ROOT):freeze(found)
    for p in (Path(__file__),ROOT/'tools/validate_original_sound_cache_io_tu15.py',ROOT/'tools/original_xbox_pose_case.py'):pins[p]=sha(p)
    (project/'dependencies.py').write_text('''import hashlib,json,shlex,sys
from pathlib import Path
project=Path(sys.argv[1]);out=Path(sys.argv[2]);results={}
for name in ('world','replay','canonical','timing','provider','resources'):
 raw=(out/(name+'.d')).read_text().replace('\\\\\\n','');deps={}
 for s in shlex.split(raw.split(':',1)[1]):
  p=Path(s).resolve(strict=True)
  if p.is_relative_to(project):deps[str(p.relative_to(project))]=hashlib.sha256(p.read_bytes()).hexdigest()
 results[name]=deps
(out/'dependencies.json').write_text(json.dumps(dict(passed=True,dependencies=results),indent=2)+'\\n')
''')
    (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(SoundCacheRead LANGUAGES CXX)
include(CTest)
find_package(PkgConfig REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
pkg_check_modules(SYNTAX REQUIRED sdl2 libnm)
add_executable(sound_cache_io src/audio/source_sound_cache_io_tu15.cpp tests/source_sound_cache_io_tu15_tests.cpp)
target_compile_features(sound_cache_io PRIVATE cxx_std_20)
target_include_directories(sound_cache_io PRIVATE include)
target_link_libraries(sound_cache_io PRIVATE nlohmann_json::nlohmann_json)
set(SYNTAX_FLAGS -std=c++20 -O2 -DNDEBUG -ffp-contract=off -DWITH_GZFILEOP -pthread -I${CMAKE_SOURCE_DIR}/include -I${CMAKE_SOURCE_DIR}/src ${SYNTAX_CFLAGS})
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")
add_custom_target(caller_syntax
 COMMAND ${CMAKE_CXX_COMPILER} ${SYNTAX_FLAGS} -fsyntax-only "${CMAKE_SOURCE_DIR}/src/physics/physics_live_world_tu15.cpp"
 COMMAND ${CMAKE_CXX_COMPILER} ${SYNTAX_FLAGS} -MM -MT world -MF "${CMAKE_BINARY_DIR}/evidence/world.d" "${CMAKE_SOURCE_DIR}/src/physics/physics_live_world_tu15.cpp"
 COMMAND ${CMAKE_CXX_COMPILER} ${SYNTAX_FLAGS} -fsyntax-only "${CMAKE_SOURCE_DIR}/tools/replay_original_live_first_person.cpp"
 COMMAND ${CMAKE_CXX_COMPILER} ${SYNTAX_FLAGS} -MM -MT replay -MF "${CMAKE_BINARY_DIR}/evidence/replay.d" "${CMAKE_SOURCE_DIR}/tools/replay_original_live_first_person.cpp"
 COMMAND ${CMAKE_CXX_COMPILER} ${SYNTAX_FLAGS} -fsyntax-only "${CMAKE_SOURCE_DIR}/src/runtime/mcc_runtime_delivery.cpp"
 COMMAND ${CMAKE_CXX_COMPILER} ${SYNTAX_FLAGS} -MM -MT canonical -MF "${CMAKE_BINARY_DIR}/evidence/canonical.d" "${CMAKE_SOURCE_DIR}/src/runtime/mcc_runtime_delivery.cpp"
 COMMAND ${CMAKE_CXX_COMPILER} ${SYNTAX_FLAGS} -fsyntax-only "${CMAKE_SOURCE_DIR}/src/audio/mcc_sound_timing_reference.cpp"
 COMMAND ${CMAKE_CXX_COMPILER} ${SYNTAX_FLAGS} -MM -MT timing -MF "${CMAKE_BINARY_DIR}/evidence/timing.d" "${CMAKE_SOURCE_DIR}/src/audio/mcc_sound_timing_reference.cpp"
 COMMAND ${CMAKE_CXX_COMPILER} ${SYNTAX_FLAGS} -fsyntax-only "${CMAKE_SOURCE_DIR}/src/audio/mcc_sound_variation_provider.cpp"
 COMMAND ${CMAKE_CXX_COMPILER} ${SYNTAX_FLAGS} -MM -MT provider -MF "${CMAKE_BINARY_DIR}/evidence/provider.d" "${CMAKE_SOURCE_DIR}/src/audio/mcc_sound_variation_provider.cpp"
 COMMAND ${CMAKE_CXX_COMPILER} ${SYNTAX_FLAGS} -fsyntax-only "${CMAKE_SOURCE_DIR}/src/runtime/source_runtime_resources.cpp"
 COMMAND ${CMAKE_CXX_COMPILER} ${SYNTAX_FLAGS} -MM -MT resources -MF "${CMAKE_BINARY_DIR}/evidence/resources.d" "${CMAKE_SOURCE_DIR}/src/runtime/source_runtime_resources.cpp"
 COMMAND "'''+sys.executable+'''" "${CMAKE_SOURCE_DIR}/dependencies.py" "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/evidence")
add_custom_target(sound_io_checks DEPENDS sound_cache_io caller_syntax)
add_test(NAME sound_cache_read_original COMMAND "'''+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- "'+sys.executable+'" -B "'+str(ROOT/'tools/validate_original_sound_cache_io_tu15.py')+'''" --binary $<TARGET_FILE:sound_cache_io> --output "${CMAKE_BINARY_DIR}/evidence/original")
set_tests_properties(sound_cache_read_original PROPERTIES TIMEOUT 300)
''')
    report=dict(passed=False,goal_complete=False,full_world_link_execution_or_audio_verified=False,retained_large_artifacts=[])
    try:
        results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(work/'builds'),
                '--output',str(args.output/compiler),'--target','sound_io_checks','--filter','^sound_cache_read_original$',
                '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',
                '--retain-report','evidence/original/report.json','--retain-report','evidence/dependencies.json']
            r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
        report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,
            source_hashes={str(p.relative_to(ROOT)):v for p,v in pins.items()})
    except BaseException as error:report['error']=str(error);raise
    finally:
        changed=[str(p)for p,v in pins.items()if not p.is_file()or sha(p)!=v];report['changed_inputs']=changed;report['inputs_unchanged']=not changed
        if changed:report['passed']=False
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
