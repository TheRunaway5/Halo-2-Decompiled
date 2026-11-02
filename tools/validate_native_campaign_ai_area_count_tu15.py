#!/usr/bin/env python3
"""Owned two-area query/component and actual callback transport qualification."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
CORE='src/campaign/ai/native_campaign_ai_area_count_tu15.cpp'
ITERATOR='src/campaign/ai/native_campaign_authored_area_iterator_tu15.hpp'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    args.output=args.output.resolve()
    args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    project=scratch/'project'
    shutil.copytree(ROOT/'include',project/'include')
    files=[CORE,ITERATOR,'include/halo2/native_campaign_ai_area_count_tu15.hpp',
           'cmake/CampaignNativeAiAreaCount.cmake','tests/native_campaign_ai_area_count_tu15_tests.cpp',
           'tests/fixtures/native_campaign_ai_area_count_tu15.json.gz',
           'tools/test_native_campaign_screen_lifecycle_tu15.py']
    for name in files:
        destination=project/name
        destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(ROOT/name,destination)
    pins={name:sha(ROOT/name)for name in files}
    variants=[
        ('one_area',CORE,'return areas.next()!=0xffffu && areas.next()!=0xffffu;',
         'return areas.next()!=0xffffu;'),
        ('wrong_mode',CORE,'const bool mode=read(memory,row+0x26cu)!=0xffffffffu;',
         'const bool mode=read(memory,row+0x26cu)==0xffffffffu;'),
        ('wrong_actor_stride',CORE,'(actor&0xffffu)*0x888u','(actor&0xffffu)*0x880u'),
        ('wrong_ret4',CORE,'abi.gp[4]=sp+8u;','abi.gp[4]=sp+12u;'),
        ('foreign_caller',CORE,'call.instruction==0x1b0195u','call.instruction!=0u'),
        ('skip_cursor',ITERATOR,'++index_;','index_+=2;'),
        ('skip_inheritance',ITERATOR,'if(inherited==none)break;','if(true)break;'),
        ('wrong_list',ITERATOR,'order_+(read(squad+0x60u,1)?0x60u:0x58u)',
         'order_+(read(squad+0x60u,1)?0x58u:0x60u)'),
    ]
    targets=[]
    for name,path,needle,replacement in variants:
        directory=project/'variants'/name
        directory.mkdir(parents=True)
        for source in (CORE,ITERATOR):
            text=(project/source).read_text()
            if source==path:
                assert needle in text,name
                text=text.replace(needle,replacement)
            (directory/Path(source).name).write_text(text)
        targets.append(name)
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(s in r.stderr for s in ('original area result differs','original ordered reads differ','Missing area input storage','Area input extent differs','GPR contract differs','Unowned callback performed source work')),(r.returncode,r.stdout,r.stderr)
print('Expected original-reference mutation rejection:',r.stderr.strip()[-2000:])
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeCampaignAreaCount LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
include(cmake/CampaignNativeAiAreaCount.cmake)
'''
    for name in targets:
        cmake+=f'''add_executable(area_{name} tests/native_campaign_ai_area_count_tu15_tests.cpp variants/{name}/{Path(CORE).name})
target_include_directories(area_{name} PRIVATE include)
target_compile_features(area_{name} PRIVATE cxx_std_20)
target_link_libraries(area_{name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME area_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:area_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_ai_area_count_tu15.json.gz")
set_tests_properties(area_{name} PROPERTIES TIMEOUT 15)
'''
    (project/'CMakeLists.txt').write_text(cmake)
    results=[]
    for kind,compiler,sanitizers in [('gcc','g++',False),('sanitizers','clang++',True),('clang-controls','clang++',False)]:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),
                 '--scratch-root',str(scratch/'builds'),'--output',str(args.output/kind),
                 '--target','native_campaign_ai_area_count_tu15_tests',
                 '--filter','^(native_campaign_ai_area_count_tu15|area_)$' if kind=='clang-controls' else '^native_campaign_ai_area_count_tu15$',
                 '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
                 '--retain-report','Testing/Temporary/LastTest.log']
        if kind=='clang-controls':
            command[command.index('--filter')+1]='^(native_campaign_ai_area_count_tu15|area_.*)$'
            for name in targets:command+=['--target','area_'+name]
        if sanitizers:command+=['--sanitizers']
        run=subprocess.run(command,check=False)
        results.append(dict(kind=kind,command=command,exit_code=run.returncode))
        if run.returncode:break
    unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items())
    report=dict(passed=len(results)==3 and all(row['exit_code']==0 for row in results) and unchanged,
                source_sha256=pins,source_unchanged=unchanged,results=results,
                negative_controls=targets,owned_scratch=str(scratch),retained_large_artifacts=[],
                actual_world_route_qualified=False,full_campaign_qualified=False)
    (args.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
    return int(not report['passed'])


if __name__=='__main__':
    raise SystemExit(main())
