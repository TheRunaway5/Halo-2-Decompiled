#!/usr/bin/env python3
"""Owned original fixture, real module/compiler/sanitizer/mutation checks."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--fixture',type=Path,required=True);args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    names=['cmake/NativeTu15DataArrayAllocate.cmake','cmake/NativeTu15DataArrayDelete.cmake','src/core/native_tu15_data_array_allocate.cpp','src/core/native_tu15_data_array_delete.cpp','tests/native_tu15_data_array_allocate_tests.cpp','tools/test_native_campaign_screen_lifecycle_tu15.py']
    for name in names:
        destination=project/name;destination.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,destination)
    fixture=project/'tests/fixtures/native_tu15_data_array_allocate.json.gz';fixture.parent.mkdir(parents=True);shutil.copyfile(args.fixture,fixture)
    core=(project/names[2]).read_text();variants={}
    for name,needle,replacement in [('wrong_bit','(1u<<(index&31u))','(1u<<((index+1u)&31u))'),('salt_sentinel','==0xffffu','==0xfffeu'),('cached_clear_length','const auto clear_bytes=read(memory,pool+0x24u);','const auto clear_bytes=stride;'),('omit_tail','n<(clear_bytes&3u)','n<0u')]:
        assert core.count(needle)==1,(name,core.count(needle));variants[name]=core.replace(needle,replacement)
    for name,text in variants.items():(project/f'src/core/{name}.cpp').write_text(text)
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('allocation datum differs','allocation read order differs','allocation store order differs','allocation retained bytes differ')),(r.returncode,r.stdout,r.stderr)
print('Expected allocation mutation rejection:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeAllocation LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(cmake/NativeTu15DataArrayAllocate.cmake)
add_library(mutation_driver OBJECT tests/native_tu15_data_array_allocate_tests.cpp)
target_include_directories(mutation_driver PRIVATE include)
target_compile_features(mutation_driver PRIVATE cxx_std_20)
target_link_libraries(mutation_driver PRIVATE nlohmann_json::nlohmann_json)
'''
    for name in variants:
        cmake+=f'''add_executable(allocate_{name} $<TARGET_OBJECTS:mutation_driver> src/core/{name}.cpp)
target_include_directories(allocate_{name} PRIVATE include)
target_compile_features(allocate_{name} PRIVATE cxx_std_20)
add_test(NAME allocate_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:allocate_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_tu15_data_array_allocate.json.gz")
'''
    (project/'CMakeLists.txt').write_text(cmake)
    pins={name:sha(ROOT/name)for name in names};pins.update({str(p.relative_to(ROOT)):sha(p)for p in (ROOT/'include/halo2/native_tu15_data_array_allocate.hpp',ROOT/'include/halo2/physics_contact_record.hpp',Path(__file__))});results=[]
    for kind,compiler,sanitizers in [('normal','g++',False),('clang-controls','clang++',False),('sanitizers','clang++',True)]:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/kind),'--target','native_tu15_data_array_allocate_tests','--filter','^(native_tu15_data_array_allocate|allocate_)'if kind=='clang-controls'else'^native_tu15_data_array_allocate$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','Testing/Temporary/LastTest.log']
        if kind=='clang-controls':
            for name in variants:command+=['--target','allocate_'+name]
        if sanitizers:command+=['--sanitizers','--retain-report','compile_commands.json']
        run=subprocess.run(command,check=False);results.append(dict(kind=kind,exit_code=run.returncode,command=command))
        if run.returncode:break
    report=dict(passed=len(results)==3 and all(x['exit_code']==0 for x in results),source_hashes=pins,source_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,results=results,negative_controls=list(variants),owned_scratch=str(scratch),retained_large_artifacts=[])
    (args.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
