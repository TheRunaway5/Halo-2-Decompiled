#!/usr/bin/env python3
"""Freeze exact production physical-stage module and compiler dependencies."""
import hashlib,json,re,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/physics-physical-storage-tu15-20261012'
OWNED=('include/halo2/physics_physical_storage_runtime.hpp','src/physics/physics_physical_storage_runtime_generated.cpp',
 'src/physics/physics_physical_storage_host.cpp','tests/physics_physical_storage_tests.cpp','cmake/PhysicsPhysicalStorage.cmake',
 'tools/research_physics_physical_storage_tu15_20261012.py','tools/generate_physics_physical_storage_tu15_20261012.py',
 'tools/oracle_physics_physical_storage_tu15_20261012.py','tools/freeze_physics_physical_storage_tu15_20261012.py',
 'tools/validate_physics_physical_storage_tu15_20261012.py')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 source=E/'source-v1';assert not source.exists()
 deps=sorted(set(re.findall(r'(?:include|src|tests)/[^\s\\]+',(E/'dependencies.d').read_text())))
 assert 'src/runtime/source_runtime_host_physical_memory.cpp'in deps
 pins={p:sha(ROOT/p)for p in sorted(set(deps)|set(OWNED))};source.mkdir()
 for p,h in pins.items():
  dest=source/p;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/p,dest);assert sha(dest)==h
 (source/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.24)
project(halo2_fresh_physical_stage_cohort LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json REQUIRED)
option(HALO2_SANITIZERS "Instrument this exact native source cohort" OFF)
add_library(halo2_physics_world_runtime STATIC)
add_library(halo2_systems STATIC src/runtime/source_runtime_memory.cpp src/runtime/source_runtime_image.cpp src/runtime/source_runtime_host_physical_memory.cpp)
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
include(cmake/PhysicsPhysicalStorage.cmake)
''')
 (source/'analysis').symlink_to(ROOT/'analysis',target_is_directory=True)
 for p,h in pins.items():assert sha(ROOT/p)==h,p
 pins['CMakeLists.txt']=sha(source/'CMakeLists.txt')
 d=dict(schema=1,files=pins,owned=OWNED,compiler_dependencies=deps,dependencies_sha256=sha(E/'dependencies.d'),
  scope='Exact production module and source closure, fresh TU1.5 physical stage and memory partitions only. No game launcher, arena construction or occupied physics world.')
 p=E/'source-manifest-v1.json';p.write_text(json.dumps(d,sort_keys=True,indent=2)+'\n');print(sha(p))
if __name__=='__main__':main()
