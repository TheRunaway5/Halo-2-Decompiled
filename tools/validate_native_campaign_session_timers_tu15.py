#!/usr/bin/env python3
"""Disposable unchanged-original retained session support compiler, sanitizer and mutation proof."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--gcc-only',action='store_true');a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    core='src/campaign/native_campaign_session_timers_tu15.cpp'
    names=[core,'include/halo2/native_campaign_session_timers_tu15.hpp','include/halo2/native_campaign_transition_request_tu15.hpp','include/halo2/physics_contact_record.hpp','cmake/CampaignNativeSessionTimers.cmake','tests/native_campaign_session_timers_tu15_tests.cpp','tests/native_campaign_session_timers_test_support.hpp','tools/test_native_campaign_screen_lifecycle_tu15.py','tests/fixtures/native_campaign_session_timers_tu15.json.gz']
    for name in names:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    original=(project/core).read_text();variants={}
    mutations=[
        ('missing_fifth_group','group<5u','group<4u',1),
        ('wrong_all_filter','filter!=0xffffffffu','filter!=0u',1),
        ('wrong_state_guard','read(memory,state)!=3u','read(memory,state)!=2u',1),
        ('wrong_slot_stride','slot*0x98u','slot*0x94u',1),
        ('wrong_cached_cell','tick=read(memory,0x51d04cu);','tick=read(memory,0x51d050u);',1),
        ('truncated_timestamp','write(memory,state+4u,tick,0x77830u,observe);','write(memory,state+4u,tick&0xffffu,0x77830u,observe);',1),
        ('late_clock_producer','producer(cell);\n            tick=read(memory,cell);','tick=read(memory,cell);\n            producer(cell);',1),
        ('supplied_missing_producer','if(!producer)throw NativeCampaignSessionTimerClockRequiredTu15(cell);\n            producer(cell);','if(producer)producer(cell);',1),
        ('wrong_filtered_return','std::uint32_t result=filter;','std::uint32_t result=0;',1),
        ('wrong_idle_return','if(group==4u)result=slot_offset;','if(group==4u)result=slot_offset+1u;',1),
        ('wrong_retired_return','if(group==4u)result=tick;','if(group==4u)result=slot_offset;',1),
    ]
    for name,before,after,count in mutations:
        assert original.count(before)==count,(name,original.count(before));path=f'src/campaign/{name}.cpp';(project/path).write_text(original.replace(before,after));variants[name]=path
    late=original.replace('write(memory,state,4,0x77818u,observe);',';').replace('write(memory,state+4u,tick,0x77830u,observe);','write(memory,state,4,0x77818u,observe);write(memory,state+4u,tick,0x77830u,observe);')
    path='src/campaign/late_state_publication.cpp';(project/path).write_text(late);variants['late_state_publication']=path
    once=original.replace('for(std::uint32_t group=0;group<5u;++group){','bool sampled=false;for(std::uint32_t group=0;group<5u;++group){').replace('producer(cell);','if(!sampled){producer(cell);sampled=true;}')
    path='src/campaign/single_clock_sample.cpp';(project/path).write_text(once);variants['single_clock_sample']=path
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('session timers ordered','retained session timers bytes differ','unexpected session timers source address','session timers completion differs','session timers continuation value differs','clock input phase differs','platform clock publication differs','recorded platform clock replay differs','required clock owner','unexpected clock input consumption')),(r.returncode,r.stdout,r.stderr)
print('Independent original rejects semantic mutation:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.24)
project(NativeSessionTimers LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime STATIC)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeSessionTimers.cmake)
add_library(mutation_driver OBJECT tests/native_campaign_session_timers_tu15_tests.cpp)
target_include_directories(mutation_driver PRIVATE include)
target_compile_features(mutation_driver PRIVATE cxx_std_20)
target_link_libraries(mutation_driver PRIVATE nlohmann_json::nlohmann_json)
'''
    for name,path in variants.items():cmake+=f'''add_executable(policy_{name} $<TARGET_OBJECTS:mutation_driver> {path})
target_include_directories(policy_{name} PRIVATE include)
target_compile_features(policy_{name} PRIVATE cxx_std_20)
add_test(NAME policy_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:policy_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_session_timers_tu15.json.gz")
'''
    (project/'CMakeLists.txt').write_text(cmake);pins={name:sha(ROOT/name)for name in names};pins[str(Path(__file__).relative_to(ROOT))]=sha(__file__)
    results=[];instrumented=[];configs=[('gcc','g++',False)]if a.gcc_only else [('gcc','g++',False),('clang-controls','clang++',False),('sanitizers','clang++',True)]
    for kind,compiler,sanitizers in configs:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_campaign_session_timers_tu15_tests','--filter','^(native_campaign_session_timers_tu15|policy_)'if kind=='clang-controls'else'^native_campaign_session_timers_tu15$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if kind=='clang-controls':
            for name in variants:command+=['--target','policy_'+name]
        if sanitizers:command+=['--sanitizers']
        status=subprocess.run(command).returncode;results.append(dict(kind=kind,exit_code=status,command=command))
        if sanitizers and status==0:
            commands=json.loads((a.output/kind/'evidence/compile_commands.json').read_text());found=[row.get('command',' '.join(row.get('arguments',[])))for row in commands if row['file'].endswith('/'+core)and'native_campaign_session_timers_tu15_tests.dir'in row.get('command',' '.join(row.get('arguments',[])))];assert found
            instrumented.append(dict(source=core,all_objects_instrumented=all('-fsanitize=address,undefined'in row for row in found),commands=found))
        if status:break
    staged_match=all(sha(project/name)==pins[name]for name in names)
    report=dict(passed=len(results)==len(configs)and all(r['exit_code']==0 for r in results)and all(r['all_objects_instrumented']for r in instrumented)and staged_match,full_matrix=not a.gcc_only,source_sha256=pins,source_unchanged=all(sha(ROOT/name)==digest for name,digest in pins.items()),staged_source_matches_pins=staged_match,results=results,negative_controls=list(variants)if not a.gcc_only else[],actual_sanitizer_objects=instrumented,lifted_fallback=False,retained_large_artifacts=[])
    (a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
