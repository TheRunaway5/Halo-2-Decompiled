#!/usr/bin/env python3
"""Owned native view-rectangle original-reference/compiler/mutation qualification."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--only',choices=('normal','sanitizers','clang-controls'));a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
 names=['cmake/CampaignNativeViewRectangle.cmake','src/rendering/world/native_campaign_view_rectangle_tu15.cpp','tests/native_campaign_view_rectangle_tu15_tests.cpp','tests/fixtures/native_campaign_view_rectangle_tu15_v2.json.gz','tools/test_native_campaign_screen_lifecycle_tu15.py','src/rendering/world/native_campaign_view_rectangle_tu15_binding.cpp']
 for name in names:
  dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
 core=(project/names[1]).read_text();variants={}
 pairs=[
 ('unsigned_coordinates','return std::bit_cast<std::int16_t>(std::uint16_t(read(memory,at,2)));','return std::uint16_t(read(memory,at,2));'),
 ('baked_half','scalar(read(memory,0x46a0ccu))','scalar(0x3f000000u)'),
 ('cached_bottom','const auto fresh_bottom=coordinate(memory,camera+0x34u);','const auto fresh_bottom=outer_bottom;'),
 ('wrong_aspect','integer(outer_right-outer_left)','integer(outer_left-outer_right)'),
 ('boolean_override','if(read(memory,camera+0x58u,1)) {','if(read(memory,camera+0x58u,1)==1) {'),
 ('commuted_nan','center_x=add(center_x,one);','center_x=add(one,center_x);'),
 ('wrong_offset_sign','minimum_y=sub(minimum_y,delta_y);','minimum_y=add(minimum_y,delta_y);'),
 ('moved_publication','rectangle.store(0x2f9bcu,0,bits(left));\n    auto right=sub(integer(coordinate(memory,camera+0x36u)),horizontal_center);','auto right=sub(integer(coordinate(memory,camera+0x36u)),horizontal_center);\n    rectangle.store(0x2f9bcu,0,bits(left));')]

 for name,needle,replacement in pairs:
  assert core.count(needle)==1,(name,core.count(needle));variants[name]=core.replace(needle,replacement)
 for name,text in variants.items():(project/f'src/rendering/world/{name}.cpp').write_text(text)
 (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('retained rectangle bytes differ','rectangle ordered reads differ','rectangle ordered store differs','rectangle store count differs','rectangle completion differs','rectangle FP differs','actual rectangle fault differs','native private rectangle differs','rectangle interleaving differs')),(r.returncode,r.stdout,r.stderr)
print('Expected physical-original view rectangle mutation rejection:',r.stderr.strip()[-3000:])
''')
 cmake='''cmake_minimum_required(VERSION 3.20)
project(CampaignNativeViewRectangle LANGUAGES CXX)
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
include(cmake/CampaignNativeViewRectangle.cmake)
'''
 for name in variants:
  cmake+=f'''add_executable(camera_{name} tests/native_campaign_view_rectangle_tu15_tests.cpp src/rendering/world/{name}.cpp src/rendering/world/native_campaign_view_rectangle_tu15_binding.cpp)
target_include_directories(camera_{name} PRIVATE include)
target_compile_features(camera_{name} PRIVATE cxx_std_20)
target_compile_options(camera_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(camera_{name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME camera_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:camera_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_view_rectangle_tu15_v2.json.gz")
'''
 (project/'CMakeLists.txt').write_text(cmake);results=[];pins={name:sha(ROOT/name)for name in names};pins.update({str(path.relative_to(ROOT)):sha(path)for path in(ROOT/'include/halo2/native_campaign_view_rectangle_tu15.hpp',ROOT/'include/halo2/native_campaign_view_rectangle_tu15_binding.hpp',ROOT/'include/halo2/physics_contact_record.hpp',ROOT/'include/halo2/native_tu15_physics_frame.hpp',Path(__file__))})
 modes=[('normal','g++',False),('sanitizers','clang++',True),('clang-controls','clang++',False)]
 modes=[mode for mode in modes if not a.only or mode[0]==a.only]
 for kind,compiler,sanitizers in modes:
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_campaign_view_rectangle_tu15_tests','--filter','^(native_campaign_view_rectangle_tu15|camera_)'if kind=='clang-controls'else'^native_campaign_view_rectangle_tu15$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log','--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','compile_commands.json']
  if kind=='clang-controls':
   for name in variants:command+=['--target','camera_'+name]
  if sanitizers:command+=['--sanitizers']
  run=subprocess.run(command,check=False);results.append(dict(kind=kind,exit_code=run.returncode,command=command))
  if run.returncode:break
  if sanitizers:
   commands=json.loads((a.output/kind/'evidence/compile_commands.json').read_text())
   selected=[item for item in commands if 'native_campaign_view_rectangle_tu15_tests.dir' in item['command']]
   assert len(selected)==3 and all('-fsanitize=address,undefined' in item['command']for item in selected),selected
 report=dict(passed=len(results)==len(modes) and all(x['exit_code']==0 for x in results),source_hashes=pins,source_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),results=results,negative_controls=list(variants)if any(mode[0]=='clang-controls'for mode in modes)else[],sanitizer_compile_commands_verified=any(mode[0]=='sanitizers'for mode in modes),owned_scratch=str(scratch),retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
