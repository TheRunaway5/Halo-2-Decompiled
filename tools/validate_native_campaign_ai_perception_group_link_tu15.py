#!/usr/bin/env python3
"""Disposable compiler/sanitizer/mutation proof for whole perception owners."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    names=['cmake/CampaignNativeAiPerceptionGroupLink.cmake','cmake/CampaignNativeAiPerceptionGroupPressure.cmake','cmake/CampaignNativeAiPerceptionClassify.cmake','cmake/CampaignNativeAiPerceptionGroupActor.cmake','cmake/CampaignNativeAiPerceptionGroupInitialize.cmake','cmake/CampaignNativeAiPerceptionGroupStorage.cmake','cmake/NativeTu15DataArrayIteration.cmake','cmake/NativeTu15DataArrayAllocate.cmake','cmake/NativeTu15DataArrayDelete.cmake','src/campaign/ai/native_campaign_ai_perception_group_link_tu15.cpp','src/campaign/ai/native_campaign_ai_perception_classify_tu15.cpp','src/campaign/ai/native_campaign_ai_perception_group_pressure_tu15.cpp','src/campaign/ai/native_campaign_ai_perception_group_actor_tu15.cpp','src/campaign/ai/native_campaign_ai_clump_retirement_tu15.cpp','src/campaign/ai/native_campaign_ai_clump_callbacks_tu15.cpp','src/campaign/ai/native_campaign_ai_perception_group_initialize_tu15.cpp','src/campaign/ai/native_campaign_object_world_point_tu15.cpp','src/campaign/ai/native_campaign_ai_perception_group_storage_tu15.cpp','src/core/native_tu15_data_array_iteration.cpp','src/core/native_tu15_data_array_allocate.cpp','src/core/native_tu15_data_array_delete.cpp','tests/native_campaign_ai_perception_group_link_tu15_tests.cpp','tests/native_tu15_data_array_allocate_tests.cpp','tests/native_campaign_ai_perception_classify_tu15_tests.cpp','tests/native_campaign_ai_perception_group_actor_tu15_tests.cpp','tests/native_campaign_ai_perception_group_initialize_tu15_tests.cpp','tests/native_campaign_ai_perception_group_storage_tu15_tests.cpp','tools/test_native_campaign_screen_lifecycle_tu15.py','tests/fixtures/native_campaign_ai_perception_group_link_tu15.json.gz','tests/fixtures/native_campaign_ai_perception_group_pressure_focus_tu15.json.gz']
    for name in names:
        path=project/name;path.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,path)
    top='src/campaign/ai/native_campaign_ai_perception_group_link_tu15.cpp';pressure='src/campaign/ai/native_campaign_ai_perception_group_pressure_tu15.cpp';core=(project/top).read_text();pressure_core=(project/pressure).read_text();variants={}
    for name,needle,replacement in [('missing_generation_increment','write(m,0x4eabccu,generation+1u);','write(m,0x4eabccu,generation);'),('missing_caller_priority','write(m,rows+(group&0xffffu)*0xc4u+0xcu,priority,2);','write(m,rows+(group&0xffffu)*0xc4u+0xcu,0,2);'),('skip_first_property_match','if(owner==actor)return current;','if(owner==actor&&property==none)return current;'),('missing_group_fanout','native_campaign_ai_perception_group_link_actors_tu15(m,record,group,fp,occupied_x87);','(void)occupied_x87;'),('generation_gate_stuck','write(m,0x4eabc8u,0,1);','write(m,0x4eabc8u,1,1);')]:
        assert core.count(needle)==1;(project/f'src/campaign/ai/{name}.cpp').write_text(core.replace(needle,replacement));variants[name]=('top','native_campaign_ai_perception_group_link_tu15.json.gz')
    for name,needle,replacement in [('wrong_prune','if(priority<minimum_priority)','if(priority>minimum_priority)'),('last_tie','if(greater(score,best_score))','if(!greater(best_score,score))'),('missing_lower_clamp','if(greater(scalar(0),value))return scalar(0);','/* omitted actual lower clamp */'),('missing_count','count=std::uint16_t(count+1u);','count=0;')]:
        assert pressure_core.count(needle)==1;(project/f'src/campaign/ai/{name}.cpp').write_text(pressure_core.replace(needle,replacement));variants[name]=('pressure','native_campaign_ai_perception_group_pressure_focus_tu15.json.gz')
    needle='const auto group=native_campaign_ai_perception_group_allocate_tu15(memory,clump,signed16(request.priority()),fp);';assert pressure_core.count(needle)==1;stale=pressure_core.replace(needle,'const auto cached_priority=request.priority();const auto group=native_campaign_ai_perception_group_allocate_tu15(memory,clump,signed16(cached_priority),fp);');needle='const auto priority=request.priority();';assert stale.count(needle)==1;(project/'src/campaign/ai/stale_request.cpp').write_text(stale.replace(needle,'const auto priority=cached_priority;'));variants['stale_request']=('pressure','native_campaign_ai_perception_group_pressure_focus_tu15.json.gz')
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('whole perception result differs','ordered whole perception reads differ','ordered whole perception stores differ','retained whole perception bytes differ','classifier output admission differs','classifier authored result differs','whole perception FP differs','whole perception result differs','completed perception distance FP differs')),(r.returncode,r.stdout,r.stderr)
print('Independent original rejects semantic mutation:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.24)
project(NativePerceptionStorage LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_systems STATIC src/campaign/ai/native_campaign_ai_clump_retirement_tu15.cpp src/campaign/ai/native_campaign_ai_clump_callbacks_tu15.cpp)
target_include_directories(halo2_systems PRIVATE include)
target_compile_features(halo2_systems PRIVATE cxx_std_20)
add_library(halo2_physics_world_runtime STATIC)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeAiPerceptionGroupLink.cmake)
add_library(mutation_driver OBJECT tests/native_campaign_ai_perception_group_link_tu15_tests.cpp)
target_include_directories(mutation_driver PRIVATE include)
target_compile_features(mutation_driver PRIVATE cxx_std_20)
target_link_libraries(mutation_driver PRIVATE nlohmann_json::nlohmann_json)
'''
    common=['src/campaign/ai/native_campaign_ai_perception_classify_tu15.cpp','src/campaign/ai/native_campaign_ai_perception_group_actor_tu15.cpp','src/campaign/ai/native_campaign_ai_perception_group_initialize_tu15.cpp','src/campaign/ai/native_campaign_ai_perception_group_storage_tu15.cpp','src/campaign/ai/native_campaign_object_world_point_tu15.cpp','src/campaign/ai/native_campaign_ai_clump_retirement_tu15.cpp','src/campaign/ai/native_campaign_ai_clump_callbacks_tu15.cpp']
    for name,sources in [('mutation_common',common),('mutation_top',[top]),('mutation_pressure',[pressure])]:
        cmake+='add_library('+name+' OBJECT '+' '.join(sources)+')\n'+f'target_include_directories({name} PRIVATE include)\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
    for name,(owner,fixture) in variants.items():
        dependency='pressure'if owner=='top'else'top'
        cmake+=f'''add_executable(storage_{name} $<TARGET_OBJECTS:mutation_driver> $<TARGET_OBJECTS:mutation_common> $<TARGET_OBJECTS:mutation_{dependency}> src/campaign/ai/{name}.cpp)
target_include_directories(storage_{name} PRIVATE include)
target_compile_features(storage_{name} PRIVATE cxx_std_20)
target_link_libraries(storage_{name} PRIVATE halo2_native_tu15_data_array_delete)
add_test(NAME storage_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:storage_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/{fixture}")
'''
    (project/'CMakeLists.txt').write_text(cmake)
    pins={name:sha(ROOT/name)for name in names};pins.update({name:sha(ROOT/name)for name in ['include/halo2/native_campaign_ai_perception_group_link_tu15.hpp','include/halo2/native_campaign_ai_perception_classify_tu15.hpp','include/halo2/native_campaign_ai_perception_group_actor_tu15.hpp','include/halo2/native_campaign_ai_perception_group_pressure_tu15.hpp','include/halo2/native_campaign_ai_clump_retirement_tu15.hpp','include/halo2/native_campaign_ai_clump_callbacks_tu15.hpp','include/halo2/native_campaign_ai_perception_group_initialize_tu15.hpp','include/halo2/native_campaign_audio_stop_callback.hpp','include/halo2/native_campaign_object_world_point_tu15.hpp','include/halo2/native_tu15_physics_frame.hpp','include/halo2/native_campaign_ai_perception_group_storage_tu15.hpp','include/halo2/native_tu15_data_array_iteration.hpp','include/halo2/native_tu15_data_array_allocate.hpp','include/halo2/native_tu15_data_array_delete.hpp','include/halo2/physics_contact_record.hpp',str(Path(__file__).relative_to(ROOT))]});results=[];instrumented=[]
    for kind,compiler,sanitizers in [('gcc','g++',False),('clang-controls','clang++',False),('sanitizers','clang++',True)]:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_campaign_ai_perception_group_link_tu15_tests','--filter','^(native_campaign_ai_perception_group_link_tu15|native_campaign_ai_perception_group_pressure_focus_tu15|storage_)'if kind=='clang-controls'else'^(native_campaign_ai_perception_group_link_tu15|native_campaign_ai_perception_group_pressure_focus_tu15)$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if kind=='clang-controls':
            for name in variants:command+=['--target','storage_'+name]
        if sanitizers:command+=['--sanitizers']
        status=subprocess.run(command).returncode;results.append(dict(kind=kind,exit_code=status,command=command))
        if sanitizers:
            commands=json.loads((a.output/kind/'evidence/compile_commands.json').read_text())
            for path in ['src/campaign/ai/native_campaign_ai_perception_group_link_tu15.cpp','src/campaign/ai/native_campaign_ai_perception_group_storage_tu15.cpp','src/core/native_tu15_data_array_allocate.cpp','src/core/native_tu15_data_array_delete.cpp','src/core/native_tu15_data_array_iteration.cpp','src/campaign/ai/native_campaign_ai_perception_group_initialize_tu15.cpp','src/campaign/ai/native_campaign_object_world_point_tu15.cpp','src/campaign/ai/native_campaign_ai_perception_classify_tu15.cpp','src/campaign/ai/native_campaign_ai_perception_group_pressure_tu15.cpp','src/campaign/ai/native_campaign_ai_perception_group_actor_tu15.cpp','src/campaign/ai/native_campaign_ai_clump_retirement_tu15.cpp','src/campaign/ai/native_campaign_ai_clump_callbacks_tu15.cpp']:
                found=[c.get('command',' '.join(c.get('arguments',[])))for c in commands if c['file'].endswith('/'+path) and (path.startswith('src/core/') or 'native_campaign_ai_perception_group_link_tu15_tests.dir' in c.get('command',' '.join(c.get('arguments',[]))))];assert found;instrumented.append(dict(source=path,all_objects_instrumented=all('-fsanitize=address,undefined' in c for c in found),commands=found))
        if status:break
    report=dict(passed=len(results)==3 and all(r['exit_code']==0 for r in results)and all(r['all_objects_instrumented']for r in instrumented),source_sha256=pins,source_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),results=results,negative_controls=list(variants),actual_sanitizer_objects=instrumented,full2749F0_closed=False,lifted_fallback=False,retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
