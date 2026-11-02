#!/usr/bin/env python3
"""Disposable unchanged-original participant compiler, sanitizer and mutation proof."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--gcc-only',action='store_true');a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    core='src/campaign/native_campaign_transition_save_metadata_tu15.cpp'
    names=[core,'include/halo2/native_campaign_transition_save_metadata_tu15.hpp','include/halo2/native_campaign_transition_request_tu15.hpp','include/halo2/native_tu15_physics_frame.hpp','include/halo2/physics_contact_record.hpp','cmake/CampaignNativeTransitionSaveMetadata.cmake','tests/native_campaign_transition_save_metadata_tu15_tests.cpp','tools/test_native_campaign_screen_lifecycle_tu15.py','tests/fixtures/native_campaign_transition_save_metadata_tu15.json.gz']
    for name in names:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    original=(project/core).read_text();variants={}
    mutations=[
        ('wrong_format_version','output.set(8,0x2651u)','output.set(8,0x2650u)',1),
        ('truncated_poison_fill','n<0x338u','n<0x334u',1),
        ('wrong_profile_stride','n*0xc74u','n*0xc70u',1),
        ('missing_fourth_profile','n<4u;++n','n<3u;++n',1),
        ('wrong_source_scalar_width','input.get(source));','input.get(source,2));',2),
        ('wrong_profile_footer','0x65707072u','0x65707073u',1),
        ('missing_nested_default_zero','if(!input.present(source))zero15(output,at);','if(!input.present(source)){}',1),
        ('missing_root_footer','output.set(0x337u,0x65636c63u);','output.set(0x337u,0x65636c62u);',1),
        ('staged_forward_profile_copy','for(std::uint32_t n=0;n<0x40u;n+=4u)output.set(at+4u+n,input.get(8u+n));','std::array<std::uint32_t,16> words{};for(std::uint32_t n=0;n<0x40u;n+=4u)words[n/4u]=input.get(8u+n);for(std::uint32_t n=0;n<0x40u;n+=4u)output.set(at+4u+n,words[n/4u]);',1),
    ]
    for name,before,after,count in mutations:
        assert original.count(before)==count,(name,original.count(before));path=f'src/campaign/{name}.cpp';(project/path).write_text(original.replace(before,after));variants[name]=path
    early=original.replace('captured.emplace();','const auto available=read(memory,row+0x1f8u);captured.emplace();').replace('if(read(memory,row+0x1f8u)==0xffffffffu)captured.reset();','if(available==0xffffffffu)captured.reset();')
    assert early!=original;path='src/campaign/early_profile_availability.cpp';(project/path).write_text(early);variants['early_profile_availability']=path
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('save-metadata ordered','retained save-metadata bytes differ','private metadata output differs','private source options changed','unexpected save-metadata source address','save-metadata completion differs')),(r.returncode,r.stdout,r.stderr)
print('Independent original rejects semantic mutation:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.24)
project(NativeTransitionSaveMetadata LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime STATIC)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeTransitionSaveMetadata.cmake)
add_library(mutation_driver OBJECT tests/native_campaign_transition_save_metadata_tu15_tests.cpp)
target_include_directories(mutation_driver PRIVATE include)
target_compile_features(mutation_driver PRIVATE cxx_std_20)
target_link_libraries(mutation_driver PRIVATE nlohmann_json::nlohmann_json)
'''
    for name,path in variants.items():cmake+=f'''add_executable(policy_{name} $<TARGET_OBJECTS:mutation_driver> {path})
target_include_directories(policy_{name} PRIVATE include)
target_compile_features(policy_{name} PRIVATE cxx_std_20)
add_test(NAME policy_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:policy_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_transition_save_metadata_tu15.json.gz")
'''
    (project/'CMakeLists.txt').write_text(cmake);pins={name:sha(ROOT/name)for name in names};pins[str(Path(__file__).relative_to(ROOT))]=sha(__file__)
    results=[];instrumented=[];configs=[('gcc','g++',False)]if a.gcc_only else [('gcc','g++',False),('clang-controls','clang++',False),('sanitizers','clang++',True)]
    for kind,compiler,sanitizers in configs:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_campaign_transition_save_metadata_tu15_tests','--filter','^(native_campaign_transition_save_metadata_tu15|policy_)'if kind=='clang-controls'else'^native_campaign_transition_save_metadata_tu15$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if kind=='clang-controls':
            for name in variants:command+=['--target','policy_'+name]
        if sanitizers:command+=['--sanitizers']
        status=subprocess.run(command).returncode;results.append(dict(kind=kind,exit_code=status,command=command))
        if sanitizers and status==0:
            commands=json.loads((a.output/kind/'evidence/compile_commands.json').read_text());found=[row.get('command',' '.join(row.get('arguments',[])))for row in commands if row['file'].endswith('/'+core)and'native_campaign_transition_save_metadata_tu15_tests.dir'in row.get('command',' '.join(row.get('arguments',[])))];assert found
            instrumented.append(dict(source=core,all_objects_instrumented=all('-fsanitize=address,undefined'in row for row in found),commands=found))
        if status:break
    staged_match=all(sha(project/name)==pins[name]for name in names)
    report=dict(passed=len(results)==len(configs)and all(r['exit_code']==0 for r in results)and all(r['all_objects_instrumented']for r in instrumented)and staged_match,full_matrix=not a.gcc_only,source_sha256=pins,source_unchanged=all(sha(ROOT/name)==digest for name,digest in pins.items()),staged_source_matches_pins=staged_match,results=results,negative_controls=list(variants)if not a.gcc_only else[],actual_sanitizer_objects=instrumented,lifted_fallback=False,retained_large_artifacts=[])
    (a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
