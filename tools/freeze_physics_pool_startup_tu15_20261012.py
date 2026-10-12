#!/usr/bin/env python3
"""Freeze exact new production sources and compiler-discovered dependencies."""
import hashlib
import json
import re
import shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/physics-pool-startup-tu15-20261012'
OWNED=(
 'include/halo2/physics_pool_startup_runtime.hpp','include/halo2/physics_pool_host_storage.hpp',
 'src/physics_pool_startup_runtime_generated.cpp','src/physics_pool_host_storage.cpp',
 'tests/physics_pool_startup_tests.cpp','cmake/PhysicsPoolStartup.cmake',
 'tools/research_physics_pool_startup_tu15_20261012.py',
 'tools/generate_physics_pool_startup_tu15_20261012.py',
 'tools/oracle_physics_pool_startup_tu15_20261012.py',
 'tools/freeze_physics_pool_startup_tu15_20261012.py',
 'tools/validate_physics_pool_startup_tu15_20261012.py',
 'docs/PHYSICS_POOL_STARTUP_TU15_20261012.md')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    source=E/'source-v1';manifest=E/'source-manifest-v1.json'
    assert not source.exists() and not manifest.exists(),'Preserve existing frozen cohort'
    dependencies=sorted(set(re.findall(r'(?:include|src|tests)/[^\s\\]+',(E/'dependencies.d').read_text())))
    assert 'src/physics_pool_startup_runtime_generated.cpp' in dependencies
    assert 'src/source_runtime_memory.cpp' in dependencies
    files=sorted(set(dependencies)|set(OWNED));pins={p:sha(ROOT/p)for p in files}
    source.mkdir()
    for p in files:
        destination=source/p;destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(ROOT/p,destination);assert sha(destination)==pins[p],p
    cmake='''cmake_minimum_required(VERSION 3.24)
project(halo2_fresh_physics_pool_cohort LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json REQUIRED)
option(HALO2_SANITIZERS "Instrument this exact native source cohort" OFF)
add_library(halo2_physics_world_runtime STATIC)
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
'''
    (source/'CMakeLists.txt').write_text(cmake)
    (source/'analysis').symlink_to(ROOT/'analysis',target_is_directory=True)
    for p,pin in pins.items():assert sha(ROOT/p)==pin,p
    pins['CMakeLists.txt']=sha(source/'CMakeLists.txt')
    report=dict(schema=1,files=pins,owned=list(OWNED),compiler_discovered_dependencies=dependencies,
        dependency_manifest_sha256=sha(E/'dependencies.d'),tool_sha256=sha(__file__),
        original_fixture_sha256=sha(E/'original-v4/fixture.json'),
        scope='Exact new production CMake module and native source dependencies. Minimal selected component cohort; no full launcher, occupied world, original assets or broad live parity claim.')
    manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(files=len(pins),sha256=sha(manifest))))
if __name__=='__main__':main()
