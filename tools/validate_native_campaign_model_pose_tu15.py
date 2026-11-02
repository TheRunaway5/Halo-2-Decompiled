#!/usr/bin/env python3
"""Owned native model-pose original-reference/compiler/mutation qualification."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--normal-only',action='store_true');a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
 names=['cmake/CampaignNativeModelPose.cmake','src/campaign/native_campaign_model_pose_tu15.cpp','tests/native_campaign_model_pose_tu15_tests.cpp','tests/fixtures/native_campaign_model_pose_tu15_v2.json.gz','tools/test_native_campaign_screen_lifecycle_tu15.py','src/campaign/native_campaign_model_pose_tu15_binding.cpp','src/campaign/ai/native_campaign_object_markers_tu15.cpp','cmake/CampaignNativeObjectMarkers.cmake','tests/native_campaign_object_markers_tu15_tests.cpp','tests/fixtures/native_campaign_object_markers_tu15.json.gz']
 for name in names:
  dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
 core=(project/names[1]).read_text();variants={}
 pairs=[
 ('wrong_pose_stride','poses+std::uint32_t(node)*0x20u','poses+std::uint32_t(node)*0x24u'),
 ('wrong_link','read(memory,record+8u,2)','read(memory,record+6u,2)'),
 ('wrong_lazy_flag','flags|1u','flags|2u'),
 ('wrong_root_side','out[6]=bits(side_z);','out[6]=0;'),
 ('short_quaternion_gain','gain=add(gain,gain);','gain=add(gain,scalar(0));'),
 ('wrong_packed_cross','_mm_shuffle_ps(w,w,0x66)','_mm_shuffle_ps(w,w,0x99)'),
 ('wrong_output_stride','destination+std::uint32_t(node)*0x34u','destination+std::uint32_t(node)*0x30u')]

 for name,needle,replacement in pairs:
  assert core.count(needle)==1,(name,core.count(needle));variants[name]=core.replace(needle,replacement)
 for name,text in variants.items():(project/f'src/campaign/{name}.cpp').write_text(text)
 (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('retained model pose bytes differ','model pose ordered reads differ','model pose ordered store differs','model pose store count differs','model pose completion differs','model pose FP differs','actual model pose fault operand differs','real CRT owner boundary differs','Missing model pose storage','Model pose storage extent differs')),(r.returncode,r.stdout,r.stderr)
print('Expected physical-original model pose mutation rejection:',r.stderr.strip()[-4000:])
''')
 cmake='''cmake_minimum_required(VERSION 3.20)
project(CampaignNativeModelPose LANGUAGES CXX)
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
include(cmake/CampaignNativeModelPose.cmake)
'''
 for name in variants:
  cmake+=f'''add_executable(camera_{name} tests/native_campaign_model_pose_tu15_tests.cpp src/campaign/{name}.cpp src/campaign/native_campaign_model_pose_tu15_binding.cpp src/campaign/ai/native_campaign_object_markers_tu15.cpp)
target_include_directories(camera_{name} PRIVATE include)
target_compile_features(camera_{name} PRIVATE cxx_std_20)
target_compile_options(camera_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(camera_{name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME camera_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:camera_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_model_pose_tu15_v2.json.gz")
'''
 (project/'CMakeLists.txt').write_text(cmake);results=[];pins={name:sha(ROOT/name)for name in names};pins.update({str(path.relative_to(ROOT)):sha(path)for path in(ROOT/'include/halo2/native_campaign_model_pose_tu15.hpp',ROOT/'include/halo2/native_campaign_model_pose_tu15_binding.hpp',ROOT/'include/halo2/physics_contact_record.hpp',ROOT/'include/halo2/native_tu15_physics_frame.hpp',Path(__file__))})
 for kind,compiler,sanitizers in ([('normal','g++',False)]if a.normal_only else [('normal','g++',False),('sanitizers','clang++',True),('clang-controls','clang++',False)]):
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_campaign_model_pose_tu15_tests','--filter','^(native_campaign_model_pose_tu15|camera_)'if kind=='clang-controls'else'^native_campaign_model_pose_tu15$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
  if kind=='clang-controls':
   for name in variants:command+=['--target','camera_'+name]
  if sanitizers:command+=['--sanitizers']
  run=subprocess.run(command,check=False);results.append(dict(kind=kind,exit_code=run.returncode,command=command))
  if run.returncode:break
 report=dict(passed=len(results)==(1 if a.normal_only else 3) and all(x['exit_code']==0 for x in results),source_hashes=pins,source_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),results=results,negative_controls=list(variants),owned_scratch=str(scratch),retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
