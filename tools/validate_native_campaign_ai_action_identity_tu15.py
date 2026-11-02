#!/usr/bin/env python3
"""Owned original/compiler/sanitizer controls for four integer AI owners."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
CORE='src/campaign/ai/native_campaign_ai_action_identity_tu15.cpp'
LOOKUP='src/core/native_tu15_data_array_lookup.cpp'
FIXTURE='tests/fixtures/native_campaign_ai_action_identity_tu15.json.gz'


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--baseline-only',action='store_true');args=parser.parse_args()
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';shutil.copytree(ROOT/'include',project/'include')
    names=[CORE,LOOKUP,'include/halo2/native_campaign_ai_action_identity_tu15.hpp',
        'include/halo2/native_tu15_data_array_lookup.hpp','include/halo2/physics_contact_record.hpp',
        'cmake/CampaignNativeAiActionIdentity.cmake','cmake/NativeTu15DataArrayLookup.cmake',
        'tests/native_campaign_ai_action_identity_tu15_tests.cpp',FIXTURE,
        'tools/test_native_campaign_screen_lifecycle_tu15.py']
    for name in names:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    pins={name:sha(ROOT/name)for name in names};sources={name:(project/name).read_text()for name in (CORE,LOOKUP)};variants={}
    pairs=[
        ('first_match_only',CORE,'found=true;','found=true;n=4;'),
        ('boolean_flag',CORE,'record+0x400u+std::uint32_t(index)*6u,flag,1);','record+0x400u+std::uint32_t(index)*6u,bool(flag),1);'),
        ('unsigned_ring',CORE,'(signed_word(read(memory,record+0x3feu,2))+1)%4','(read(memory,record+0x3feu,2)+1u)%4u'),
        ('cached_ring',CORE,'index=signed_word(read(memory,record+0x3feu,2));\n    publish(memory,observe,0x26c58du','/* cached index */\n    publish(memory,observe,0x26c58du'),
        ('partial_goal',CORE,'if(read(memory,entry+2u)==goal)','if((read(memory,entry+2u)&0xffffu)==(goal&0xffffu))'),
        ('constant_none',CORE,'if(goal==read(memory,0x47d6c0u))return;','if(goal==0xffffffffu)return;'),
        ('early_static_bound',CORE,'const auto index=signed_word(goal);const auto zone=goal>>16u;','const auto index=signed_word(goal);const auto zone=goal>>16u;if(index<0)return 0;'),
        ('fixed_goal_cap',CORE,'if(index>=count)return 0;','if(index>=count || index>=32)return 0;'),
        ('skip_goal_bit',CORE,'if(!(mask&(1u<<(std::uint32_t(index)&31u))))return 0;','(void)mask;'),
        ('wrong_owner_mask',CORE,'owner=(goal>>16u)&0x7fffu','owner=(goal>>16u)&0x7ffeu'),
        ('unsigned_highwater',LOOKUP,'if(std::int32_t(index)>=count)return 0;','if(index>=std::uint32_t(count))return 0;'),
        ('ignore_salt',LOOKUP,'if(salt==0 || salt!=std::uint16_t(datum>>16u))return 0;','(void)salt;')]
    if not args.baseline_only:
        for name,path,needle,replacement in pairs:
            original=sources[path];assert original.count(needle)==1,(name,original.count(needle))
            variant=Path(path).with_name(name+'.cpp');(project/variant).write_text(original.replace(needle,replacement));variants[name]=(path,str(variant))
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],capture_output=True,text=True)
assert r.returncode!=0 and any(s in r.stderr for s in ('retained bytes differ','ordered reads differ','store count differs','ordered store differs','returned identity differs','input timeline differs','unexpected missing storage','completion differs')),(r.returncode,r.stdout,r.stderr)
print('Expected original action-identity mutation rejection:',r.stderr[-1500:])
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeActionIdentity LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeAiActionIdentity.cmake)
'''
    for name,(path,variant)in variants.items():
        target='action_identity_'+name
        dependencies=[variant if source==path else source for source in (CORE,LOOKUP)]
        cmake+=f'''add_executable({target} tests/native_campaign_ai_action_identity_tu15_tests.cpp {' '.join(dependencies)})
target_include_directories({target} PRIVATE include)
target_compile_features({target} PRIVATE cxx_std_20)
target_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME {target} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:{target}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/{FIXTURE}")
set_tests_properties({target} PROPERTIES TIMEOUT 30)
'''
    (project/'CMakeLists.txt').write_text(cmake);results=[]
    modes=[('gcc','g++',False)]if args.baseline_only else[('gcc','g++',False),('sanitizers','clang++',True),('clang-controls','clang++',False)]
    for kind,compiler,sanitizers in modes:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
            '--output',str(args.output/kind),'--target','native_campaign_ai_action_identity_tu15_tests','--filter',
            '^(native_campaign_ai_action_identity_tu15|action_identity_.*)$'if kind=='clang-controls'else'^native_campaign_ai_action_identity_tu15$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
            '--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if sanitizers:command+=['--sanitizers']
        if kind=='clang-controls':
            for name in variants:command+=['--target','action_identity_'+name]
        result=subprocess.run(command);results.append(dict(kind=kind,command=command,exit_code=result.returncode))
        if result.returncode:break
    unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items())
    passed=len(results)==len(modes)and all(r['exit_code']==0 for r in results)and unchanged
    (args.output/'validation.json').write_text(json.dumps(dict(passed=passed,baseline_only=args.baseline_only,
        source_sha256=pins,source_unchanged=unchanged,results=results,negative_controls=list(variants),
        owned_scratch=str(scratch),retained_large_artifacts=[],actual_world_route_qualified=False,
        complete_action_pipeline_qualified=False),indent=2)+'\n');return int(not passed)


if __name__=='__main__':raise SystemExit(main())
