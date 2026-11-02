#!/usr/bin/env python3
"""Owned compiler, sanitizer and semantic mutation tests for27C520."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
CORE='src/campaign/ai/native_campaign_ai_path_influence_tu15.cpp'
FIXTURE='tests/fixtures/native_campaign_ai_path_influence_tu15_v2.json.gz'
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--baseline-only',action='store_true');args=parser.parse_args();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';shutil.copytree(ROOT/'include',project/'include')
    names=[CORE,'src/campaign/ai/native_campaign_ai_frame_geometry_tu15.cpp','src/campaign/ai/native_campaign_ai_point_geometry_tu15.cpp',
           'include/halo2/native_campaign_ai_path_influence_tu15.hpp','include/halo2/native_campaign_ai_frame_geometry_tu15.hpp',
           'include/halo2/native_campaign_ai_point_geometry_tu15.hpp','include/halo2/native_tu15_physics_frame.hpp','include/halo2/physics_contact_record.hpp',
           'cmake/CampaignNativeAiPathInfluence.cmake','tests/native_campaign_ai_path_influence_tu15_tests.cpp',FIXTURE,'tools/test_native_campaign_screen_lifecycle_tu15.py']
    for name in names:
        dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
    pins={name:sha(ROOT/name)for name in names};core=(project/CORE).read_text();variants={}
    pairs=[
        ('stale_radius','const auto current_radius=read(memory,influence+0x3cu);','const auto current_radius=bits(radius);'),
        ('inclusive_radius','if(above(mul(radius,radius),squared))','if(!above(squared,mul(radius,radius)))'),
        ('wrong_endpoint','chosen_x=scalar(end[0]);chosen_y=scalar(end[1]);chosen_z=scalar(end[2]);','chosen_x=scalar(read(memory,origin));chosen_y=scalar(read(memory,origin+4u));chosen_z=scalar(read(memory,origin+8u));'),
        ('conventional_direction','auto toward_x=sub(scalar(read(memory,origin)),scalar(center[0]));','auto toward_x=sub(scalar(center[0]),scalar(read(memory,origin)));'),
        ('rounded_root','asm volatile("flds %1\\n\\tfsqrt\\n\\tfsts %0":"=m"(distance):"m"(squared_bits):"memory","st");',
         'asm volatile("flds %1\\n\\tfsqrt\\n\\tfstps %0\\n\\tflds %0":"=m"(distance):"m"(squared_bits):"memory","st");'),
        ('private_distance','result.distance=value;','result.distance=0;'),
        ('pending_bits','return result;\n}\ntemplate<class Sink>','result.extended[0]^=std::byte{1};return result;\n}\ntemplate<class Sink>'),
        ('zero_default','auto distance=read(memory,0x45695cu);','auto distance=0u;'),
        ('cached_frame','const auto second_frame=std::uint16_t(read(memory,origin+0xcu,2));','const auto second_frame=first_frame;'),
        ('early_sum','const auto radius=scalar(read(memory,influence+0x3cu));\n        // The retained radius read precedes the original squared-distance\n        // additions, including their sticky FP effects on a rejected read.\n        auto squared=add(z_squared,y_squared);squared=add(squared,x_squared);',
         'auto squared=add(z_squared,y_squared);squared=add(squared,x_squared);\n        const auto radius=scalar(read(memory,influence+0x3cu));')]
    for name,needle,replacement in pairs:
        assert core.count(needle)==1,(name,core.count(needle));variants[name]=core.replace(needle,replacement)
        (project/f'src/campaign/ai/{name}.cpp').write_text(variants[name])
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],capture_output=True,text=True)
assert r.returncode!=0 and any(text in r.stderr for text in ('influence result bits differ','influence FP differs','influence ordered reads differ','influence retained bytes differ','influence store count differs','influence ordered store differs','influence pending extended bits differ','private influence result differs','influence input timeline differs','influence completion differs')),(r.returncode,r.stdout,r.stderr)
print('Expected original influence mutation rejection:',r.stderr[-2500:])
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativePathInfluence LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeAiPathInfluence.cmake)
'''
    for name in variants:
        cmake+=f'''add_executable(influence_{name} tests/native_campaign_ai_path_influence_tu15_tests.cpp src/campaign/ai/{name}.cpp src/campaign/ai/native_campaign_ai_frame_geometry_tu15.cpp src/campaign/ai/native_campaign_ai_point_geometry_tu15.cpp)
target_include_directories(influence_{name} PRIVATE include)
target_compile_features(influence_{name} PRIVATE cxx_std_20)
target_compile_options(influence_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(influence_{name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME influence_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:influence_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/{FIXTURE}")
set_tests_properties(influence_{name} PROPERTIES TIMEOUT 30)
'''
    (project/'CMakeLists.txt').write_text(cmake);results=[]
    modes=[('gcc','g++',False)]if args.baseline_only else [('gcc','g++',False),('sanitizers','clang++',True),('clang-controls','clang++',False)]
    for kind,compiler,sanitizers in modes:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/kind),
                 '--target','native_campaign_ai_path_influence_tu15_tests','--filter','^(native_campaign_ai_path_influence_tu15|influence_.*)$'if kind=='clang-controls'else'^native_campaign_ai_path_influence_tu15$',
                 '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
                 '--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if sanitizers:command+=['--sanitizers']
        if kind=='clang-controls':
            for name in variants:command+=['--target','influence_'+name]
        run=subprocess.run(command);results.append(dict(kind=kind,command=command,exit_code=run.returncode))
        if run.returncode:break
    unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items());passed=len(results)==len(modes)and all(row['exit_code']==0 for row in results)and unchanged
    report=dict(passed=passed,baseline_only=args.baseline_only,source_sha256=pins,source_unchanged=unchanged,results=results,negative_controls=list(variants),owned_scratch=str(scratch),retained_large_artifacts=[],full_navigation_search_qualified=False,actual_world_route_qualified=False)
    (args.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not passed)


if __name__=='__main__':raise SystemExit(main())
