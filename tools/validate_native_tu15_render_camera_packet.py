#!/usr/bin/env python3
"""Owned native render-camera-packet original-reference/compiler/mutation qualification."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
 names=['cmake/NativeTu15RenderCameraPacket.cmake','src/rendering/world/native_tu15_render_camera_packet.cpp','tests/native_tu15_render_camera_packet_tests.cpp','tests/fixtures/native_tu15_render_camera_packet.json.gz','tools/test_native_campaign_screen_lifecycle_tu15.py','src/rendering/world/native_tu15_render_camera_packet_binding.cpp']
 for name in names:
  dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
 core=(project/names[1]).read_text();variants={}
 pairs=[
 ('baked_factor','auto factor=read(memory,0x45c55cu);','auto factor=0x3f48f5c3u;'),
 ('wrong_ratio200','if(ratio==200)','if(ratio==100)'),
 ('unsigned_dimensions','return std::bit_cast<std::int16_t>(std::uint16_t(value));','return std::int32_t(std::uint16_t(value));'),
 ('unsigned_divide','const auto ratio=dividend/denominator;','const auto ratio=std::uint32_t(dividend)/std::uint32_t(denominator);'),
 ('wrong_position_lane','read(memory,observer+n*4u)','read(memory,observer+(n==2?4u:n*4u))'),
 ('wrong_final_flag','destination+0x58u','destination+0x59u'),
 ('skip_grid_argument','const auto grid=argument.grid();','const auto grid=spans?argument.grid():0u;')]
 for name,needle,replacement in pairs:
  assert core.count(needle)==1,(name,core.count(needle));variants[name]=core.replace(needle,replacement)
 for name,text in variants.items():(project/f'src/rendering/world/{name}.cpp').write_text(text)
 (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('retained render camera packet bytes differ','render camera packet ordered reads differ','render camera packet ordered store differs','render camera packet store count differs','render camera packet completion differs','render camera packet FP differs','actual render camera packet fault operand differs','original signed divide trap differs','default projection capability boundary differs','Missing render camera packet storage','Render camera packet storage extent differs')),(r.returncode,r.stdout,r.stderr)
print('Expected physical-original render camera packet mutation rejection:',r.stderr.strip()[-4000:])
''')
 cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeTu15RenderCameraPacket LANGUAGES CXX)
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
include(cmake/NativeTu15RenderCameraPacket.cmake)
'''
 for name in variants:
  cmake+=f'''add_executable(camera_{name} tests/native_tu15_render_camera_packet_tests.cpp src/rendering/world/{name}.cpp src/rendering/world/native_tu15_render_camera_packet_binding.cpp)
target_include_directories(camera_{name} PRIVATE include)
target_compile_features(camera_{name} PRIVATE cxx_std_20)
target_compile_options(camera_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(camera_{name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME camera_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:camera_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_tu15_render_camera_packet.json.gz")
'''
 (project/'CMakeLists.txt').write_text(cmake);results=[];pins={name:sha(ROOT/name)for name in names};pins.update({str(path.relative_to(ROOT)):sha(path)for path in(ROOT/'include/halo2/native_tu15_render_camera_packet.hpp',ROOT/'include/halo2/native_tu15_render_camera_packet_binding.hpp',ROOT/'include/halo2/physics_contact_record.hpp',ROOT/'include/halo2/native_tu15_physics_frame.hpp',Path(__file__))})
 for kind,compiler,sanitizers in [('normal','g++',False),('sanitizers','clang++',True),('clang-controls','clang++',False)]:
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_tu15_render_camera_packet_tests','--filter','^(native_tu15_render_camera_packet|camera_)'if kind=='clang-controls'else'^native_tu15_render_camera_packet$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
  if kind=='clang-controls':
   for name in variants:command+=['--target','camera_'+name]
  if sanitizers:command+=['--sanitizers']
  run=subprocess.run(command,check=False);results.append(dict(kind=kind,exit_code=run.returncode,command=command))
  if run.returncode:break
 report=dict(passed=len(results)==3 and all(x['exit_code']==0 for x in results),source_hashes=pins,source_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),results=results,negative_controls=list(variants),owned_scratch=str(scratch),retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
