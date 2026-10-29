#!/usr/bin/env python3
"""Qualify screen lifecycle kernels and reject two private source mutations.

Run via tools/run_test.py. All frozen/mutated source copies, executables,
objects and decompressed references belong to that scratch owner. Builds and
tests use the repository's official disposable CMake worker.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'src/campaign/native_campaign_screen_lifecycle_tu15.cpp'
TEST=ROOT/'tests/native_campaign_screen_lifecycle_tu15_tests.cpp'
FIXTURE=ROOT/'tests/fixtures/native_campaign_screen_lifecycle_tu15.json.gz'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',default='g++')
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--compile-callers',action='store_true')
    parser.add_argument('--callers-only',action='store_true')
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    if args.callers_only and not args.compile_callers:
        parser.error('--callers-only requires --compile-callers')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    project=scratch/'project';project.mkdir()
    original=SOURCE.read_text()
    frozen={str(SOURCE.relative_to(ROOT)):hashlib.sha256(original.encode()).hexdigest()}
    variants={'native':original,
        'missing_tail':original.replace('word<0xffu','word<0xfeu'),
        'wrong_ack':original.replace('interface+0xa0u','interface+0xa1u')}
    for name,source in variants.items():
        if name!='native' and source==original:raise ValueError('Mutation no longer changes its declared algorithm')
        (project/(name+'.cpp')).write_text(source)
    shutil.copyfile(TEST,project/'test.cpp')
    frozen[str(TEST.relative_to(ROOT))]=sha(project/'test.cpp')
    (project/'original.json').write_bytes(gzip.decompress(FIXTURE.read_bytes()))
    (project/'negative.py').write_text('''import subprocess,sys
result=subprocess.run(sys.argv[1:],capture_output=True,text=True)
if result.returncode==0 or not any(s in result.stderr for s in ('original store count differs','original ordered store differs','original retained bank differs')):
    raise SystemExit('Source mutation was not rejected by the original behavior: '+result.stdout+result.stderr)
print('Independent original rejects source mutation: '+result.stderr.strip())
''')
    cmake=f'''cmake_minimum_required(VERSION 3.20)
project(NativeCampaignScreenLifecycleProof LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
enable_testing()
find_package(nlohmann_json REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
'''
    if args.sanitizers:
        cmake+='add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)\nadd_link_options(-fsanitize=address,undefined)\n'
    for name in variants:
        cmake+=f'''add_executable(screen_{name} test.cpp {name}.cpp)
target_include_directories(screen_{name} PRIVATE "{ROOT/'include'}")
target_link_libraries(screen_{name} PRIVATE nlohmann_json::nlohmann_json)
'''
        if name=='native':cmake+='add_test(NAME screen_native COMMAND screen_native "${CMAKE_CURRENT_SOURCE_DIR}/original.json")\n'
        else:cmake+=f'add_test(NAME screen_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "$<TARGET_FILE:screen_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/original.json")\n'
    caller_files=[]
    if args.compile_callers:
        caller_files=[ROOT/'src/physics/physics_live_world_tu15.cpp',
            ROOT/'src/campaign/source_campaign_screen_runtime_tu15.cpp',
            ROOT/'src/runtime/source_execution_calls.cpp']
        for n,path in enumerate(caller_files):
            data=path.read_bytes();(project/f'caller{n}.cpp').write_bytes(data)
            frozen[str(path.relative_to(ROOT))]=hashlib.sha256(data).hexdigest()
        cmake+=f'''find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
add_library(screen_callers OBJECT caller0.cpp caller1.cpp caller2.cpp)
target_include_directories(screen_callers PRIVATE "{ROOT/'include'}" "{ROOT/'src'}" "{ROOT/'third_party/boost-pfr/include'}")
target_link_libraries(screen_callers PRIVATE SDL2::SDL2 Vulkan::Vulkan)
'''
    cmake+='add_custom_target(halo2_tests DEPENDS screen_native screen_missing_tail screen_wrong_ack'+(' screen_callers' if args.compile_callers else '')+')\n'
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),
        '--jobs','2','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
    command+=['--target','screen_callers','--build-only'] if args.callers_only else ['--filter','^screen_']
    paths=[SOURCE,TEST,FIXTURE,ROOT/'include/halo2/native_campaign_screen_lifecycle_tu15.hpp',Path(__file__),*caller_files]
    before={str(path.relative_to(ROOT)):sha(path) for path in paths}
    if any(before[name]!=pin for name,pin in frozen.items()):
        raise ValueError('Source changed while freezing caller validation inputs')
    result=subprocess.run(command,check=False)
    after={str(path.relative_to(ROOT)):sha(path) for path in paths}
    report=dict(passed=result.returncode==0 and before==after,scope=__doc__,command=command,
        compiler=args.compiler,sanitizers=args.sanitizers,compile_callers=args.compile_callers,source_sha256=before,
        caller_compile_only=args.callers_only,validation_not_run=args.callers_only,
        source_unchanged=before==after,owned_scratch=str(scratch),retained_large_artifacts=[],
        reference_fixture_bytes=FIXTURE.stat().st_size,
        source_mutations=['omit final reset word','clear wrong acknowledgement byte'],
        composed_campaign_or_launcher_qualified=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
