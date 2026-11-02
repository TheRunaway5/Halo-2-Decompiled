#!/usr/bin/env python3
"""Disposable unchanged-original participant compiler, sanitizer and mutation proof."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--gcc-only',action='store_true');a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    core='src/campaign/native_campaign_transition_participants_tu15.cpp'
    names=[core,'include/halo2/native_campaign_transition_participants_tu15.hpp','include/halo2/native_campaign_transition_request_tu15.hpp','include/halo2/native_tu15_physics_frame.hpp','include/halo2/physics_contact_record.hpp','cmake/CampaignNativeTransitionParticipants.cmake','tests/native_campaign_transition_participants_tu15_tests.cpp','tools/test_native_campaign_screen_lifecycle_tu15.py','tests/fixtures/native_campaign_transition_participants_tu15.json.gz']
    for name in names:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    original=(project/core).read_text();variants={}
    mutations=[
        ('truncated_participant_zero','n<0xe40u','n<0xe3cu',1),
        ('missing_sixteenth_capture','count>=16u','count>=15u',1),
        ('first_identity_match','if(equal){const auto at=','if(equal&&!selected){const auto at=',1),
        ('unsigned_recent_age','signed_value(age)<signed_value(best)','age<best',2),
        ('duplicate_second_equipment','&&(!pass||item!=chosen)','',2),
        ('missing_tag_kind_fallback','if(!kind)kind=read(memory,tag+0x1fcu,1);','if(!kind)kind=0;',1),
        ('missing_spare_reread','if(read(memory,body+0x218u+n*4u)!=none)return read(memory,body+0x218u+n*4u);','const auto item=read(memory,body+0x218u+n*4u);if(item!=none)return item;',1),
        ('missing_scalar_clamp_rounding','asm volatile("flds %1;fistpl %0":"=m"(converted):"m"(single):"memory","st");','converted=_mm_cvttss_si32(_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(single))));',1),
        ('zero_quantity_absent_fields','if(signed_value(read(memory,tag+0x2c0u))>0){','if(signed_value(read(memory,tag+0x2c0u))>0){options.set(output+2u,0,4);',1),
        ('omit_configuration_tail','n<0x90u;n+=4u','n<0x8cu;n+=4u',1),
    ]
    for name,before,after,count in mutations:
        assert original.count(before)==count,(name,original.count(before));path=f'src/campaign/{name}.cpp';(project/path).write_text(original.replace(before,after));variants[name]=path
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('participant ordered','retained participant bytes differ','private participant options differ','participant FP differs','actual spare-equipment result differs','unexpected participant source address','participant completion differs')),(r.returncode,r.stdout,r.stderr)
print('Independent original rejects semantic mutation:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.24)
project(NativeTransitionParticipants LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime STATIC)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeTransitionParticipants.cmake)
add_library(mutation_driver OBJECT tests/native_campaign_transition_participants_tu15_tests.cpp)
target_include_directories(mutation_driver PRIVATE include)
target_compile_features(mutation_driver PRIVATE cxx_std_20)
target_link_libraries(mutation_driver PRIVATE nlohmann_json::nlohmann_json)
'''
    for name,path in variants.items():cmake+=f'''add_executable(policy_{name} $<TARGET_OBJECTS:mutation_driver> {path})
target_include_directories(policy_{name} PRIVATE include)
target_compile_features(policy_{name} PRIVATE cxx_std_20)
add_test(NAME policy_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:policy_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_transition_participants_tu15.json.gz")
'''
    (project/'CMakeLists.txt').write_text(cmake);pins={name:sha(ROOT/name)for name in names};pins[str(Path(__file__).relative_to(ROOT))]=sha(__file__)
    results=[];instrumented=[];configs=[('gcc','g++',False)]if a.gcc_only else [('gcc','g++',False),('clang-controls','clang++',False),('sanitizers','clang++',True)]
    for kind,compiler,sanitizers in configs:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_campaign_transition_participants_tu15_tests','--filter','^(native_campaign_transition_participants_tu15|policy_)'if kind=='clang-controls'else'^native_campaign_transition_participants_tu15$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if kind=='clang-controls':
            for name in variants:command+=['--target','policy_'+name]
        if sanitizers:command+=['--sanitizers']
        status=subprocess.run(command).returncode;results.append(dict(kind=kind,exit_code=status,command=command))
        if sanitizers and status==0:
            commands=json.loads((a.output/kind/'evidence/compile_commands.json').read_text());found=[row.get('command',' '.join(row.get('arguments',[])))for row in commands if row['file'].endswith('/'+core)and'native_campaign_transition_participants_tu15_tests.dir'in row.get('command',' '.join(row.get('arguments',[])))];assert found
            instrumented.append(dict(source=core,all_objects_instrumented=all('-fsanitize=address,undefined'in row for row in found),commands=found))
        if status:break
    staged_match=all(sha(project/name)==pins[name]for name in names)
    report=dict(passed=len(results)==len(configs)and all(r['exit_code']==0 for r in results)and all(r['all_objects_instrumented']for r in instrumented)and staged_match,full_matrix=not a.gcc_only,source_sha256=pins,source_unchanged=all(sha(ROOT/name)==digest for name,digest in pins.items()),staged_source_matches_pins=staged_match,results=results,negative_controls=list(variants)if not a.gcc_only else[],actual_sanitizer_objects=instrumented,lifted_fallback=False,retained_large_artifacts=[])
    (a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
