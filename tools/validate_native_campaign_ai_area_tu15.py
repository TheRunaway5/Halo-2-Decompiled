#!/usr/bin/env python3
"""Owned compiler/sanitizer/actual-mutation qualification of native area-query selector."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
 names=['cmake/CampaignNativeAiArea.cmake','src/campaign/ai/native_campaign_ai_area_tu15.cpp','src/campaign/ai/native_campaign_ai_area_tu15_binding.cpp','tests/native_campaign_ai_area_tu15_tests.cpp','tests/fixtures/native_campaign_ai_area_tu15.json.gz','tools/test_native_campaign_screen_lifecycle_tu15.py']
 for name in names:
  dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
 core=(project/names[1]).read_text();variants={}
 for name,needle,replacement in [('zero_pair_count','if(std::bit_cast<std::int8_t>(std::uint8_t(read(squad+0x30u,1)))<0)return false;','if(std::bit_cast<std::int8_t>(std::uint8_t(read(squad+0x30u,1)))<=0)return false;'),('type_unmasked','(5u&(1u<<(kind&31u)))!=0','kind<32u && (5u&(1u<<(kind&31u)))!=0'),('class_inverted','(read(area+0x20u)&1u)==unsigned(mode)','(read(area+0x20u)&1u)!=unsigned(mode)'),('missing_second_zone_read','if(word(definition+0x38u)==none)','if(zone==none)'),('al_only_return','abi.gp[0]=(abi.gp[0]&0xffff0000u)|result','abi.gp[0]=(abi.gp[0]&0xffffff00u)|(result&0xffu)')]:
  owner=core if name!='al_only_return' else (project/names[2]).read_text()
  assert owner.count(needle)==1,(name,needle);variants[name]=owner.replace(needle,replacement)

 for name,text in variants.items():(project/f'src/campaign/ai/{name}.cpp').write_text(text)
 (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('original ordered reads differ','original area result differs','candidate AX16/RET8/native preserved GPR contract differs','original query completion differs')),(r.returncode,r.stdout,r.stderr)
print('Expected actual area semantic mutation rejection:',r.stderr.strip())
''')
 cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeAiArea LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeAiArea.cmake)
'''
 for name in variants:
  sources=('src/campaign/ai/native_campaign_ai_area_tu15.cpp src/campaign/ai/'+name+'.cpp') if name=='al_only_return' else ('src/campaign/ai/'+name+'.cpp src/campaign/ai/native_campaign_ai_area_tu15_binding.cpp')
  cmake+=f'''add_executable(reset_{name} tests/native_campaign_ai_area_tu15_tests.cpp {sources})
target_include_directories(reset_{name} PRIVATE include)
target_compile_features(reset_{name} PRIVATE cxx_std_20)
target_link_libraries(reset_{name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME reset_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:reset_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_ai_area_tu15.json.gz")
'''
 (project/'CMakeLists.txt').write_text(cmake);results=[];pins={name:sha(ROOT/name)for name in names};pins.update({str(path.relative_to(ROOT)):sha(path)for path in(ROOT/'include/halo2/native_campaign_ai_area_tu15.hpp',ROOT/'include/halo2/native_campaign_ai_area_tu15_binding.hpp',Path(__file__))})
 for kind,compiler,sanitizers in [('normal','g++',False),('sanitizers','g++',True),('clang-controls','clang++',False)]:
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_campaign_ai_area_tu15_tests','--filter','^(native_campaign_ai_area_tu15|reset_)'if kind=='clang-controls'else'^native_campaign_ai_area_tu15$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
  if kind=='clang-controls':
   for name in variants:command+=['--target','reset_'+name]
  if sanitizers:command+=['--sanitizers']
  run=subprocess.run(command,check=False);results.append(dict(kind=kind,exit_code=run.returncode,command=command))
  if run.returncode:break
 report=dict(passed=len(results)==3 and all(x['exit_code']==0 for x in results),source_hashes=pins,source_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),results=results,negative_controls=list(variants),owned_scratch=str(scratch),retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
