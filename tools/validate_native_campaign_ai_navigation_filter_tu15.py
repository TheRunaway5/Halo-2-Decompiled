#!/usr/bin/env python3
"""Owned native navigation-filter normal/sanitizer/mutation qualification."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
 names=['cmake/CampaignNativeAiNavigationFilter.cmake','src/campaign/ai/native_campaign_ai_navigation_filter_tu15.cpp','tests/native_campaign_ai_navigation_filter_tu15_tests.cpp','tests/fixtures/native_campaign_ai_navigation_filter_tu15.json.gz','tools/test_native_campaign_screen_lifecycle_tu15.py','src/campaign/ai/native_campaign_ai_character_tu15.cpp']
 for name in names:
  dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
 core=(project/names[1]).read_text();variants={}
 pairs=[('skip_action_mask','publish(memory,observe,0x2015b3,destination,current&(restriction<<1u));','publish(memory,observe,0x2015b3,destination,current);'),('wrong_vehicle_word','read(memory,vehicle_navigation+0xb0u,2)','0u'),('wrong_kind_cap','kinds>6 ? 6u','kinds>5 ? 5u'),('skip_kind_reread','if(std::int16_t(read(memory,navigation+0x1au,2))>=7)','if(kinds>=7)'),('clear_padding','publish(memory,observe,0x20162b,destination+0xc,0,1);','publish(memory,observe,0x20162b,destination+0xc,0,2);')]
 for name,needle,replacement in pairs:
  assert core.count(needle)==1,(name,core.count(needle));variants[name]=core.replace(needle,replacement)
 for name,text in variants.items():(project/f'src/campaign/ai/{name}.cpp').write_text(text)
 (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('retained navigation-filter bytes differ','navigation-filter ordered reads differ','navigation-filter ordered store differs','navigation-filter store count differs','Missing navigation-filter storage','Navigation-filter storage extent differs')),(r.returncode,r.stdout,r.stderr)
print('Expected physical-original navigation-filter mutation rejection:',r.stderr.strip()[-4000:])
''')
 cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeCampaignNavigationFilter LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeAiNavigationFilter.cmake)
'''
 for name in variants:
  cmake+=f'''add_executable(filter_{name} tests/native_campaign_ai_navigation_filter_tu15_tests.cpp src/campaign/ai/{name}.cpp src/campaign/ai/native_campaign_ai_character_tu15.cpp)
target_include_directories(filter_{name} PRIVATE include)
target_compile_features(filter_{name} PRIVATE cxx_std_20)
target_compile_options(filter_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(filter_{name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME filter_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:filter_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_ai_navigation_filter_tu15.json.gz")
'''
 (project/'CMakeLists.txt').write_text(cmake);results=[];pins={name:sha(ROOT/name)for name in names};pins.update({str(path.relative_to(ROOT)):sha(path)for path in(ROOT/'include/halo2/native_campaign_ai_navigation_filter_tu15.hpp',ROOT/'include/halo2/native_campaign_ai_character_tu15.hpp',ROOT/'include/halo2/physics_contact_record.hpp',Path(__file__))})
 for kind,compiler,sanitizers in [('normal','g++',False),('sanitizers','g++',True),('clang-controls','clang++',False)]:
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_campaign_ai_navigation_filter_tu15_tests','--filter','^(native_campaign_ai_navigation_filter_tu15|filter_)'if kind=='clang-controls'else'^native_campaign_ai_navigation_filter_tu15$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
  if kind=='clang-controls':
   for name in variants:command+=['--target','filter_'+name]
  if sanitizers:command+=['--sanitizers']
  run=subprocess.run(command,check=False);results.append(dict(kind=kind,exit_code=run.returncode,command=command))
  if run.returncode:break
 report=dict(passed=len(results)==3 and all(x['exit_code']==0 for x in results),source_hashes=pins,source_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),results=results,negative_controls=list(variants),owned_scratch=str(scratch),retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
