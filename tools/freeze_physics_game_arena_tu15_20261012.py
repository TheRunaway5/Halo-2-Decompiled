#!/usr/bin/env python3
"""Freeze exact arena production modules and compiler-discovered source closure."""
import hashlib,json,re,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/physics-game-arena-tu15-20261012'
OWNED=('include/halo2/physics_game_arena_runtime.hpp','include/halo2/physics_game_arena_native.hpp',
 'src/physics_game_arena_runtime_generated.cpp','src/physics_game_arena_native.cpp','src/physics_game_arena_call_frames.hpp',
 'tests/physics_game_arena_tests.cpp','cmake/PhysicsGameArena.cmake','cmake/PhysicsPhysicalStorage.cmake',
 'tools/research_physics_game_arena_tu15_20261012.py','tools/inspect_physics_game_arena_tu15_20261012.py',
 'tools/generate_physics_game_arena_tu15_20261012.py','tools/export_physics_game_arena_frames_tu15_20261012.py',
 'tools/oracle_physics_game_arena_tu15_20261012.py','tools/freeze_physics_game_arena_tu15_20261012.py',
 'tools/validate_physics_game_arena_tu15_20261012.py')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 source=E/'source-v1';assert not source.exists()
 deps=sorted(set(re.findall(r'(?:include|src|tests)/[^\s\\]+',(E/'dependencies.d').read_text())))
 pins={p:sha(ROOT/p)for p in sorted(set(deps)|set(OWNED))};source.mkdir()
 for p,h in pins.items():
  dest=source/p;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/p,dest);assert sha(dest)==h
 (source/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.24)
project(halo2_fresh_game_arena_cohort LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)
find_package(Threads REQUIRED)
option(HALO2_SANITIZERS "Instrument exact cohort with ASan/UBSan" OFF)
option(HALO2_THREAD_SANITIZER "Instrument exact cohort with TSan" OFF)
if(HALO2_SANITIZERS AND HALO2_THREAD_SANITIZER)
 message(FATAL_ERROR "Select one sanitizer runtime")
endif()
add_library(halo2_physics_world_runtime STATIC)
add_library(halo2_systems STATIC src/source_runtime_memory.cpp src/source_runtime_image.cpp src/source_runtime_host_physical_memory.cpp src/source_runtime_host_storage.cpp src/native_threads.cpp src/native_synchronization.cpp)
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
''')
 (source/'analysis').symlink_to(ROOT/'analysis',target_is_directory=True)
 for p,h in pins.items():assert sha(ROOT/p)==h,p
 pins['CMakeLists.txt']=sha(source/'CMakeLists.txt')
 d=dict(schema=1,files=pins,owned=OWNED,compiler_dependencies=deps,dependencies_sha256=sha(E/'dependencies.d'),
  scope='Exact native TU1.5 physical/async/arena construction and clients, real native workers/files. No occupied physics world, game launcher or live multiplayer proof.')
 p=E/'source-manifest-v1.json';p.write_text(json.dumps(d,sort_keys=True,indent=2)+'\n');print(sha(p))
if __name__=='__main__':main()
