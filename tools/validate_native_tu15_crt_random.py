#!/usr/bin/env python3
"""Disposable unchanged-original retained CRT random compiler, sanitizer and mutation proof."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--gcc-only',action='store_true');a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    core='src/platform/native_tu15_crt_random.cpp'
    names=[core,'include/halo2/native_tu15_crt_random.hpp','include/halo2/physics_contact_record.hpp','cmake/NativeTu15CrtRandom.cmake','tests/native_tu15_crt_random_tests.cpp','tools/test_native_campaign_screen_lifecycle_tu15.py','tests/fixtures/native_tu15_crt_random.json.gz']
    for name in names:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    original=(project/core).read_text();variants={}
    mutations=[
        ('wrong_elevated_threshold','fs+0x24u,1)>=2u','fs+0x24u,1)>=3u',1),
        ('supplied_fatal_target','const auto owner=read(memory,0x41b758u);','const auto owner=0x8000005fu;',1),
        ('ignore_tls_context','if(read(memory,thread+0x28u)){','if(read(memory,thread+0x28u)&&false){',1),
        ('wrong_tls_stride','table+index*4u','table+index*8u',1),
        ('wrong_tls_holder','holder=read(memory,table+index*4u)+0xcu;','holder=read(memory,table+index*4u)+8u;',1),
        ('wrong_multiplier','0x343fdu','0x343ffu',1),
        ('wide_random_result','&0x7fffu','&0xffffu',1),
        ('missing_seed_publication','write(memory,context+0x14u,next);',';',1),
        ('late_seed_reread','return (next>>16u)&0x7fffu;','return (read(memory,context+0x14u)>>16u)&0x7fffu;',1),
    ]
    for name,before,after,count in mutations:
        assert original.count(before)==count,(name,original.count(before));path=f'src/platform/{name}.cpp';(project/path).write_text(original.replace(before,after));variants[name]=path
    before='if(!context)throw NativeTu15CrtRandomRequiredOwner(NativeTu15CrtRandomOwnerReason::context_allocation,\n        0x3301c4u,0x332383u,{1u,0x88u},2);'
    assert original.count(before)==1;path='src/platform/supplied_context.cpp';(project/path).write_text(original.replace(before,'if(!context)return 1u;'));variants['supplied_context']=path
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('CRT random ordered','retained CRT random bytes differ','unexpected CRT random source address','CRT random completion differs','CRT return differs','CRT required owner','CRT owner arguments differ')),(r.returncode,r.stdout,r.stderr)
print('Independent original rejects semantic mutation:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.24)
project(NativeCrtRandom LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime STATIC)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/NativeTu15CrtRandom.cmake)
add_library(mutation_driver OBJECT tests/native_tu15_crt_random_tests.cpp)
target_include_directories(mutation_driver PRIVATE include)
target_compile_features(mutation_driver PRIVATE cxx_std_20)
target_link_libraries(mutation_driver PRIVATE nlohmann_json::nlohmann_json)
'''
    for name,path in variants.items():cmake+=f'''add_executable(policy_{name} $<TARGET_OBJECTS:mutation_driver> {path})
target_include_directories(policy_{name} PRIVATE include)
target_compile_features(policy_{name} PRIVATE cxx_std_20)
add_test(NAME policy_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:policy_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_tu15_crt_random.json.gz")
'''
    (project/'CMakeLists.txt').write_text(cmake);pins={name:sha(ROOT/name)for name in names};pins[str(Path(__file__).relative_to(ROOT))]=sha(__file__)
    results=[];instrumented=[];configs=[('gcc','g++',False)]if a.gcc_only else [('gcc','g++',False),('clang-controls','clang++',False),('sanitizers','clang++',True)]
    for kind,compiler,sanitizers in configs:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_tu15_crt_random_tests','--filter','^(native_tu15_crt_random|policy_)'if kind=='clang-controls'else'^native_tu15_crt_random$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if kind=='clang-controls':
            for name in variants:command+=['--target','policy_'+name]
        if sanitizers:command+=['--sanitizers']
        status=subprocess.run(command).returncode;results.append(dict(kind=kind,exit_code=status,command=command))
        if sanitizers and status==0:
            commands=json.loads((a.output/kind/'evidence/compile_commands.json').read_text());found=[row.get('command',' '.join(row.get('arguments',[])))for row in commands if row['file'].endswith('/'+core)and'native_tu15_crt_random_tests.dir'in row.get('command',' '.join(row.get('arguments',[])))];assert found
            instrumented.append(dict(source=core,all_objects_instrumented=all('-fsanitize=address,undefined'in row for row in found),commands=found))
        if status:break
    staged_match=all(sha(project/name)==pins[name]for name in names)
    report=dict(passed=len(results)==len(configs)and all(r['exit_code']==0 for r in results)and all(r['all_objects_instrumented']for r in instrumented)and staged_match,full_matrix=not a.gcc_only,source_sha256=pins,source_unchanged=all(sha(ROOT/name)==digest for name,digest in pins.items()),staged_source_matches_pins=staged_match,results=results,negative_controls=list(variants)if not a.gcc_only else[],actual_sanitizer_objects=instrumented,lifted_fallback=False,retained_large_artifacts=[])
    (a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
