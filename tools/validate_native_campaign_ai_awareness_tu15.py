#!/usr/bin/env python3
"""Owned awareness physical-reference/compiler/sanitizer/mutation qualification."""
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
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir()
    shutil.copytree(ROOT/'include',project/'include')
    names=['cmake/CampaignNativeAiAwareness.cmake','src/campaign/ai/native_campaign_ai_awareness_tu15.cpp',
        'tests/native_campaign_ai_awareness_tu15_tests.cpp','tests/fixtures/native_campaign_ai_awareness_tu15.json.gz',
        'tools/test_native_campaign_screen_lifecycle_tu15.py','src/campaign/ai/native_campaign_ai_query_tu15.cpp',
        'src/campaign/ai/native_campaign_ai_fighting_count_tu15.cpp']
    for name in names:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    core=(project/names[1]).read_text()
    pairs=[
        ('wrong_awareness_state','output+0x1cu,3);','output+0x1cu,2);',1),
        ('wrong_seconds','std::bit_cast<std::int32_t>(seconds_bits)','std::int32_t(0x3f800000u)',1),
        ('clamp_negative_tick_rate','"r"(tick_rate),"x"(seconds)','"r"(tick_rate<0?0:tick_rate),"x"(seconds)',1),
        ('clock_before_state','publish(memory,observe,0x27d9a6u,output+0x1cu,3);\n                            const auto ticks=duration(short_value(read(memory,clock+2u,2)),seconds);','const auto ticks=duration(short_value(read(memory,clock+2u,2)),seconds);\n                            publish(memory,observe,0x27d9a6u,output+0x1cu,3);',1),
        ('repeat_clump','if(visited&bit)return false;','if(false)return false;',1),
        ('skip_group_ancestor','parent=read(memory,record(group_rows,parent,0x38u)+0x10u);','parent=none;',1),
        ('admit_object_group','if(observers&0xc0000000u)return;','if((observers&0xc0000000u)==0xc0000000u)return;',1),
        ('omit_fistp_status','asm volatile("flds %1;fistpl %0":"=m"(ticks):"m"(single):"memory","st");','ticks=tick_rate*5;',1),
        ('stale_property_pool','const auto pool=read(memory,0x50eef0u),records=read(memory,pool+0x44u),output=','const auto pool=properties,records=property_rows,output=',1)]
    variants=[]
    for name,needle,replacement,count in pairs:
        assert core.count(needle)==count,(name,core.count(needle))
        (project/f'src/campaign/ai/{name}.cpp').write_text(core.replace(needle,replacement));variants.append(name)
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True,timeout=45)
assert r.returncode!=0 and any(v in r.stderr for v in ('retained awareness bytes differ','awareness ordered reads differ','awareness ordered store differs','awareness store count differs','awareness completion differs','awareness FP differs','actual awareness fault operand differs','Missing awareness storage','Awareness storage extent differs')),(r.returncode,r.stdout,r.stderr)
print('Expected physical-original awareness mutation rejection:',r.stderr.strip()[-4000:])
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(CampaignNativeAiAwareness LANGUAGES CXX)
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
include(cmake/CampaignNativeAiAwareness.cmake)
add_library(awareness_driver OBJECT tests/native_campaign_ai_awareness_tu15_tests.cpp)
target_include_directories(awareness_driver PRIVATE include)
target_compile_features(awareness_driver PRIVATE cxx_std_20)
target_compile_options(awareness_driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(awareness_driver PRIVATE nlohmann_json::nlohmann_json)
add_library(awareness_resolver STATIC src/campaign/ai/native_campaign_ai_query_tu15.cpp src/campaign/ai/native_campaign_ai_fighting_count_tu15.cpp)
target_include_directories(awareness_resolver PRIVATE include)
target_compile_features(awareness_resolver PRIVATE cxx_std_20)
'''
    for name in variants:
        cmake+=f'''add_executable(awareness_{name} src/campaign/ai/{name}.cpp $<TARGET_OBJECTS:awareness_driver>)
target_include_directories(awareness_{name} PRIVATE include)
target_compile_features(awareness_{name} PRIVATE cxx_std_20)
target_compile_options(awareness_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(awareness_{name} PRIVATE awareness_resolver)
add_test(NAME awareness_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:awareness_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_ai_awareness_tu15.json.gz")
'''
    (project/'CMakeLists.txt').write_text(cmake)
    pins={name:sha(ROOT/name)for name in names}
    for name in ['include/halo2/native_campaign_ai_awareness_tu15.hpp','include/halo2/native_campaign_ai_query_tu15.hpp',
                 'include/halo2/physics_contact_record.hpp','include/halo2/native_tu15_physics_frame.hpp',str(Path(__file__).relative_to(ROOT))]:pins[name]=sha(ROOT/name)
    results=[]
    matrix=[('normal','g++',False)]if args.normal_only else [('normal','g++',False),('sanitizers','clang++',True),('clang-controls','clang++',False)]
    for kind,compiler,sanitizers in matrix:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
            '--output',str(args.output/kind),'--target','native_campaign_ai_awareness_tu15_tests',
            '--filter','^(native_campaign_ai_awareness_tu15|awareness_)'if kind=='clang-controls'else'^native_campaign_ai_awareness_tu15$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
        if kind=='clang-controls':
            for name in variants:command+=['--target','awareness_'+name]
        if sanitizers:command+=['--sanitizers']
        run=subprocess.run(command,check=False);results.append(dict(kind=kind,exit_code=run.returncode,command=command))
        if run.returncode:break
    report=dict(passed=len(results)==len(matrix)and all(r['exit_code']==0 for r in results),source_hashes=pins,
        source_unchanged=all(sha(ROOT/n)==pin for n,pin in pins.items()),results=results,negative_controls=variants,
        owned_scratch=str(scratch),retained_large_artifacts=[])
    (args.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
    return int(not report['passed'])

if __name__=='__main__':raise SystemExit(main())
