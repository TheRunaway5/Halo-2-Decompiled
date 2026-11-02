#!/usr/bin/env python3
"""Owned actual-caller awareness binding/reference/compiler qualification."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--normal-only',action='store_true')
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    names=['cmake/CampaignNativeAiAwareness.cmake','cmake/CampaignNativeAiAwarenessBinding.cmake',
        'src/campaign/ai/native_campaign_ai_awareness_tu15.cpp','src/campaign/ai/native_campaign_ai_awareness_tu15_binding.cpp',
        'tests/native_campaign_ai_awareness_tu15_tests.cpp','tests/native_campaign_ai_awareness_tu15_binding_tests.cpp',
        'tests/fixtures/native_campaign_ai_awareness_tu15.json.gz','tools/test_native_campaign_screen_lifecycle_tu15.py',
        'src/campaign/ai/native_campaign_ai_query_tu15.cpp','src/campaign/ai/native_campaign_ai_fighting_count_tu15.cpp']
    for name in names:
        path=project/name;path.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,path)
    source=(project/names[3]).read_text();variants={}
    pairs=[('wrong_input_register','memory,abi.gp[2],abi.gp[1],fp','memory,abi.gp[0],abi.gp[1],fp'),
        ('fresh_fp_defaults','fp{control.x87_control_word,abi.x87_status,control.mxcsr}','fp{0x27fu,0u,0x1f80u}'),
        ('wrong_stack_pop','abi.gp[4]+=ai?12u:8u;','abi.gp[4]+=ai?16u:12u;'),
        ('skip_ret_read','const auto target=returned(memory,abi.gp[4]);','const auto target=ai?0x2af0f5u:0x2af144u;'),
        ('argument_prepeek','const bool ai=call.target==0x27d7d0u;','const bool ai=call.target==0x27d7d0u;(void)returned(memory,abi.gp[4]+4u);'),
        ('omit_status_publication','abi.x87_status=fp.status;','abi.x87_status=abi.x87_status;'),
        ('admit_tail','return !call.tail &&','return true &&')]
    for name,needle,replacement in pairs:
        assert source.count(needle)==1,(name,source.count(needle));variants[name]=source.replace(needle,replacement)
        (project/f'src/campaign/ai/binding_{name}.cpp').write_text(variants[name])
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True,timeout=45)
assert r.returncode!=0 and any(v in r.stderr for v in('awareness binding','Awareness binding','Missing awareness storage','Awareness storage extent differs','Awareness RET failed')),(r.returncode,r.stdout,r.stderr)
print('Expected actual-caller binding rejection:',r.stderr.strip()[-4000:])
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeAwarenessBinding LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
if(HALO2_SANITIZERS)
add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
add_link_options(-fsanitize=address,undefined)
endif()
add_library(halo2_systems OBJECT)
target_include_directories(halo2_systems PRIVATE include)
target_compile_features(halo2_systems PRIVATE cxx_std_20)
include(cmake/CampaignNativeAiAwarenessBinding.cmake)
add_library(binding_driver OBJECT tests/native_campaign_ai_awareness_tu15_binding_tests.cpp)
target_include_directories(binding_driver PRIVATE include)
target_compile_features(binding_driver PRIVATE cxx_std_20)
target_compile_options(binding_driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(binding_driver PRIVATE nlohmann_json::nlohmann_json)
add_library(binding_semantics STATIC src/campaign/ai/native_campaign_ai_awareness_tu15.cpp src/campaign/ai/native_campaign_ai_query_tu15.cpp src/campaign/ai/native_campaign_ai_fighting_count_tu15.cpp)
target_include_directories(binding_semantics PRIVATE include)
target_compile_features(binding_semantics PRIVATE cxx_std_20)
target_compile_options(binding_semantics PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
'''
    for name in variants:
        cmake+=f'''add_executable(binding_{name} src/campaign/ai/binding_{name}.cpp $<TARGET_OBJECTS:binding_driver>)
target_include_directories(binding_{name} PRIVATE include)
target_compile_features(binding_{name} PRIVATE cxx_std_20)
target_link_libraries(binding_{name} PRIVATE binding_semantics)
add_test(NAME binding_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:binding_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_ai_awareness_tu15.json.gz")
'''
    (project/'CMakeLists.txt').write_text(cmake);pins={n:sha(ROOT/n)for n in names}
    for n in['include/halo2/native_campaign_ai_awareness_tu15_binding.hpp','include/halo2/native_campaign_ai_awareness_tu15.hpp',
        'include/halo2/physics_contact_runtime.hpp','include/halo2/native_tu15_physics_frame.hpp',str(Path(__file__).relative_to(ROOT))]:pins[n]=sha(ROOT/n)
    results=[];matrix=[('normal','g++',False)]if args.normal_only else [('normal','g++',False),('sanitizers','clang++',True),('clang-controls','clang++',False)]
    for kind,compiler,san in matrix:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
            '--output',str(args.output/kind),'--target','native_campaign_ai_awareness_tu15_binding_tests',
            '--filter','^(native_campaign_ai_awareness_tu15_binding|binding_)'if kind=='clang-controls'else'^native_campaign_ai_awareness_tu15_binding$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
        if kind=='clang-controls':
            for name in variants:command+=['--target','binding_'+name]
        if san:command+=['--sanitizers']
        result=subprocess.run(command,check=False);results.append(dict(kind=kind,exit_code=result.returncode,command=command))
        if result.returncode:break
    report=dict(passed=len(results)==len(matrix)and all(r['exit_code']==0 for r in results),source_hashes=pins,
        sources_unchanged=all(sha(ROOT/n)==pin for n,pin in pins.items()),negative_controls=list(variants),results=results,
        owned_scratch=str(scratch),retained_large_artifacts=[])
    (args.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])

if __name__=='__main__':raise SystemExit(main())
