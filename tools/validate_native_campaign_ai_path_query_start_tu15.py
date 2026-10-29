#!/usr/bin/env python3
"""Owned native path-query-start normal/sanitizer/mutation qualification."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
 names=['cmake/CampaignNativeAiPathQueryStart.cmake','src/campaign/ai/native_campaign_ai_path_query_start_tu15.cpp','tests/native_campaign_ai_path_query_start_tu15_tests.cpp','tests/fixtures/native_campaign_ai_path_query_start_tu15.json.gz','tools/test_native_campaign_screen_lifecycle_tu15.py','src/campaign/ai/native_campaign_ai_point_geometry_tu15.cpp','src/campaign/ai/native_campaign_ai_path_heap_tu15.cpp','cmake/CampaignNativeAiPointGeometry.cmake','tests/native_campaign_ai_point_geometry_tu15_tests.cpp','tests/fixtures/native_campaign_ai_point_geometry_tu15.json.gz']
 for name in names:
  dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
 core=(project/names[1]).read_text();variants={}
 pairs=[('round_distance_before_scale','const auto scale=read(memory,0x46a110u);','const auto scale=read(memory,0x46a110u);asm volatile("fstp %%st(0);flds %0"::"m"(result.distance):"memory","st");'),('check_full64_cost','std::bit_cast<std::int32_t>(std::uint32_t(cost.quantized))>=0x7fff','std::bit_cast<std::int64_t>(cost.quantized)>=0x7fff'),('sanitize_invalid_cost','if(std::bit_cast<std::int32_t>(std::uint32_t(cost.quantized))>=0x7fff)','if((cost.distance&0x7f800000u)==0x7f800000u)return false;\n        if(std::bit_cast<std::int32_t>(std::uint32_t(cost.quantized))>=0x7fff)'),('reorder_point_reads','auto x=scalar(read(memory,second));x=subtract(x,scalar(read(memory,first)));\n        auto z=scalar(read(memory,second+8));z=subtract(z,scalar(read(memory,first+8)));','auto z=scalar(read(memory,second+8));z=subtract(z,scalar(read(memory,first+8)));\n        auto x=scalar(read(memory,second));x=subtract(x,scalar(read(memory,first)));'),('wrong_sector_mask','read(memory,sector,2)&0x3c0u','read(memory,sector,2)&0x40u'),('skip_native_heap','native_campaign_ai_path_heap_insert_tu15(memory,workspace','if(false)native_campaign_ai_path_heap_insert_tu15(memory,workspace')]
 for name,needle,replacement in pairs:
  assert core.count(needle)==1,(name,core.count(needle));variants[name]=core.replace(needle,replacement)
 for name,text in variants.items():(project/f'src/campaign/ai/{name}.cpp').write_text(text)
 (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('retained query-start bytes differ','query-start ordered reads differ','query-start ordered store differs','query-start store count differs','query-start FP differs','query-start bool differs','query-start completion differs','Missing query-start storage','Query-start storage extent differs')),(r.returncode,r.stdout,r.stderr)
print('Expected physical-original query-start mutation rejection:',r.stderr.strip()[-4000:])
''')
 cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeCampaignPathQueryStart LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeAiPathQueryStart.cmake)
include(cmake/CampaignNativeAiPointGeometry.cmake)
'''
 for name in variants:
  cmake+=f'''add_executable(query_{name} tests/native_campaign_ai_path_query_start_tu15_tests.cpp src/campaign/ai/{name}.cpp src/campaign/ai/native_campaign_ai_point_geometry_tu15.cpp src/campaign/ai/native_campaign_ai_path_heap_tu15.cpp)
target_include_directories(query_{name} PRIVATE include)
target_compile_features(query_{name} PRIVATE cxx_std_20)
target_compile_options(query_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(query_{name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME query_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:query_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_ai_path_query_start_tu15.json.gz")
'''
 (project/'CMakeLists.txt').write_text(cmake);results=[];pins={name:sha(ROOT/name)for name in names};pins.update({str(path.relative_to(ROOT)):sha(path)for path in(ROOT/'include/halo2/native_campaign_ai_path_query_start_tu15.hpp',ROOT/'include/halo2/native_campaign_ai_point_geometry_tu15.hpp',ROOT/'include/halo2/native_campaign_ai_path_heap_tu15.hpp',ROOT/'include/halo2/native_tu15_physics_frame.hpp',ROOT/'include/halo2/physics_contact_record.hpp',Path(__file__))})
 for kind,compiler,sanitizers in [('normal','g++',False),('sanitizers','g++',True),('clang-controls','clang++',False)]:
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_campaign_ai_path_query_start_tu15_tests','--filter','^(native_campaign_ai_path_query_start_tu15|query_)'if kind=='clang-controls'else'^native_campaign_ai_path_query_start_tu15$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
  if kind=='normal':command+=['--target','native_campaign_ai_point_geometry_tu15_tests'];command[command.index('--filter')+1]='^(native_campaign_ai_path_query_start_tu15|native_campaign_ai_point_geometry_tu15)$'
  if kind=='clang-controls':
   for name in variants:command+=['--target','query_'+name]
  if sanitizers:command+=['--sanitizers']
  run=subprocess.run(command,check=False);results.append(dict(kind=kind,exit_code=run.returncode,command=command))
  if run.returncode:break
 report=dict(passed=len(results)==3 and all(x['exit_code']==0 for x in results),source_hashes=pins,source_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),results=results,negative_controls=list(variants),owned_scratch=str(scratch),retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
