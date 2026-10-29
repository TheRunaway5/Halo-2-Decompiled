#!/usr/bin/env python3
"""Disposable production tracking-allocation compiler/FP/mutation proof."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--gcc-only',action='store_true');a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    core='src/campaign/ai/native_campaign_ai_tracking_allocate_tu15.cpp'
    names=['cmake/CampaignNativeAiTrackingAllocate.cmake','cmake/NativeTu15DataArrayAllocate.cmake','cmake/NativeTu15DataArrayIteration.cmake','cmake/NativeTu15DataArrayDelete.cmake',core,'src/campaign/ai/native_campaign_ai_combat_reset_tu15.cpp','src/campaign/ai/native_campaign_ai_perception_refs_tu15.cpp','src/core/native_tu15_data_array_allocate.cpp','src/core/native_tu15_data_array_iteration.cpp','src/core/native_tu15_data_array_delete.cpp','tests/native_campaign_ai_tracking_allocate_tu15_tests.cpp','tests/native_tu15_data_array_allocate_tests.cpp','tools/test_native_campaign_screen_lifecycle_tu15.py','tests/fixtures/native_campaign_ai_tracking_allocate_tu15.json.gz']
    for name in names:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    text=(project/core).read_text();variants={}
    for name,before,after in [
        ('priority_inclusive','if(candidate<priority){','if(candidate<=priority){'),
        ('timestamp_last_tie','better=signed32(read(m,position))<best_time;','better=signed32(read(m,position))<=best_time;'),
        ('skip_retirement','if(read(m,member+0x14u)!=none)native_campaign_ai_tracking_retire_tu15(m,actor,current);','/* omitted actual retirement */'),
        ('wrong_pass_policy','retire=pass==1&&!equal;','retire=pass==1&&equal;'),
        ('wrong_numeric_rounding','flds %1; fistpl %0','flds %1; fisttpl %0'),
        ('missing_extension_flag','write(m,extension+0x6cu,1,1);','write(m,extension+0x6cu,0,1);'),
        ('skip_actor_filter','if(state.active_only&&!read(m,state.storage.current+9u,1))continue;','if(state.active_only&&read(m,state.storage.current+9u,1))continue;')]:
        assert text.count(before)==1,(name,text.count(before));path=f'src/campaign/ai/{name}.cpp';(project/path).write_text(text.replace(before,after));variants[name]=path
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('tracking result differs','ordered tracking allocation reads differ','ordered tracking allocation stores differ','retained tracking allocation bytes differ','completed tracking FP differs','private actor iterator differs')),(r.returncode,r.stdout,r.stderr)
print('Independent original rejects semantic mutation:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.24)
project(NativeTrackingAllocate LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_systems STATIC src/campaign/ai/native_campaign_ai_combat_reset_tu15.cpp src/campaign/ai/native_campaign_ai_perception_refs_tu15.cpp)
target_include_directories(halo2_systems PRIVATE include)
target_compile_features(halo2_systems PRIVATE cxx_std_20)
include(cmake/CampaignNativeAiTrackingAllocate.cmake)
add_library(mutation_driver OBJECT tests/native_campaign_ai_tracking_allocate_tu15_tests.cpp)
target_include_directories(mutation_driver PRIVATE include)
target_compile_features(mutation_driver PRIVATE cxx_std_20)
target_link_libraries(mutation_driver PRIVATE nlohmann_json::nlohmann_json)
add_library(mutation_common OBJECT src/campaign/ai/native_campaign_ai_combat_reset_tu15.cpp src/campaign/ai/native_campaign_ai_perception_refs_tu15.cpp)
target_include_directories(mutation_common PRIVATE include)
target_compile_features(mutation_common PRIVATE cxx_std_20)
'''
    for name,path in variants.items():cmake+=f'''add_executable(tracking_{name} $<TARGET_OBJECTS:mutation_driver> $<TARGET_OBJECTS:mutation_common> {path})
target_include_directories(tracking_{name} PRIVATE include)
target_compile_features(tracking_{name} PRIVATE cxx_std_20)
target_compile_options(tracking_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(tracking_{name} PRIVATE halo2_native_tu15_data_array_delete)
add_test(NAME tracking_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:tracking_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_ai_tracking_allocate_tu15.json.gz")
'''
    (project/'CMakeLists.txt').write_text(cmake)
    pins={name:sha(ROOT/name)for name in names}
    for header in ['native_campaign_ai_tracking_allocate_tu15.hpp','native_campaign_ai_combat_reset_tu15.hpp','native_campaign_ai_perception_refs_tu15.hpp','native_tu15_physics_frame.hpp','native_tu15_data_array_allocate.hpp','native_tu15_data_array_iteration.hpp','native_tu15_data_array_delete.hpp','physics_contact_record.hpp']:pins['include/halo2/'+header]=sha(ROOT/'include/halo2'/header)
    pins[str(Path(__file__).relative_to(ROOT))]=sha(__file__);results=[];instrumented=[]
    configs=[('gcc','g++',False)]if a.gcc_only else [('gcc','g++',False),('clang-controls','clang++',False),('sanitizers','clang++',True)]
    for kind,compiler,san in configs:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_campaign_ai_tracking_allocate_tu15_tests','--filter','^(native_campaign_ai_tracking_allocate_tu15|tracking_)'if kind=='clang-controls'else'^native_campaign_ai_tracking_allocate_tu15$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if kind=='clang-controls':
            for name in variants:command+=['--target','tracking_'+name]
        if san:command+=['--sanitizers']
        status=subprocess.run(command).returncode;results.append(dict(kind=kind,exit_code=status,command=command))
        if san:
            commands=json.loads((a.output/kind/'evidence/compile_commands.json').read_text())
            for path in [core,'src/campaign/ai/native_campaign_ai_combat_reset_tu15.cpp','src/campaign/ai/native_campaign_ai_perception_refs_tu15.cpp','src/core/native_tu15_data_array_allocate.cpp','src/core/native_tu15_data_array_iteration.cpp','src/core/native_tu15_data_array_delete.cpp']:
                found=[c.get('command',' '.join(c.get('arguments',[])))for c in commands if c['file'].endswith('/'+path)and(path.startswith('src/core/')or'native_campaign_ai_tracking_allocate_tu15_tests.dir'in c.get('command',' '.join(c.get('arguments',[]))))];assert found;instrumented.append(dict(source=path,all_objects_instrumented=all('-fsanitize=address,undefined'in c for c in found),commands=found))
        if status:break
    report=dict(passed=len(results)==len(configs)and all(r['exit_code']==0 for r in results)and all(x['all_objects_instrumented']for x in instrumented),full_matrix=not a.gcc_only,source_sha256=pins,source_unchanged=all(sha(ROOT/n)==h for n,h in pins.items()),results=results,negative_controls=list(variants)if not a.gcc_only else[],actual_sanitizer_objects=instrumented,lifted_fallback=False,retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
