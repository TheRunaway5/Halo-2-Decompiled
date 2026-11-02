#!/usr/bin/env python3
"""Owned compiler/sanitizer/original controls for native27A460 path result."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
CORE='src/campaign/ai/native_campaign_ai_path_result_tu15.cpp'
DEPENDENCIES=[f'src/campaign/ai/native_campaign_ai_{name}_tu15.cpp' for name in
              ('path_candidates','point_geometry','frame_geometry','vector_math')]
FIXTURE='tests/fixtures/native_campaign_ai_path_result_tu15.json.gz'


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    modes_group=parser.add_mutually_exclusive_group()
    modes_group.add_argument('--baseline-only',action='store_true')
    modes_group.add_argument('--controls-only',action='store_true',
        help='Run Clang plus semantic controls after an unchanged GCC/sanitizer pass')
    args=parser.parse_args();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';shutil.copytree(ROOT/'include',project/'include')
    names=[CORE,*DEPENDENCIES,*[f'include/halo2/native_campaign_ai_{name}_tu15.hpp' for name in
        ('path_result','path_candidates','point_geometry','frame_geometry','vector_math')],
        'include/halo2/native_tu15_physics_frame.hpp','include/halo2/physics_contact_record.hpp',
        'cmake/CampaignNativeAiPathResult.cmake','tests/native_campaign_ai_path_result_tu15_tests.cpp',
        FIXTURE,'tools/test_native_campaign_screen_lifecycle_tu15.py']
    for name in names:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    pins={name:sha(ROOT/name)for name in names};core=(project/CORE).read_text();variants={}
    pairs=[
        ('missing_node_cost','asm volatile("fadds %1\\n\\tfstps %0"','asm volatile("fstps %0"'),
        ('ignore_clearance_flag','result.clearance=read(memory,workspace+0x28u,1)!=0;','result.clearance=(read(memory,workspace+0x28u,1),false);'),
        ('stale_radius','if(capped)return read(memory,record+0x2cu);','if(capped)return limit;'),
        ('no_link_reversal','publish(memory,observe,0x27a56du,record,previous,2);','(void)previous;'),
        ('always_goal','return current==0xffffu?goal:record+0x18u;','return goal;'),
        ('wrong_fallback','const auto distance=read(memory,0x45695cu);','const auto distance=0u;'),
        ('bad_pending_bits','return value;\n}\nstruct Cost','value.quantities[0][0]^=std::byte{1};return value;\n}\nstruct Cost')]
    if not args.baseline_only:
        for name,needle,replacement in pairs:
            assert core.count(needle)==1,(name,core.count(needle));variants[name]=core.replace(needle,replacement)
            (project/f'src/campaign/ai/{name}.cpp').write_text(variants[name])
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],capture_output=True,text=True)
assert r.returncode!=0 and any(s in r.stderr for s in ('retained bytes differ','ordered reads differ','FP differs','store count differs','ordered store differs','pending extended bits differ','AL result differs','input timeline differs','unexpected missing storage')),(r.returncode,r.stdout,r.stderr)
print('Expected original path-result mutation rejection:',r.stderr[-2000:])
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativePathResult LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeAiPathResult.cmake)
'''
    for name in variants:
        target='path_result_'+name
        cmake+=f'''add_executable({target} tests/native_campaign_ai_path_result_tu15_tests.cpp src/campaign/ai/{name}.cpp {' '.join(DEPENDENCIES)})
target_include_directories({target} PRIVATE include)
target_compile_features({target} PRIVATE cxx_std_20)
target_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME {target} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:{target}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/{FIXTURE}")
set_tests_properties({target} PROPERTIES TIMEOUT 30)
'''
    (project/'CMakeLists.txt').write_text(cmake);results=[]
    modes=([('gcc','g++',False)]if args.baseline_only else
           [('clang-controls','clang++',False)]if args.controls_only else
           [('gcc','g++',False),('sanitizers','clang++',True),('clang-controls','clang++',False)])
    for kind,compiler,sanitizers in modes:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
            '--output',str(args.output/kind),'--target','native_campaign_ai_path_result_tu15_tests','--filter',
            '^(native_campaign_ai_path_result_tu15|path_result_.*)$'if kind=='clang-controls'else'^native_campaign_ai_path_result_tu15$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
            '--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if sanitizers:command+=['--sanitizers']
        if kind=='clang-controls':
            for name in variants:command+=['--target','path_result_'+name]
        result=subprocess.run(command);results.append(dict(kind=kind,command=command,exit_code=result.returncode))
        if result.returncode:break
    unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items())
    passed=len(results)==len(modes)and all(r['exit_code']==0 for r in results)and unchanged
    (args.output/'validation.json').write_text(json.dumps(dict(passed=passed,baseline_only=args.baseline_only,controls_only=args.controls_only,
        source_sha256=pins,source_unchanged=unchanged,results=results,negative_controls=list(variants),
        owned_scratch=str(scratch),retained_large_artifacts=[],actual_world_route_qualified=False,full_navigation_search_qualified=False),indent=2)+'\n')
    return int(not passed)


if __name__=='__main__':raise SystemExit(main())
