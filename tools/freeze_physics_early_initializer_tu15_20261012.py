#!/usr/bin/env python3
"""Freeze complete early initializer production closure and selected build."""
import hashlib,json,re,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/physics-early-initializer-tu15-20261012'
OWNED=('include/halo2/physics_early_initializer_runtime.hpp','src/physics/physics_early_initializer_runtime_generated.cpp',
 'src/physics/physics_early_initializer_host.cpp','src/physics/physics_early_initializer_call_frames.hpp',
 'tests/physics_early_initializer_tests.cpp','cmake/PhysicsEarlyInitializer.cmake',
 'cmake/PhysicsPhysicalStorage.cmake','cmake/PhysicsGameArena.cmake','cmake/PhysicsPoolStartup.cmake','cmake/PhysicsMainHeap.cmake',
 'tools/research_physics_early_initializer_tu15_20261012.py','tools/generate_physics_early_initializer_tu15_20261012.py',
 'tools/export_physics_early_initializer_frames_tu15_20261012.py','tools/oracle_physics_early_initializer_tu15_20261012.py',
 'tools/freeze_physics_early_initializer_tu15_20261012.py','tools/validate_physics_early_initializer_tu15_20261012.py')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 source=E/'source-v1';assert not source.exists()
 deps=sorted(set(re.findall(r'(?:include|src|tests)/[^\s\\]+',(E/'dependencies.d').read_text())))
 pins={p:sha(ROOT/p)for p in sorted(set(deps)|set(OWNED))};source.mkdir()
 for p,h in pins.items():
  dest=source/p;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/p,dest);assert sha(dest)==h
 # Only the raw main producer participates in this prefix. The existing full
 # world allocator graph and main-heap tests have their own sealed cohort.
 (source/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.24)
project(halo2_fresh_early_initializer_cohort LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)
find_package(Threads REQUIRED)
option(HALO2_SANITIZERS "Instrument exact cohort with ASan/UBSan" OFF)
option(HALO2_THREAD_SANITIZER "Instrument exact cohort with TSan" OFF)
if(HALO2_SANITIZERS AND HALO2_THREAD_SANITIZER)
 message(FATAL_ERROR "Select one sanitizer runtime")
endif()
add_library(halo2_physics_world_runtime STATIC src/physics/physics_main_heap_runtime_generated.cpp)
add_library(halo2_systems STATIC src/runtime/source_runtime_memory.cpp src/runtime/source_runtime_image.cpp src/runtime/source_runtime_host_physical_memory.cpp src/runtime/source_runtime_host_storage.cpp src/platform/native_threads.cpp src/platform/native_synchronization.cpp)
target_link_libraries(halo2_systems PUBLIC halo2_physics_world_runtime Threads::Threads)
foreach(target IN ITEMS halo2_systems halo2_physics_world_runtime)
 target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
 target_compile_features(${target} PUBLIC cxx_std_20)
 target_compile_options(${target} PUBLIC -fno-fast-math -ffp-contract=off -frounding-math)
 if(HALO2_SANITIZERS)
  target_compile_options(${target} PUBLIC -O0 -g1 -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(${target} PUBLIC -fsanitize=address,undefined)
 elseif(HALO2_THREAD_SANITIZER)
  target_compile_options(${target} PUBLIC -O1 -g1 -fsanitize=thread -fno-omit-frame-pointer)
  target_link_options(${target} PUBLIC -fsanitize=thread)
 endif()
endforeach()
include(cmake/PhysicsPhysicalStorage.cmake)
include(cmake/PhysicsGameArena.cmake)
include(cmake/PhysicsPoolStartup.cmake)
include(cmake/PhysicsEarlyInitializer.cmake)
''')
 (source/'analysis').symlink_to(ROOT/'analysis',target_is_directory=True)
 for p,h in pins.items():assert sha(ROOT/p)==h,p
 pins['CMakeLists.txt']=sha(source/'CMakeLists.txt')
 d=dict(schema=1,files=pins,owned=OWNED,compiler_dependencies=deps,dependencies_sha256=sha(E/'dependencies.d'),
  selected_main_source='src/physics/physics_main_heap_runtime_generated.cpp',main_production_module_sha256=pins['cmake/PhysicsMainHeap.cmake'],
  scope='Exact native fresh TU1.5 startup and early initializer prefix to required renderer owner. Idle async worker is retired before synchronous bank use. No renderer pointer, mode transition, occupied world, launcher or live multiplayer proof.')
 p=E/'source-manifest-v1.json';p.write_text(json.dumps(d,sort_keys=True,indent=2)+'\n');print(sha(p))
if __name__=='__main__':main()
