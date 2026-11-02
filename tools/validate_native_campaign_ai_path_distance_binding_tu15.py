#!/usr/bin/env python3
"""Owned compiler/sanitizer/semantic controls for actual distance transport."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
CORE='src/campaign/ai/native_campaign_ai_path_distance_tu15_binding.cpp'
DEPENDENCIES=['src/campaign/ai/native_campaign_ai_path_distance_tu15.cpp','src/campaign/ai/native_campaign_ai_point_geometry_tu15.cpp']
FIXTURE='tests/fixtures/native_campaign_ai_path_distance_transport_tu15.json.gz'


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--baseline-only',action='store_true');args=parser.parse_args()
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';shutil.copytree(ROOT/'include',project/'include')
    names=[CORE,*DEPENDENCIES,'include/halo2/native_campaign_ai_path_distance_tu15.hpp',
        'include/halo2/native_campaign_ai_path_distance_tu15_binding.hpp','include/halo2/native_campaign_ai_point_geometry_tu15.hpp',
        'include/halo2/native_tu15_physics_frame.hpp','include/halo2/physics_contact_record.hpp',
        'include/halo2/physics_contact_runtime.hpp','include/halo2/physics_world_runtime.hpp',
        'cmake/CampaignNativeAiPathDistance.cmake','cmake/CampaignNativeAiPathDistanceBinding.cmake',
        'tests/native_campaign_ai_path_distance_tu15_tests.cpp','tests/fixtures/native_campaign_ai_path_distance_tu15.json.gz',
        'tests/native_campaign_ai_path_distance_tu15_binding_tests.cpp',FIXTURE,'tools/test_native_campaign_screen_lifecycle_tu15.py']
    for name in names:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    pins={name:sha(ROOT/name)for name in names};core=(project/CORE).read_text();variants={}
    pairs=[
        ('early_zero','const auto pool=read(memory,0x5020c8u)','abi.xmm[0].fill(0);const auto pool=read(memory,0x5020c8u)'),
        ('late_zero','abi.xmm[0].fill(0);','/* missing provisional zero */'),
        ('wrong_upper_lane','abi.xmm[0]={result,0,0,0};','abi.xmm[0]={result,1,0,0};'),
        ('early_return','const auto pool=read(memory,0x5020c8u)','(void)read(memory,sp);const auto pool=read(memory,0x5020c8u)'),
        ('lost_pending','retain_pending(abi,error.pending());','(void)error.pending();'),
        ('swapped_pending','pending.previous_z_dy_dx[n].data()','pending.previous_z_dy_dx[count-1u-n].data()'),
        ('lost_permuted_coordinate','abi.xmm[0]={coordinate.bits,0,0,0};','if(coordinate.stage!=NativeCampaignAiProjectionCoordinateTu15::Stage::PermutedZ)abi.xmm[0]={coordinate.bits,0,0,0};'),
        ('lost_local_coordinate','abi.xmm[0]={coordinate.bits,0,0,0};','if(coordinate.stage!=NativeCampaignAiProjectionCoordinateTu15::Stage::LocalX)abi.xmm[0]={coordinate.bits,0,0,0};'),
        ('lost_scaled_coordinate','abi.xmm[0]={coordinate.bits,0,0,0};','if(coordinate.stage!=NativeCampaignAiProjectionCoordinateTu15::Stage::ScaledLocalX)abi.xmm[0]={coordinate.bits,0,0,0};'),
        ('short_cleanup','abi.gp[4]=sp+4u;','abi.gp[4]=sp+8u;'),
        ('zero_result','abi.xmm[0]={result,0,0,0};','abi.xmm[0]={0,0,0,0};')]
    if not args.baseline_only:
        for name,needle,replacement in pairs:
            assert core.count(needle)==1,(name,core.count(needle));variants[name]=core.replace(needle,replacement)
            (project/f'src/campaign/ai/{name}.cpp').write_text(variants[name])
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],capture_output=True,text=True)
assert r.returncode!=0 and any(s in r.stderr for s in ('scalar publication differs','ordered reads differ','retained bytes differ','FP differs','pending depth differs','pending quantity differs','stack cleanup differs','input timeline differs','unexpected missing storage','completion differs')),(r.returncode,r.stdout,r.stderr)
print('Expected original distance transport mutation rejection:',r.stderr[-1500:])
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeDistanceBinding LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeAiPathDistanceBinding.cmake)
'''
    for name in variants:
        target='distance_binding_'+name
        cmake+=f'''add_executable({target} tests/native_campaign_ai_path_distance_tu15_binding_tests.cpp src/campaign/ai/{name}.cpp {' '.join(DEPENDENCIES)})
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
            '--output',str(args.output/kind),'--target','native_campaign_ai_path_distance_tu15_binding_tests','--filter',
            '^(native_campaign_ai_path_distance_tu15_binding|distance_binding_.*)$'if kind=='clang-controls'else'^native_campaign_ai_path_distance_tu15_binding$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
            '--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if sanitizers:command+=['--sanitizers']
        if kind=='clang-controls':
            for name in variants:command+=['--target','distance_binding_'+name]
        result=subprocess.run(command);results.append(dict(kind=kind,command=command,exit_code=result.returncode))
        if result.returncode:break
    unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items())
    passed=len(results)==len(modes)and all(r['exit_code']==0 for r in results)and unchanged
    (args.output/'validation.json').write_text(json.dumps(dict(passed=passed,baseline_only=args.baseline_only,
        source_sha256=pins,source_unchanged=unchanged,results=results,negative_controls=list(variants),
        owned_scratch=str(scratch),retained_large_artifacts=[],actual_world_route_qualified=False,
        complete_action_pipeline_qualified=False),indent=2)+'\n');return int(not passed)


if __name__=='__main__':raise SystemExit(main())
