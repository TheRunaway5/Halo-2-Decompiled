#!/usr/bin/env python3
"""Qualify real native private marker values against independent BAD60 calls."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';shutil.copytree(ROOT/'include',project/'include')
    request='src/campaign/ai/native_campaign_object_marker_request_tu15.cpp';markers='src/campaign/ai/native_campaign_object_markers_tu15.cpp';motion='src/campaign/ai/native_campaign_object_motion_refs_tu15.cpp'
    driver='tests/native_campaign_object_marker_request_value_tu15_tests.cpp';fixture='tests/fixtures/native_campaign_object_marker_request_value_tu15.json.gz';wrapper='tools/test_native_campaign_screen_lifecycle_tu15.py'
    names=[driver,fixture,wrapper,request,markers,motion]+[f'cmake/CampaignNativeObject{part}.cmake'for part in ('MarkerRequest','Markers','MotionRefs')]+[f'tests/native_campaign_object_{part}_tu15_tests.cpp'for part in ('marker_request','markers','motion_refs')]+[f'tests/fixtures/native_campaign_object_{part}_tu15.json.gz'for part in ('marker_request','markers','motion_refs')]
    for name in names:
        dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
    variants={}
    for name,owner,needle,replacement in [
        ('capacity_two',request,'result.count=request(m,object,direct,marker,1,target,fp);','result.count=request(m,object,direct,marker,2,target,fp);'),
        ('forced_named_count',request,'result.count=request(m,object,direct,marker,1,target,fp);','result.count=request(m,object,direct,marker,1,target,fp);result.count=1;'),
        ('missing_private_node',markers,'void NativeCampaignObjectMarkerLocalTu15::node(std::uint16_t value){value_.node=value;}','void NativeCampaignObjectMarkerLocalTu15::node(std::uint16_t value){value_.node=0;}'),
        ('missing_private_radius',markers,'void NativeCampaignObjectMarkerLocalTu15::radius(std::uint32_t value){value_.radius=value;}','void NativeCampaignObjectMarkerLocalTu15::radius(std::uint32_t value){value_.radius=0;}'),
        ('private_vector_short',markers,'for(unsigned n=0;n<3;++n)scalar(pose,first+n,value[n]);','for(unsigned n=0;n<2;++n)scalar(pose,first+n,value[n]);'),
        ('private_pose_alias',markers,'(pose==NativeCampaignObjectMarkerPoseTu15::local?value_.local:value_.world).at(field)=value;','value_.local.at(field)=value;')]:
        text=(ROOT/owner).read_text();assert text.count(needle)==1,(name,text.count(needle));(project/f'src/campaign/ai/{name}.cpp').write_text(text.replace(needle,replacement));variants[name]=owner
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(s in r.stderr for s in ('private marker count differs','private marker node differs','private marker radius differs','private marker local field differs','private marker world field differs','private marker FP differs','private marker ordered engine reads differ','A private marker value requires the real one-record capacity')),(r.returncode,r.stdout,r.stderr)
print('Independent original rejects private marker mutation:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.24)
project(NativePrivateMarkerValues LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeObjectMarkerRequest.cmake)
'''
    cmake+=f'''add_executable(private_marker_value "{driver}" "{request}" "{markers}" "{motion}")
target_include_directories(private_marker_value PRIVATE include)
target_compile_features(private_marker_value PRIVATE cxx_std_20)
target_compile_options(private_marker_value PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(private_marker_value PRIVATE nlohmann_json::nlohmann_json)
if(HALO2_SANITIZERS)
target_compile_options(private_marker_value PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
target_link_options(private_marker_value PRIVATE -fsanitize=address,undefined)
endif()
add_test(NAME private_marker_value COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/{wrapper}" --probe "$<TARGET_FILE:private_marker_value>" --fixture "${{CMAKE_CURRENT_SOURCE_DIR}}/{fixture}")
add_library(value_mutation_driver OBJECT "{driver}")
target_include_directories(value_mutation_driver PRIVATE include)
target_compile_features(value_mutation_driver PRIVATE cxx_std_20)
target_link_libraries(value_mutation_driver PRIVATE nlohmann_json::nlohmann_json)
'''
    for target,source in [('value_motion',motion),('value_request',request),('value_markers',markers)]:
        cmake+=f'add_library({target} OBJECT "{source}")\ntarget_include_directories({target} PRIVATE include)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
    for name,owner in variants.items():
        common='value_markers'if owner==request else'value_request'
        cmake+=f'''add_executable(value_{name} $<TARGET_OBJECTS:value_mutation_driver> $<TARGET_OBJECTS:value_motion> $<TARGET_OBJECTS:{common}> src/campaign/ai/{name}.cpp)
target_include_directories(value_{name} PRIVATE include)
target_compile_features(value_{name} PRIVATE cxx_std_20)
target_compile_options(value_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME value_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/{wrapper}" "$<TARGET_FILE:value_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/{fixture}")
'''
    (project/'CMakeLists.txt').write_text(cmake)
    names+=['include/halo2/native_campaign_object_marker_request_tu15.hpp','include/halo2/native_campaign_object_markers_tu15.hpp','include/halo2/native_campaign_object_motion_refs_tu15.hpp',str(Path(__file__).relative_to(ROOT)),'tools/oracle_native_campaign_object_marker_request_value_tu15.py','tools/observe_native_campaign_object_markers_tu15.cpp','tools/oracle_native_campaign_object_markers_tu15.py','tools/oracle_native_campaign_object_marker_request_tu15.py']
    pins={name:sha(ROOT/name)for name in names};results=[];instrumented=[]
    for kind,compiler,sanitizers in [('gcc','g++',False),('clang-controls','clang++',False),('sanitizers','clang++',True)]:
        cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','private_marker_value','--target','halo2_physics_world_runtime','--filter','^(private_marker_value|value_)'if kind=='clang-controls'else'^private_marker_value$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if kind=='clang-controls':
            for name in variants:cmd+=['--target','value_'+name]
        if sanitizers:cmd+=['--sanitizers']
        status=subprocess.run(cmd).returncode;results.append(dict(kind=kind,exit_code=status,command=cmd))
        if sanitizers:
            commands=json.loads((a.output/kind/'evidence/compile_commands.json').read_text());instrumented=[c['command']for c in commands if any(c['file'].endswith('/'+p)for p in (request,markers,motion))and'private_marker_value.dir'in c['command']];assert len(instrumented)==3 and all('-fsanitize=address,undefined'in c for c in instrumented)
        if status:break
    unchanged=all(sha(ROOT/p)==h for p,h in pins.items())
    report=dict(passed=len(results)==3 and all(r['exit_code']==0 for r in results)and unchanged,source_sha256=pins,source_unchanged=unchanged,results=results,negative_controls=list(variants),actual_sanitizer_objects=instrumented,owned_scratch=str(scratch),retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
