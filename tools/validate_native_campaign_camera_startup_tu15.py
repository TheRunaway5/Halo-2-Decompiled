#!/usr/bin/env python3
"""Owned current CMake/compiler/sanitizer/mutation proof for camera startup and CRC."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    names=['cmake/CampaignNativeCameraStartup.cmake','cmake/NativeTu15Crc32.cmake','src/core/native_tu15_crc32.cpp','src/campaign/native_campaign_camera_startup_tu15.cpp','src/campaign/native_campaign_camera_startup_tu15_binding.cpp','tests/native_campaign_camera_startup_tu15_tests.cpp','tests/fixtures/native_campaign_camera_startup_tu15.json.gz','tools/test_native_campaign_screen_lifecycle_tu15.py']
    for name in names:
        dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
    crc=(project/names[2]).read_text();startup=(project/names[3]).read_text();variants={}
    for name,which,needle,replacement in[
        ('wrong_polynomial','crc','0xedb88320u','0xedb88321u'),
        ('unsigned_count','crc','if(count>0)','if(count!=0)'),
        ('reversed_input','crc','input.byte(n)','input.byte(n^3u)'),
        ('missing_crc_table_tail','crc','byte<256u','byte<255u'),
        ('missing_director_tail','startup','n<0x143u','n<0x142u'),
        ('wrong_cinematic_size','startup','account(memory,0x24u,observe)','account(memory,0x20u,observe)'),
        ('cached_original_base','startup','fresh_base=data.word(0x4f2950u)','fresh_base=base'),
        ('wrong_default_operand','startup','data.word(0x46ab88u)','data.word(0x46a5f8u)')]:
        original=crc if which=='crc'else startup;assert original.count(needle)==1,(name,original.count(needle));variants[name]=dict(crc=crc,startup=startup);variants[name][which]=original.replace(needle,replacement)
    for name,texts in variants.items():
        for which,text in texts.items():(project/f'{name}_{which}.cpp').write_text(text)
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('retained camera startup bytes differ','camera startup ordered reads differ','camera startup ordered store differs','camera startup store count differs','camera startup completion differs','actual camera startup fault operand differs','Missing camera startup storage','Camera startup storage extent differs')),(r.returncode,r.stdout,r.stderr)
print('Unchanged physical original rejects source mutation:',r.stderr.strip()[-4000:])
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeCampaignCameraStartup LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
if(HALO2_SANITIZERS)
add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
add_link_options(-fsanitize=address,undefined)
endif()
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeCameraStartup.cmake)
'''
    for name in variants:
        cmake+=f'''add_executable(startup_{name} tests/native_campaign_camera_startup_tu15_tests.cpp {name}_crc.cpp {name}_startup.cpp src/campaign/native_campaign_camera_startup_tu15_binding.cpp)
target_include_directories(startup_{name} PRIVATE include)
target_compile_features(startup_{name} PRIVATE cxx_std_20)
target_link_libraries(startup_{name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME startup_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:startup_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_camera_startup_tu15.json.gz")
'''
    (project/'CMakeLists.txt').write_text(cmake);results=[];pins={name:sha(ROOT/name)for name in names};pins.update({str(path.relative_to(ROOT)):sha(path)for path in(ROOT/'include/halo2/native_campaign_camera_startup_tu15.hpp',ROOT/'include/halo2/native_campaign_camera_startup_tu15_binding.hpp',ROOT/'include/halo2/native_tu15_crc32.hpp',ROOT/'include/halo2/physics_contact_record.hpp',Path(__file__))})
    for kind,compiler,sanitizers in[('normal','g++',False),('sanitizers','clang++',True),('clang-controls','clang++',False)]:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_campaign_camera_startup_tu15_tests','--filter','^(native_campaign_camera_startup_tu15|startup_)'if kind=='clang-controls'else'^native_campaign_camera_startup_tu15$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
        if kind=='clang-controls':
            for name in variants:command+=['--target','startup_'+name]
        if sanitizers:command+=['--sanitizers']
        run=subprocess.run(command,check=False);results.append(dict(kind=kind,exit_code=run.returncode,command=command))
        if run.returncode:break
    report=dict(passed=len(results)==3 and all(x['exit_code']==0 for x in results),source_hashes=pins,source_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),results=results,negative_controls=list(variants),owned_scratch=str(scratch),retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
