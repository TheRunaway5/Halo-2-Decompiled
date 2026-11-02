#!/usr/bin/env python3
"""Owned native camera-BSP-selection original-reference/compiler/mutation qualification."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--transport-extension',action='store_true');a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
 names=['cmake/NativeTu15CameraBspSelection.cmake','src/rendering/world/native_tu15_camera_bsp_selection.cpp','tests/native_tu15_camera_bsp_selection_tests.cpp','tests/fixtures/native_tu15_camera_bsp_selection.json.gz','tools/test_native_campaign_screen_lifecycle_tu15.py','src/rendering/world/native_tu15_camera_bsp_selection_binding.cpp']
 for name in names:
  dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
 core=(project/names[1]).read_text();variants={}
 pairs=[
 ('unsigned_plane_cluster','return std::bit_cast<std::int16_t>(std::uint16_t(value));','return std::int32_t(std::uint16_t(value));'),
 ('unordered_quiet_compare','comiss %2,%1; setae %0','ucomiss %2,%1; setae %0'),
 ('wrong_child_offset','record+1u+side*3u','record+2u+side*3u'),
 ('unmasked_start','nodes+(node&0x7fffffu)*8u','nodes+node*8u'),
 ('inclusive_history','old_leaf<signed_value(read(memory,bsp+0x2cu))','old_leaf<=signed_value(read(memory,bsp+0x2cu))'),
 ('stale_leaf_bank','publish(memory,observe,0x2b71eu,leaf_output,std::uint32_t(selected));\n        const auto leaves=read(memory,bsp+0x30u);','const auto leaves=read(memory,bsp+0x30u);\n        publish(memory,observe,0x2b71eu,leaf_output,std::uint32_t(selected));'),
 ('reassociated_plane','distance=sum(distance,product(ny,y));','const auto yterm=product(ny,y);'),
 ('wrong_history_clear','publish(memory,observe,0x2b752u,cluster_output,0xffffffffu);','publish(memory,observe,0x2b752u,cluster_output,0u);')]

 for name,needle,replacement in pairs:
  assert core.count(needle)==1,(name,core.count(needle));variants[name]=core.replace(needle,replacement)
  if name=='reassociated_plane':variants[name]=variants[name].replace('distance=sum(distance,product(nx,x));','distance=sum(distance,sum(yterm,product(nx,x)));')
 for name,text in variants.items():(project/f'src/rendering/world/{name}.cpp').write_text(text)
 (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('BSP retained bytes differ','BSP ordered reads differ','BSP ordered store differs','BSP store count differs','BSP completion differs','BSP floating point status differs','BSP actual fault operand differs','BSP semantic outcome differs','Missing camera BSP selection storage','Camera BSP selection storage extent differs')),(r.returncode,r.stdout,r.stderr)
print('Expected physical-original render camera packet mutation rejection:',r.stderr.strip()[-4000:])
''')
 cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeTu15CameraBspSelection LANGUAGES CXX)
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
include(cmake/NativeTu15CameraBspSelection.cmake)
'''
 for name in variants:
  cmake+=f'''add_executable(camera_{name} tests/native_tu15_camera_bsp_selection_tests.cpp src/rendering/world/{name}.cpp src/rendering/world/native_tu15_camera_bsp_selection_binding.cpp)
target_include_directories(camera_{name} PRIVATE include)
target_compile_features(camera_{name} PRIVATE cxx_std_20)
target_compile_options(camera_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(camera_{name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME camera_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:camera_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_tu15_camera_bsp_selection.json.gz")
'''
 (project/'CMakeLists.txt').write_text(cmake);results=[];pins={name:sha(ROOT/name)for name in names};pins.update({str(path.relative_to(ROOT)):sha(path)for path in(ROOT/'include/halo2/native_tu15_camera_bsp_selection.hpp',ROOT/'include/halo2/native_tu15_camera_bsp_selection_binding.hpp',ROOT/'include/halo2/physics_contact_record.hpp',ROOT/'include/halo2/native_tu15_physics_frame.hpp',Path(__file__))})
 matrix=[('normal','g++',False),('sanitizers','clang++',True)]
 if not a.transport_extension:matrix.append(('clang-controls','clang++',False))
 for kind,compiler,sanitizers in matrix:
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_tu15_camera_bsp_selection_tests','--filter','^(native_tu15_camera_bsp_selection|camera_)'if kind=='clang-controls'else'^native_tu15_camera_bsp_selection$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
  if kind=='clang-controls':
   for name in variants:command+=['--target','camera_'+name]
  if sanitizers:command+=['--sanitizers']
  run=subprocess.run(command,check=False);results.append(dict(kind=kind,exit_code=run.returncode,command=command))
  if run.returncode:break
 report=dict(passed=len(results)==len(matrix) and all(x['exit_code']==0 for x in results),source_hashes=pins,source_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),results=results,negative_controls=[]if a.transport_extension else list(variants),transport_extension=a.transport_extension,owned_scratch=str(scratch),retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
