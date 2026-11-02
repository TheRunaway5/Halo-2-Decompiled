#!/usr/bin/env python3
"""Disposable genuine-caller timer transport, original-state and negative-control proof."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--gcc-only',action='store_true');args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    core='src/campaign/native_campaign_session_timers_tu15.cpp';binding='src/campaign/native_campaign_session_timers_tu15_binding.cpp'
    names=[core,binding,'include/halo2/native_campaign_session_timers_tu15.hpp','include/halo2/native_campaign_session_timers_tu15_binding.hpp',
        'include/halo2/native_campaign_transition_request_tu15.hpp','include/halo2/physics_contact_runtime.hpp','include/halo2/physics_contact_record.hpp',
        'cmake/CampaignNativeSessionTimers.cmake','cmake/CampaignNativeSessionTimersBinding.cmake',
        'tests/native_campaign_session_timers_tu15_tests.cpp','tests/native_campaign_session_timers_tu15_binding_tests.cpp','tests/native_campaign_session_timers_test_support.hpp',
        'tools/test_native_campaign_screen_lifecycle_tu15.py','tests/fixtures/native_campaign_session_timers_tu15.json.gz','tests/fixtures/native_campaign_session_timers_callers_tu15.json.gz']
    for name in names:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    original=(project/binding).read_text();variants={}
    mutations=[
        ('wrong_eax_slot','call.instruction==0x77bd3u?abi.gp[0]','call.instruction==0x77bd3u?abi.gp[6]'),
        ('wrong_edi_slot','call.instruction==0x78204u?abi.gp[7]','call.instruction==0x78204u?abi.gp[6]'),
        ('wrong_group_filter','call.instruction==0x78204u?abi.gp[5]:0xffffffffu','0xffffffffu'),
        ('discarded_real_return','abi.gp[0]=native_campaign_session_retire_timers_value_tu15','(void)native_campaign_session_retire_timers_value_tu15'),
        ('argument_prepeek','calls.fetch_add(1,std::memory_order_relaxed);','calls.fetch_add(1,std::memory_order_relaxed);(void)returned(memory,abi.gp[4]+4u);'),
        ('wrong_ret_cleanup','abi.gp[4]+=12u;','abi.gp[4]+=8u;'),
        ('supplied_continuation','if(target!=call.instruction+5u)throw NativeCampaignSessionTimerReturnRequiredTu15(target);','(void)target;'),
        ('damaged_ecx','abi.gp[4]+=12u;','abi.gp[4]+=12u;abi.gp[1]=0;'),
        ('damaged_fp','abi.gp[4]+=12u;','abi.gp[4]+=12u;abi.x87_status^=1u;'),
        ('omitted_observer','memory,filter,slot,producer,observe)','memory,filter,slot,producer,{})'),
    ]
    for name,before,after in mutations:
        assert original.count(before)==1,(name,original.count(before));path=f'src/campaign/{name}.cpp';(project/path).write_text(original.replace(before,after));variants[name]=(path,core)
    before='    abi.gp[0]=native_campaign_session_retire_timers_value_tu15(memory,filter,slot,producer,observe);\n    const auto target=returned(memory,abi.gp[4]);'
    after='    const auto target=returned(memory,abi.gp[4]);\n    abi.gp[0]=native_campaign_session_retire_timers_value_tu15(memory,filter,slot,producer,observe);'
    assert original.count(before)==1;path='src/campaign/early_ret_read.cpp';(project/path).write_text(original.replace(before,after));variants['early_ret_read']=(path,core)
    text=(project/core).read_text();before='if(group==4u)result=slot_offset;';assert text.count(before)==1
    path='src/campaign/idle_slot_instead_of_offset.cpp';(project/path).write_text(text.replace(before,'if(group==4u)result=slot;'));variants['idle_slot_instead_of_offset']=(binding,path)
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(value in r.stderr for value in ('timer binding','timer observer','Unsupported timer','Timer binding')),(r.returncode,r.stdout,r.stderr)
print('Independent actual caller rejects mutation:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.24)
project(NativeSessionTimerCaller LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime STATIC)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeSessionTimersBinding.cmake)
add_library(binding_mutation_driver OBJECT tests/native_campaign_session_timers_tu15_binding_tests.cpp)
target_include_directories(binding_mutation_driver PRIVATE include)
target_compile_features(binding_mutation_driver PRIVATE cxx_std_20)
target_link_libraries(binding_mutation_driver PRIVATE nlohmann_json::nlohmann_json)
'''
    for name,sources in variants.items():
        cmake+=f'''add_executable(policy_{name} $<TARGET_OBJECTS:binding_mutation_driver> {' '.join(sources)})
target_include_directories(policy_{name} PRIVATE include)
target_compile_features(policy_{name} PRIVATE cxx_std_20)
add_test(NAME policy_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:policy_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_session_timers_callers_tu15.json.gz")
'''
    (project/'CMakeLists.txt').write_text(cmake);pins={name:sha(ROOT/name)for name in names};pins[str(Path(__file__).relative_to(ROOT))]=sha(__file__)
    configs=[('gcc','g++',False)]if args.gcc_only else [('gcc','g++',False),('clang-controls','clang++',False),('sanitizers','clang++',True)]
    results=[];instrumented=[]
    for kind,compiler,sanitizers in configs:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/kind),
            '--target','native_campaign_session_timers_tu15_tests','--target','native_campaign_session_timers_tu15_binding_tests',
            '--filter','^(native_campaign_session_timers_tu15(_binding)?|policy_)'if kind=='clang-controls'else'^native_campaign_session_timers_tu15(_binding)?$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
            '--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if kind=='clang-controls':
            for name in variants:command+=['--target','policy_'+name]
        if sanitizers:command+=['--sanitizers']
        status=subprocess.run(command).returncode;results.append(dict(kind=kind,exit_code=status,command=command))
        if sanitizers and not status:
            commands=json.loads((args.output/kind/'evidence/compile_commands.json').read_text())
            for source in(core,binding):
                found=[row.get('command',' '.join(row.get('arguments',[])))for row in commands if row['file'].endswith('/'+source)and'native_campaign_session_timers_tu15_binding_tests.dir'in row.get('command',' '.join(row.get('arguments',[])))];assert found
                instrumented.append(dict(source=source,all_objects_instrumented=all('-fsanitize=address,undefined'in row for row in found),commands=found))
        if status:break
    staged=all(sha(project/name)==pins[name]for name in names)
    report=dict(passed=len(results)==len(configs)and all(row['exit_code']==0 for row in results)and all(row['all_objects_instrumented']for row in instrumented)and staged,
        source_sha256=pins,source_unchanged=all(sha(ROOT/name)==value for name,value in pins.items()),staged_source_matches_pins=staged,full_matrix=not args.gcc_only,
        results=results,negative_controls=list(variants)if not args.gcc_only else[],actual_sanitizer_objects=instrumented,lifted_fallback=False,retained_large_artifacts=[])
    (args.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
