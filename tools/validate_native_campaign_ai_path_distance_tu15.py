#!/usr/bin/env python3
"""Owned physical-original/compiler/sanitizer controls for native200C00."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
CORE='src/campaign/ai/native_campaign_ai_path_distance_tu15.cpp'
DEPENDENCIES=['src/campaign/ai/native_campaign_ai_point_geometry_tu15.cpp']
FIXTURE='tests/fixtures/native_campaign_ai_path_distance_tu15.json.gz'


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--baseline-only',action='store_true');args=parser.parse_args()
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';shutil.copytree(ROOT/'include',project/'include')
    names=[CORE,*DEPENDENCIES,'include/halo2/native_campaign_ai_path_distance_tu15.hpp',
        'include/halo2/native_campaign_ai_point_geometry_tu15.hpp','include/halo2/native_tu15_physics_frame.hpp',
        'include/halo2/physics_contact_record.hpp','cmake/CampaignNativeAiPathDistance.cmake',
        'tests/native_campaign_ai_path_distance_tu15_tests.cpp',FIXTURE,'tools/test_native_campaign_screen_lifecycle_tu15.py']
    for name in names:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    pins={name:sha(ROOT/name)for name in names};core=(project/CORE).read_text();variants={}
    pairs=[
        ('ignore_active','!read(memory,record+0x50cu,1)','!(read(memory,record+0x50cu,1),true)'),
        ('wrong_state','read(memory,record+0x504u,2)!=1','read(memory,record+0x504u,2)==0'),
        ('unsigned_indices','return std::bit_cast<std::int8_t>(std::uint8_t(read(memory,address,1)));','return std::uint8_t(read(memory,address,1));'),
        ('cached_end','try {end=index_byte(memory,record+0x539u);}','try {end=initial_end;}'),
        ('ignore_frame','const auto current=native_campaign_ai_project_point_value_tu15(memory,point,fp,coordinate);','const std::array<std::uint32_t,3> current{read(memory,point),read(memory,point+4u),read(memory,point+8u)};'),
        ('early_end','asm volatile("flds %0\\n\\tfsubs %1\\n\\tflds %2\\n\\tfsubs %3\\n\\tflds %4"::','const auto early=index_byte(memory,record+0x539u);\n            asm volatile("flds %0\\n\\tfsubs %1\\n\\tflds %2\\n\\tfsubs %3\\n\\tflds %4"::'),
        ('wrong_y_square','\\n\\tfld %%st(2)\\n\\tfmul %%st(3),%%st','\\n\\tfld %%st(3)\\n\\tfmul %%st(4),%%st'),
        ('omit_accumulation','\\n\\tfadds %1\\n\\tfstps %0','\\n\\tfstps %0'),
        ('wrong_pending_bits','return pending;\n}','pending.previous_z_dy_dx[0][0]^=std::byte{1};return pending;\n}')]
    if not args.baseline_only:
        for name,needle,replacement in pairs:
            assert core.count(needle)==1,(name,core.count(needle));variant=core.replace(needle,replacement)
            if name=='early_end':variant=variant.replace('try {end=index_byte(memory,record+0x539u);}','try {end=early;}')
            variants[name]=variant;(project/f'src/campaign/ai/{name}.cpp').write_text(variant)
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],capture_output=True,text=True)
assert r.returncode!=0 and any(s in r.stderr for s in ('retained bytes differ','ordered reads differ','distance result differs','FP differs','pending extended bits differ','pending quantity presence differs','input timeline differs','unexpected missing storage','completion differs')),(r.returncode,r.stdout,r.stderr)
print('Expected original path-distance mutation rejection:',r.stderr[-1500:])
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativePathDistance LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeAiPathDistance.cmake)
'''
    for name in variants:
        target='path_distance_'+name
        cmake+=f'''add_executable({target} tests/native_campaign_ai_path_distance_tu15_tests.cpp src/campaign/ai/{name}.cpp {' '.join(DEPENDENCIES)})
target_include_directories({target} PRIVATE include)
target_compile_features({target} PRIVATE cxx_std_20)
target_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME {target} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:{target}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/{FIXTURE}")
set_tests_properties({target} PROPERTIES TIMEOUT 30)
'''
    (project/'CMakeLists.txt').write_text(cmake);results=[]
    modes=[('gcc','g++',False)]if args.baseline_only else[('gcc','g++',False),('sanitizers','clang++',True),('clang-controls','clang++',False)]
    for kind,compiler,sanitizers in modes:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
            '--output',str(args.output/kind),'--target','native_campaign_ai_path_distance_tu15_tests','--filter',
            '^(native_campaign_ai_path_distance_tu15|path_distance_.*)$'if kind=='clang-controls'else'^native_campaign_ai_path_distance_tu15$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
            '--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if sanitizers:command+=['--sanitizers']
        if kind=='clang-controls':
            for name in variants:command+=['--target','path_distance_'+name]
        result=subprocess.run(command);results.append(dict(kind=kind,command=command,exit_code=result.returncode))
        if result.returncode:break
    unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items())
    passed=len(results)==len(modes)and all(r['exit_code']==0 for r in results)and unchanged
    (args.output/'validation.json').write_text(json.dumps(dict(passed=passed,baseline_only=args.baseline_only,
        source_sha256=pins,source_unchanged=unchanged,results=results,negative_controls=list(variants),
        owned_scratch=str(scratch),retained_large_artifacts=[],actual_world_route_qualified=False,
        complete_action_pipeline_qualified=False),indent=2)+'\n');return int(not passed)


if __name__=='__main__':raise SystemExit(main())
