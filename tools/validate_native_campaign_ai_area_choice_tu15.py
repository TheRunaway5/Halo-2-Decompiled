#!/usr/bin/env python3
"""Owned compiler, sanitizer and semantic mutation tests for1BBBD0/1BBEC0."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
CORE='src/campaign/ai/native_campaign_ai_area_choice_tu15.cpp'
FIXTURE='tests/fixtures/native_campaign_ai_area_choice_tu15_v2.json.gz'
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--baseline-only',action='store_true');args=parser.parse_args();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';shutil.copytree(ROOT/'include',project/'include')
    names=[CORE,'src/campaign/ai/native_campaign_authored_area_iterator_tu15.hpp','src/campaign/ai/native_campaign_ai_point_geometry_tu15.cpp',
           'include/halo2/native_campaign_ai_area_choice_tu15.hpp',
           'include/halo2/native_campaign_ai_point_geometry_tu15.hpp','include/halo2/native_tu15_physics_frame.hpp','include/halo2/physics_contact_record.hpp',
           'cmake/CampaignNativeAiAreaChoice.cmake','tests/native_campaign_ai_area_choice_tu15_tests.cpp',FIXTURE,'tools/test_native_campaign_screen_lifecycle_tu15.py']
    for name in names:
        dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
    pins={name:sha(ROOT/name)for name in names};core=(project/CORE).read_text();variants={}
    pairs=[
        ('reversed_penalty','fsubrp %%st,%%st(1)','fsubp %%st,%%st(1)'),
        ('inclusive_score','if(better)return score;','if(better || score==best)return score;'),
        ('empty_reset','const auto mask=1u<<(old_ordinal&31u),current_pool=read(memory,0x5020c8u);','const auto mask=0u,current_pool=read(memory,0x5020c8u);'),
        ('short_limit','if(count>=32u)break;','if(count>=31u)break;'),
        ('cached_actor_pool','current_pool=read(memory,0x5020c8u);','current_pool=pool;'),
        ('invented_sentinel','const auto absent=read(memory,0x47d6c0u);','const auto absent=0xffffffffu;'),
        ('pending_bits','return value;\n}\nstd::optional','value.quantities[0][0]^=std::byte{1};return value;\n}\nstd::optional'),
        ('wrong_request_offset','request+0xcu','request+8u'),
        ('ignore_mask','if(!(read(memory,selection)&mask))','if((read(memory,selection),true))')]
    for name,needle,replacement in pairs:
        assert core.count(needle)==1,(name,core.count(needle));variants[name]=core.replace(needle,replacement)
        (project/f'src/campaign/ai/{name}.cpp').write_text(variants[name])
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],capture_output=True,text=True)
assert r.returncode!=0 and any(text in r.stderr for text in ('AL result differs','FP differs','ordered reads differ','retained bytes differ','store count differs','ordered store differs','pending extended bits differ','input timeline differs','completion differs')),(r.returncode,r.stdout,r.stderr)
print('Expected original influence mutation rejection:',r.stderr[-2500:])
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeAreaChoice LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeAiAreaChoice.cmake)
'''
    for name in variants:
        cmake+=f'''add_executable(area_choice_{name} tests/native_campaign_ai_area_choice_tu15_tests.cpp src/campaign/ai/{name}.cpp src/campaign/ai/native_campaign_ai_point_geometry_tu15.cpp)
target_include_directories(area_choice_{name} PRIVATE include)
target_compile_features(area_choice_{name} PRIVATE cxx_std_20)
target_compile_options(area_choice_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(area_choice_{name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME area_choice_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:area_choice_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/{FIXTURE}")
set_tests_properties(area_choice_{name} PROPERTIES TIMEOUT 30)
'''
    (project/'CMakeLists.txt').write_text(cmake);results=[]
    modes=[('gcc','g++',False)]if args.baseline_only else [('gcc','g++',False),('sanitizers','clang++',True),('clang-controls','clang++',False)]
    for kind,compiler,sanitizers in modes:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/kind),
                 '--target','native_campaign_ai_area_choice_tu15_tests','--filter','^(native_campaign_ai_area_choice_tu15|area_choice_.*)$'if kind=='clang-controls'else'^native_campaign_ai_area_choice_tu15$',
                 '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
                 '--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if sanitizers:command+=['--sanitizers']
        if kind=='clang-controls':
            for name in variants:command+=['--target','area_choice_'+name]
        run=subprocess.run(command);results.append(dict(kind=kind,command=command,exit_code=run.returncode))
        if run.returncode:break
    unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items());passed=len(results)==len(modes)and all(row['exit_code']==0 for row in results)and unchanged
    report=dict(passed=passed,baseline_only=args.baseline_only,source_sha256=pins,source_unchanged=unchanged,results=results,negative_controls=list(variants),owned_scratch=str(scratch),retained_large_artifacts=[],full_navigation_search_qualified=False,actual_world_route_qualified=False)
    (args.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not passed)


if __name__=='__main__':raise SystemExit(main())
