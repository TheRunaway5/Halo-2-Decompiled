#!/usr/bin/env python3
"""Owned native camera-frame-policy original-reference/compiler/mutation qualification."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--only',choices=('normal','sanitizers','clang-controls'));a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
 names=['cmake/NativeTu15CameraFramePolicy.cmake','src/rendering/world/native_tu15_camera_frame_policy.cpp','tests/native_tu15_camera_frame_policy_tests.cpp','tests/fixtures/native_tu15_camera_frame_policy.json.gz','tools/test_native_campaign_screen_lifecycle_tu15.py','src/rendering/world/native_tu15_camera_frame_policy_binding.cpp']
 for name in names:
  dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
 core=(project/names[1]).read_text();variants={}
 pairs=[
 ('wrong_three_view_branch','if(view_count==2u || view_count==3u) {','if(view_count==2u) {'),
 ('baked_scale','const auto scale=read(memory,0x46a140u);','const auto scale=0x3f59999au;'),
 ('reloaded_four_view_scale','publish(memory,observe,0x2c4c0u,0x4c684cu,scale);','publish(memory,observe,0x2c4c0u,0x4c684cu,read(memory,0x46a130u));'),
 ('early_coverage','publish(memory,observe,0x2c4e4u,0x473644u,scale);\n        const auto detail=read(memory,0x46a0ccu);','const auto detail=read(memory,0x46a0ccu);\n        publish(memory,observe,0x2c4e4u,0x473644u,scale);'),
 ('flag_order','publish(memory,observe,0x2c521u,0x4c6844u,ordinary,1);\n    publish(memory,observe,0x2c526u,0x4c6843u,ordinary,1);','publish(memory,observe,0x2c526u,0x4c6843u,ordinary,1);\n    publish(memory,observe,0x2c521u,0x4c6844u,ordinary,1);'),
 ('boolean_retirement','!read(memory,0x4c1fedu,1)','read(memory,0x4c1fedu,1)!=1u'),
 ('wrong_none','return 0xffffffffu;','return 0u;'),
 ('wrong_target_priority','if(read(memory,0x4c1fe0u) && !read(memory,0x4c1fedu,1))return 20u;\n    if(read(memory,0x4c1eb0u) && !read(memory,0x4c1ebdu,1))return 18u;','if(read(memory,0x4c1eb0u) && !read(memory,0x4c1ebdu,1))return 18u;\n    if(read(memory,0x4c1fe0u) && !read(memory,0x4c1fedu,1))return 20u;')]


 for name,needle,replacement in pairs:
  assert core.count(needle)==1,(name,core.count(needle));variants[name]=core.replace(needle,replacement)
 for name,text in variants.items():(project/f'src/rendering/world/{name}.cpp').write_text(text)
 (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('policy retained bytes differ','policy ordered reads differ','policy ordered store differs','policy store count differs','policy completion differs','policy actual fault differs','policy target result differs','policy interleaving differs')),(r.returncode,r.stdout,r.stderr)
print('Expected physical-original view rectangle mutation rejection:',r.stderr.strip()[-3000:])
''')
 cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeTu15CameraFramePolicy LANGUAGES CXX)
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
include(cmake/NativeTu15CameraFramePolicy.cmake)
'''
 for name in variants:
  cmake+=f'''add_executable(camera_{name} tests/native_tu15_camera_frame_policy_tests.cpp src/rendering/world/{name}.cpp src/rendering/world/native_tu15_camera_frame_policy_binding.cpp)
target_include_directories(camera_{name} PRIVATE include)
target_compile_features(camera_{name} PRIVATE cxx_std_20)
target_compile_options(camera_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(camera_{name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME camera_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:camera_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_tu15_camera_frame_policy.json.gz")
'''
 (project/'CMakeLists.txt').write_text(cmake);results=[];pins={name:sha(ROOT/name)for name in names};pins.update({str(path.relative_to(ROOT)):sha(path)for path in(ROOT/'include/halo2/native_tu15_camera_frame_policy.hpp',ROOT/'include/halo2/native_tu15_camera_frame_policy_binding.hpp',ROOT/'include/halo2/physics_contact_record.hpp',ROOT/'include/halo2/native_tu15_physics_frame.hpp',Path(__file__))})
 modes=[('normal','g++',False),('sanitizers','clang++',True),('clang-controls','clang++',False)]
 modes=[mode for mode in modes if not a.only or mode[0]==a.only]
 for kind,compiler,sanitizers in modes:
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_tu15_camera_frame_policy_tests','--filter','^(native_tu15_camera_frame_policy|camera_)'if kind=='clang-controls'else'^native_tu15_camera_frame_policy$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log','--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','compile_commands.json']
  if kind=='clang-controls':
   for name in variants:command+=['--target','camera_'+name]
  if sanitizers:command+=['--sanitizers']
  run=subprocess.run(command,check=False);results.append(dict(kind=kind,exit_code=run.returncode,command=command))
  if run.returncode:break
  if sanitizers:
   commands=json.loads((a.output/kind/'evidence/compile_commands.json').read_text())
   selected=[item for item in commands if 'native_tu15_camera_frame_policy_tests.dir' in item['command']]
   assert len(selected)==3 and all('-fsanitize=address,undefined' in item['command']for item in selected),selected
 report=dict(passed=len(results)==len(modes) and all(x['exit_code']==0 for x in results),source_hashes=pins,source_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),results=results,negative_controls=list(variants)if any(mode[0]=='clang-controls'for mode in modes)else[],sanitizer_compile_commands_verified=any(mode[0]=='sanitizers'for mode in modes),owned_scratch=str(scratch),retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
