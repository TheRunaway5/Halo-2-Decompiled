#!/usr/bin/env python3
"""Owned semantic resource retirement, native page permissions and mutation qualification."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
 names=['cmake/CampaignNativeAiResourceRetirement.cmake','cmake/NativeTu15DataArrayDelete.cmake','cmake/SnapshotStorage.cmake','src/campaign/ai/native_campaign_ai_resource_retirement_tu15.cpp','src/core/native_tu15_data_array_delete.cpp','src/runtime/source_runtime_memory.cpp','src/runtime/source_runtime_host_physical_memory.cpp','src/runtime/source_runtime_snapshot_bytes.cpp','src/runtime/source_runtime_write_protection.inc','src/runtime/source_runtime_page_copy.hpp','src/runtime/source_runtime_host_physical_pages.hpp','src/assets/cache_file_snapshot.cpp','tests/native_campaign_ai_resource_retirement_tu15_tests.cpp','tests/fixtures/native_campaign_ai_resource_retirement_tu15.json.gz','tools/test_native_campaign_screen_lifecycle_tu15.py']
 for name in names:
  dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
 core=(project/names[3]).read_text();variants={}
 pairs=[('skip_real_protection','protection.protect_pages(protected_address, protected_bytes, 0x404u);','(void)protection;'),('skip_delete_callback','native_campaign_ai_unassociated_resource_retire_tu15(memory, datum, observe);','(void)datum;'),('cache_deleted_container_pool','const auto pool = read(memory, container + 0x64u);\n    native_tu15_data_array_delete(memory, pool, datum);','native_tu15_data_array_delete(memory, initial_pool, datum);'),('late_container_read','container = read(memory, 0x4f2d34u);\n        publish(memory, observe, 0x13007du, next + 0x20u, previous);','publish(memory, observe, 0x13007du, next + 0x20u, previous);\n        container = read(memory, 0x4f2d34u);'),('invent_gpu_completion','if (read(memory, record + 0x10u))','if (read(memory, record + 0x10u) && false)'),('wrong_terminal_neighbor','0x130094u, 0x4f2d2cu, previous','0x130094u, 0x4f2d2cu, next')]
 for name,needle,replacement in pairs:
  assert core.count(needle)==1,(name,core.count(needle));variants[name]=core.replace(needle,replacement)
 for name,text in variants.items():(project/f'src/campaign/ai/{name}.cpp').write_text(text)
 (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('retained resource bytes differ','interleaved resource effects differ','resource completion differs','actual original/native page permissions differ','Original runtime write to read-only physical page','Original runtime read outside retained banks','Required original runtime memory','Unmapped source runtime address','Original runtime access outside retained bank','Source runtime','source memory','Original runtime access','Original runtime read from an unmapped')),(r.returncode,r.stdout,r.stderr)
print('Expected physical-original resource mutation rejection:',r.stderr.strip()[-4000:])
''')
 cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeCampaignResourceRetirement LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/SnapshotStorage.cmake)
include(cmake/CampaignNativeAiResourceRetirement.cmake)
'''
 for name in variants:
  cmake+=f'''add_executable(resource_{name} tests/native_campaign_ai_resource_retirement_tu15_tests.cpp src/campaign/ai/{name}.cpp src/runtime/source_runtime_memory.cpp src/runtime/source_runtime_host_physical_memory.cpp)
target_include_directories(resource_{name} PRIVATE include src)
target_compile_features(resource_{name} PRIVATE cxx_std_20)
target_link_libraries(resource_{name} PRIVATE halo2_native_tu15_data_array_delete halo2_snapshot_storage nlohmann_json::nlohmann_json)
add_test(NAME resource_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:resource_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_ai_resource_retirement_tu15.json.gz")
'''
 (project/'CMakeLists.txt').write_text(cmake);results=[];pins={name:sha(ROOT/name)for name in names};pins.update({str(path.relative_to(ROOT)):sha(path)for path in(ROOT/'include/halo2/native_campaign_ai_resource_retirement_tu15.hpp',ROOT/'include/halo2/physics_contact_record.hpp',ROOT/'include/halo2/source_runtime_memory.hpp',ROOT/'include/halo2/source_runtime_host_physical_memory.hpp',Path(__file__))})
 for kind,compiler,sanitizers in [('normal','g++',False),('sanitizers','g++',True),('clang-controls','clang++',False)]:
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_campaign_ai_resource_retirement_tu15_tests','--filter','^(native_campaign_ai_resource_retirement_tu15|resource_)'if kind=='clang-controls'else'^native_campaign_ai_resource_retirement_tu15$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
  if kind=='clang-controls':
   for name in variants:command+=['--target','resource_'+name]
  if sanitizers:command+=['--sanitizers']
  run=subprocess.run(command,check=False);results.append(dict(kind=kind,exit_code=run.returncode,command=command))
  if run.returncode:break
 report=dict(passed=len(results)==3 and all(x['exit_code']==0 for x in results),source_hashes=pins,source_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),results=results,negative_controls=list(variants),owned_scratch=str(scratch),retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
