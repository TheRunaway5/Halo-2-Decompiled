#!/usr/bin/env python3
"""Owned real-module/compiler/sanitizer/mutation proof for native markers."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';shutil.copytree(ROOT/'include',project/'include')
    core='src/campaign/ai/native_campaign_object_marker_request_tu15.cpp';driver='tests/native_campaign_object_marker_request_tu15_tests.cpp';fixture='tests/fixtures/native_campaign_object_marker_request_tu15.json.gz'
    names=[core,driver,fixture,'cmake/CampaignNativeObjectMarkerRequest.cmake','tools/test_native_campaign_screen_lifecycle_tu15.py']+[prefix+middle+suffix for middle in ('object_markers','object_motion_refs') for prefix,suffix in [('src/campaign/ai/native_campaign_','_tu15.cpp'),('tests/native_campaign_','_tu15_tests.cpp'),('tests/fixtures/native_campaign_','_tu15.json.gz')]]+['cmake/CampaignNativeObjectMarkers.cmake','cmake/CampaignNativeObjectMotionRefs.cmake']
    for name in names:
        dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
    text=(ROOT/core).read_text();variants={}
    for name,needle,replacement in[
        ('ignored_owner','direct?object:native_campaign_object_marker_owner_tu15(m,object)','object'),
        ('unsigned_offset','return std::bit_cast<std::int16_t>(std::uint16_t(value));','return std::uint16_t(value);'),
        ('missing_fallback_scale','out.scalar(local,0,one);','out.scalar(local,0,0);'),
        ('short_fallback_copy','lane<13;++lane','lane<12;++lane'),
        ('missing_marker_fallback','if(std::uint16_t(count))return std::uint16_t(count);','if(group==none)return 0;if(std::uint16_t(count))return std::uint16_t(count);'),
        ('wrong_named_status','return marker==0u?1u:0u;','return 1u;')]:
        assert text.count(needle)==1,(name,text.count(needle));(project/f'src/campaign/ai/{name}.cpp').write_text(text.replace(needle,replacement));variants[name]=True
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(s in r.stderr for s in ('object request result differs','ordered object request reads differ','ordered object request stores differ','completed perception distance FP differs','retained object request bytes differ','Missing object request bank','Whole perception extent differs')),(r.returncode,r.stdout,r.stderr)
print('Independent original rejects marker mutation:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.24)
project(NativeMarkers LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeObjectMarkerRequest.cmake)
add_library(mutation_common OBJECT src/campaign/ai/native_campaign_object_markers_tu15.cpp src/campaign/ai/native_campaign_object_motion_refs_tu15.cpp)
target_include_directories(mutation_common PRIVATE include)
target_compile_features(mutation_common PRIVATE cxx_std_20)
target_compile_options(mutation_common PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_library(mutation_driver OBJECT tests/native_campaign_object_marker_request_tu15_tests.cpp)
target_include_directories(mutation_driver PRIVATE include)
target_compile_features(mutation_driver PRIVATE cxx_std_20)
target_link_libraries(mutation_driver PRIVATE nlohmann_json::nlohmann_json)
'''
    for name in variants:
        cmake+=f'''add_executable(marker_{name} $<TARGET_OBJECTS:mutation_driver> $<TARGET_OBJECTS:mutation_common> src/campaign/ai/{name}.cpp)
target_include_directories(marker_{name} PRIVATE include)
target_compile_features(marker_{name} PRIVATE cxx_std_20)
target_compile_options(marker_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME marker_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:marker_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/{fixture}")
'''
    (project/'CMakeLists.txt').write_text(cmake)
    pins={name:sha(ROOT/name)for name in names+['include/halo2/native_campaign_object_marker_request_tu15.hpp','include/halo2/native_campaign_object_markers_tu15.hpp','include/halo2/native_campaign_object_motion_refs_tu15.hpp','include/halo2/native_tu15_physics_frame.hpp','include/halo2/physics_contact_record.hpp',str(Path(__file__).relative_to(ROOT))]};results=[];instrumented=[]
    for kind,compiler,sanitizers in[('gcc','g++',False),('clang-controls','clang++',False),('sanitizers','clang++',True)]:
        cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_campaign_object_marker_request_tu15_tests','--filter','^(native_campaign_object_marker_request_tu15|marker_)'if kind=='clang-controls'else'^native_campaign_object_marker_request_tu15$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if kind=='clang-controls':
            for name in variants:cmd+=['--target','marker_'+name]
        if sanitizers:cmd+=['--sanitizers']
        status=subprocess.run(cmd).returncode;results.append(dict(kind=kind,exit_code=status,command=cmd))
        if sanitizers:
            commands=json.loads((a.output/kind/'evidence/compile_commands.json').read_text());instrumented=[c['command']for c in commands if any(c['file'].endswith('/'+p)for p in(core,'src/campaign/ai/native_campaign_object_markers_tu15.cpp','src/campaign/ai/native_campaign_object_motion_refs_tu15.cpp'))and'native_campaign_object_marker_request_tu15_tests.dir'in c['command']];assert instrumented and all('-fsanitize=address,undefined'in c for c in instrumented)
        if status:break
    report=dict(passed=len(results)==3 and all(r['exit_code']==0 for r in results),source_sha256=pins,source_unchanged=all(sha(ROOT/p)==h for p,h in pins.items()),results=results,negative_controls=list(variants),actual_sanitizer_objects=instrumented,owned_scratch=str(scratch),retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
