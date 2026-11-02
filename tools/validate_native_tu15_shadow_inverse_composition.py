#!/usr/bin/env python3
"""Build actual projection+shadow modules together with one inverse definition."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
 files=['cmake/NativeTu15CameraProjection.cmake','cmake/NativeTu15ShadowInverse.cmake','src/rendering/world/native_tu15_camera_projection.cpp','src/rendering/world/native_tu15_camera_projection_binding.cpp','src/rendering/shadows/native_tu15_shadow_inverse_binding.cpp','tests/native_tu15_camera_projection_tests.cpp','tests/native_tu15_shadow_inverse_tests.cpp']
 for name in files:dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
 (project/'ownership.cpp').write_text('''#include "halo2/native_tu15_shadow_inverse_binding.hpp"
#include "halo2/native_tu15_camera_projection_binding.hpp"
#include <stdexcept>
struct Memory:halo2::SourcePhysicsContactMemory {
 mutable unsigned reads{};unsigned writes{};
 void read(std::uint32_t,std::span<std::byte>)const override{++reads;throw std::runtime_error("Unexpected composition read");}
 void write(std::uint32_t,std::span<const std::byte>)override{++writes;throw std::runtime_error("Unexpected composition write");}
};
int main(){using namespace halo2;
 const SourceShadowProfileDirect actual{SourcePhysicsContactImage::TitleUpdate15,{0xc4879,0x144090,false}};
 if(!native_tu15_shadow_inverse_owns_call(actual)||native_tu15_camera_projection_owns_entry(0x144090)||native_tu15_camera_projection_owns_call(0x144090,0xc4879))return 1;
 auto foreign=actual;foreign.image=SourcePhysicsContactImage::MapPack11;Memory m;SourcePhysicsWorldRuntimeAbi abi{};bool rejected{};
 try{native_tu15_shadow_inverse_run(m,foreign,abi);}catch(const std::invalid_argument&){rejected=true;}
 return !rejected||m.reads||m.writes||native_tu15_shadow_inverse_calls();}
''')
 (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(NativeShadowInverseComposition LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/NativeTu15CameraProjection.cmake)
include(cmake/NativeTu15ShadowInverse.cmake)
get_target_property(native_sources halo2_physics_world_runtime SOURCES)
foreach(required IN ITEMS src/rendering/world/native_tu15_camera_projection.cpp src/rendering/world/native_tu15_camera_projection_binding.cpp src/rendering/shadows/native_tu15_shadow_inverse_binding.cpp)
 set(found 0)
 foreach(actual IN LISTS native_sources)
  if(actual STREQUAL required)
   math(EXPR found "${found}+1")
  endif()
 endforeach()
 if(NOT found EQUAL 1)
  message(FATAL_ERROR "Native definition ownership differs: ${required} count=${found}")
 endif()
endforeach()
add_executable(shadow_inverse_composition ownership.cpp $<TARGET_OBJECTS:halo2_physics_world_runtime>)
target_include_directories(shadow_inverse_composition PRIVATE include)
target_compile_features(shadow_inverse_composition PRIVATE cxx_std_20)
add_test(NAME shadow_inverse_composition COMMAND shadow_inverse_composition)
''')
 files+=['include/halo2/native_tu15_shadow_inverse_binding.hpp','include/halo2/native_tu15_camera_projection.hpp','include/halo2/native_tu15_camera_projection_binding.hpp',str(Path(__file__).relative_to(ROOT))];pins={name:sha(ROOT/name)for name in files}
 cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/'cmake'),'--target','shadow_inverse_composition','--filter','^shadow_inverse_composition$','--jobs','2','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log'];run=subprocess.run(cmd,check=False);unchanged=all(sha(ROOT/n)==v for n,v in pins.items());report=dict(passed=run.returncode==0 and unchanged,scope=__doc__,source_hashes=pins,source_unchanged=unchanged,command=cmd,exit_code=run.returncode,single_production_inverse_definition=True,production_core_binding_units=3,component_reference_tests_rerun=False,shared_World_CMake_or_shadow_retired=False,owned_scratch=str(scratch),retained_large_artifacts=[]);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
