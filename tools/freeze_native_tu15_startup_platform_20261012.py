#!/usr/bin/env python3
"""Freeze the native desktop boot adapter and matching original source graph."""
import ast,hashlib,json,re,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/native-tu15-startup-platform-20261012'
P=E.parent/'tu15-timer-startup-20261012'
OWN=('include/halo2/native_tu15_startup_platform.hpp','src/native_tu15_startup_platform.cpp',
 'tests/native_tu15_startup_platform_tests.cpp','cmake/NativeTu15StartupPlatform.cmake',
 'cmake/SourceTu15Settings.cmake','cmake/SourceTu15StartupRecords.cmake','cmake/SourceProcessHeapRuntime.cmake',
 'cmake/PhysicsPlatformLocks.cmake','cmake/PhysicsPlatformCall.cmake',
 'tools/oracle_native_tu15_startup_platform_20261012.py','tools/freeze_native_tu15_startup_platform_20261012.py',
 'tools/validate_native_tu15_startup_platform_20261012.py')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 source=E/'source-v1';assert not source.exists()
 parent=json.loads((P/'source-manifest-v1.json').read_text())
 deps=sorted(set(re.findall(r'(?:include|src|tests)/[^\s\\]+',(E/'dependencies.d').read_text())))
 # Reused world arithmetic must retain every compiler-discovered source/header
 # input from its already validated archive. Other adapter sources build fresh.
 required=set(re.findall(r'(?:include|src)/[^\s\\]+',(E/'dependencies.d').read_text().split('physics_world_runtime_generated.o:')[-1]))
 for name in required:assert sha(ROOT/name)==parent['files'][name],name
 names=set(deps)|set(OWN);pending=list(OWN)
 while pending:
  name=pending.pop()
  if not name.endswith('.py'):continue
  for node in ast.walk(ast.parse((ROOT/name).read_text())):
   modules=[node.module] if isinstance(node,ast.ImportFrom) and node.module else [v.name for v in node.names] if isinstance(node,ast.Import) else []
   for module in modules:
    found=next((q for q in (ROOT/'tools'/(module.replace('.','/')+'.py'),ROOT/'tools/reference/halo2-decompiled/tools'/(module.replace('.','/')+'.py')) if q.is_file()),None)
    if found:
     name=str(found.relative_to(ROOT))
     if name not in names:names.add(name);pending.append(name)
 pins={name:sha(ROOT/name) for name in sorted(names)};source.mkdir()
 for name,pin in pins.items():
  dest=source/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest);assert sha(dest)==pin==sha(ROOT/name)
 archives={}
 for mode,row in parent['archives'].items():
  origin=P/'source-v1'/row['file'];assert sha(origin)==row['sha256']
  dest=source/row['file'];dest.parent.mkdir(exist_ok=True);shutil.copyfile(origin,dest);assert sha(dest)==row['sha256']
  archives[mode]=dict(file=row['file'],sha256=row['sha256'])
 (source/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.24)
project(halo2_native_desktop_startup LANGUAGES CXX)
include(CTest)
option(HALO2_SANITIZERS "Instrument the fresh native startup graph" OFF)
find_package(Threads REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
add_compile_options(-fno-fast-math -ffp-contract=off -frounding-math)
if(HALO2_SANITIZERS)
 add_compile_options(-O0 -g1 -fsanitize=address,undefined -fno-omit-frame-pointer)
 add_link_options(-fsanitize=address,undefined)
 set(world_archive sanitizer-world.a)
else()
 set(world_archive release-world.a)
endif()
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/registration.cpp "// Actual CMake consumer\\n")
add_library(halo2_systems STATIC ${CMAKE_CURRENT_BINARY_DIR}/registration.cpp)
add_library(halo2_physics_world_runtime STATIC IMPORTED GLOBAL)
set_target_properties(halo2_physics_world_runtime PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/reference-libraries/${world_archive})
target_link_libraries(halo2_systems PUBLIC halo2_physics_world_runtime)
set(BUILD_TESTING OFF)
include(cmake/PhysicsPlatformLocks.cmake)
include(cmake/PhysicsPlatformCall.cmake)
include(cmake/SourceProcessHeapRuntime.cmake)
include(cmake/SourceTu15StartupRecords.cmake)
include(cmake/SourceTu15Settings.cmake)
set(BUILD_TESTING ON)
include(cmake/NativeTu15StartupPlatform.cmake)
add_executable(source_runtime_image_tests tests/source_runtime_image_tests.cpp src/source_runtime_memory.cpp src/source_runtime_image.cpp)
target_include_directories(source_runtime_image_tests PRIVATE include)
target_compile_features(source_runtime_image_tests PRIVATE cxx_std_20)
add_test(NAME original_runtime_image_tu15 COMMAND source_runtime_image_tests "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe" 1.5)
''')
 (source/'analysis').symlink_to(ROOT/'analysis',target_is_directory=True)
 m=dict(schema=1,files=pins,compiler_dependencies=deps,dependencies_sha256=sha(E/'dependencies.d'),top_sha256=sha(source/'CMakeLists.txt'),
  archives=archives,archive_compiler_inputs=sorted(required),parent_manifest_sha256=sha(P/'source-manifest-v1.json'),
  scope='Declared native desktop capability/configuration/query/status adapter with unchanged TU1.5 callers. No runtime BIOS/EEPROM/kernel table dependency; genuine timer and later heap/world remain required.')
 out=E/'source-manifest-v1.json';out.write_text(json.dumps(m,sort_keys=True,indent=2)+'\n');print(sha(out))
if __name__=='__main__':main()
