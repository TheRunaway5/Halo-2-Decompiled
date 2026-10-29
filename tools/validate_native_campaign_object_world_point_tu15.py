#!/usr/bin/env python3
"""Owned object-world-point module/compiler/sanitizer/mutation qualification."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--fixture',type=Path,required=True);args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    names=['cmake/CampaignNativeObjectWorldPoint.cmake','src/campaign/ai/native_campaign_object_world_point_tu15.cpp','tests/native_campaign_object_world_point_tu15_tests.cpp','tools/test_native_campaign_screen_lifecycle_tu15.py']
    for name in names:
        destination=project/name;destination.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,destination)
    fixture=project/'tests/fixtures/native_campaign_object_world_point_tu15.json.gz';fixture.parent.mkdir(parents=True);shutil.copyfile(args.fixture,fixture)
    core=(project/names[1]).read_text();variants={}
    for name,needle,replacement in [('parent_index','parent&0xffffu','object&0xffffu'),('unsigned_node','std::bit_cast<std::int8_t>(std::uint8_t(read(memory,record+0x18u,1)))','std::uint8_t(read(memory,record+0x18u,1))'),('reverse_row_sum','auto sum=add(z_term,y_term);','auto sum=add(y_term,z_term);'),('scale_once','y=multiply(scalar(read(memory,matrix)),y);','y=multiply(scale,y);'),('wrong_translation','matrix+0x28u','matrix+0x2cu')]:
        assert core.count(needle)==1,(name,core.count(needle));variants[name]=core.replace(needle,replacement)
    for name,text in variants.items():(project/f'src/campaign/ai/{name}.cpp').write_text(text)
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('allocation datum differs','allocation read order differs','allocation store order differs','allocation retained bytes differ','Allocation bank extent differs','Missing point storage','Point storage extent differs','point FP differs','retained point bytes differ','point ordered reads differ','point ordered store differs','private point value differs','point store count differs')),(r.returncode,r.stdout,r.stderr)
print('Expected allocation mutation rejection:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeAllocation LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeObjectWorldPoint.cmake)
add_library(mutation_driver OBJECT tests/native_campaign_object_world_point_tu15_tests.cpp)
target_include_directories(mutation_driver PRIVATE include)
target_compile_features(mutation_driver PRIVATE cxx_std_20)
target_link_libraries(mutation_driver PRIVATE nlohmann_json::nlohmann_json)
'''
    for name in variants:
        cmake+=f'''add_executable(allocate_{name} $<TARGET_OBJECTS:mutation_driver> src/campaign/ai/{name}.cpp)
target_include_directories(allocate_{name} PRIVATE include)
target_compile_features(allocate_{name} PRIVATE cxx_std_20)
add_test(NAME allocate_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:allocate_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_object_world_point_tu15.json.gz")
'''
    (project/'CMakeLists.txt').write_text(cmake)
    pins={name:sha(ROOT/name)for name in names};pins.update({str(p.relative_to(ROOT)):sha(p)for p in (ROOT/'include/halo2/native_campaign_object_world_point_tu15.hpp',ROOT/'include/halo2/physics_contact_record.hpp',ROOT/'include/halo2/native_tu15_physics_frame.hpp',Path(__file__))});results=[]
    for kind,compiler,sanitizers in [('normal','g++',False),('clang-controls','clang++',False),('sanitizers','clang++',True)]:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/kind),'--target','native_campaign_object_world_point_tu15_tests','--filter','^(native_campaign_object_world_point_tu15|allocate_)'if kind=='clang-controls'else'^native_campaign_object_world_point_tu15$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','Testing/Temporary/LastTest.log']
        if kind=='clang-controls':
            for name in variants:command+=['--target','allocate_'+name]
        if sanitizers:command+=['--sanitizers','--retain-report','compile_commands.json']
        run=subprocess.run(command,check=False);results.append(dict(kind=kind,exit_code=run.returncode,command=command))
        if run.returncode:break
    report=dict(passed=len(results)==3 and all(x['exit_code']==0 for x in results),source_hashes=pins,source_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,results=results,negative_controls=list(variants),owned_scratch=str(scratch),retained_large_artifacts=[])
    (args.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
