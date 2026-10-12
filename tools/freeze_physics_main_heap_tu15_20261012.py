#!/usr/bin/env python3
"""Freeze fresh heap source closure and the verified existing world archives."""
import hashlib,json,re,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/physics-main-heap-tu15-20261012'
Q=ROOT/'analysis/multiplayer/sandbox-source/world-qword-tu15-20261012'
OWNED=('include/halo2/physics_main_heap_runtime.hpp','src/physics_main_heap_runtime_generated.cpp',
 'src/physics_main_heap_host.cpp','tests/physics_main_heap_tests.cpp','cmake/PhysicsMainHeap.cmake',
 'tools/research_physics_main_heap_tu15_20261012.py','tools/generate_physics_main_heap_tu15_20261012.py',
 'tools/oracle_physics_main_heap_tu15_20261012.py','tools/freeze_physics_main_heap_tu15_20261012.py',
 'tools/validate_physics_main_heap_tu15_20261012.py')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 source=E/'source-v2';assert not source.exists()
 deps=sorted(set(re.findall(r'(?:include|src|tests)/[^\s\\]+',(E/'dependencies.d').read_text())))
 files=sorted(set(deps)|set(OWNED)|{'cmake/PhysicsPoolStartup.cmake'})
 pins={p:sha(ROOT/p)for p in files};parents={};(E/'parent-archives').mkdir(exist_ok=True)
 for mode in ('release','sanitizer'):
  dep=Q/f'build-{mode}/dependency-pins.json';d=json.loads(dep.read_text())
  assert d['source_manifest_sha256']=='3511cf814da3ed1bf030d591397a3a5fcc2ae5f15e8f64c2cb83ec47400ae606'
  parent_inputs={}
  for p,h in d['files'].items():
   p=Path(p);assert sha(p)==h,p
   rel=str(p.relative_to(Q/'source-v1'))
   if rel=='CMakeLists.txt':continue # Selected-component driver, never a C++ input.
   assert sha(ROOT/rel)==h,rel;parent_inputs[rel]=h
  archive=Q/f'build-{mode}/libhalo2_physics_world_runtime.a'
  expected={'release':'acece3ba446f5d797c14f0b7ade272cb2716f7db3d8acbb4fc5c4d13b86252d6',
   'sanitizer':'a9f92f396c65ea98508b8c1cf2dec98899150567aee5c27fdd12702337863658'}[mode]
  assert sha(archive)==expected
  target=E/f'parent-archives/{mode}.a'
  if target.exists():assert sha(target)==expected
  else:shutil.copyfile(archive,target)
  assert sha(target)==expected
  parents[mode]=dict(archive=str(target.relative_to(E)),archive_sha256=expected,dependency_manifest=str(dep.relative_to(ROOT)),
   dependency_manifest_sha256=sha(dep),source_inputs=parent_inputs,compile_commands_sha256=sha(Q/f'build-{mode}/compile_commands.json'))
 source.mkdir()
 for p,h in pins.items():
  dest=source/p;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/p,dest);assert sha(dest)==h
 cmake='''cmake_minimum_required(VERSION 3.24)
project(halo2_fresh_main_heap_cohort LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json REQUIRED)
option(HALO2_SANITIZERS "Instrument this exact native source cohort" OFF)
add_library(verified_parent_world STATIC IMPORTED)
if(HALO2_SANITIZERS)
 set(parent_mode sanitizer)
else()
 set(parent_mode release)
endif()
set_target_properties(verified_parent_world PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../parent-archives/${parent_mode}.a")
add_library(halo2_physics_world_runtime STATIC)
target_link_libraries(halo2_physics_world_runtime PUBLIC verified_parent_world)
add_library(halo2_systems STATIC src/source_runtime_memory.cpp src/source_runtime_image.cpp src/source_runtime_host_storage.cpp)
target_link_libraries(halo2_systems PUBLIC halo2_physics_world_runtime)
foreach(target IN ITEMS halo2_systems halo2_physics_world_runtime)
 target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
 target_compile_features(${target} PUBLIC cxx_std_20)
 target_compile_options(${target} PUBLIC -fno-fast-math -ffp-contract=off -frounding-math)
 if(HALO2_SANITIZERS)
  target_compile_options(${target} PUBLIC -O0 -g1 -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(${target} PUBLIC -fsanitize=address,undefined)
 endif()
endforeach()
include(cmake/PhysicsPoolStartup.cmake)
include(cmake/PhysicsMainHeap.cmake)
'''
 (source/'CMakeLists.txt').write_text(cmake);pins['CMakeLists.txt']=sha(source/'CMakeLists.txt')
 (source/'analysis').symlink_to(ROOT/'analysis',target_is_directory=True)
 for p,h in pins.items():
  if p!='CMakeLists.txt':assert sha(ROOT/p)==h,p
 report=dict(schema=1,files=pins,owned=OWNED,compiler_dependencies=deps,parents=parents,
  dependencies_sha256=sha(E/'dependencies.d'),original_fixture_sha256=sha(E/'original-v2/fixture.json'),
  archive_reuse=True,scope='Fresh heap composition only. Existing world bodies are linked from independently sealed matching Release/sanitizer archives. No game launcher or live world proof.')
 (E/'source-manifest-v2.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(sha(E/'source-manifest-v2.json'))
if __name__=='__main__':main()
