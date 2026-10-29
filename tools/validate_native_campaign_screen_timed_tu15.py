#!/usr/bin/env python3
"""Disposable GCC/Clang and sanitizer qualification plus actual semantic mutations."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--mode',choices=('all','sanitizers'),default='all');a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
 names=['cmake/CampaignNativeScreenTimed.cmake','src/campaign/native_campaign_screen_timed_tu15.cpp','src/campaign/native_campaign_screen_timed_tu15_binding.cpp','tests/native_campaign_screen_timed_tu15_tests.cpp','tests/fixtures/native_campaign_screen_timed_tu15.json.gz','tools/test_native_campaign_screen_lifecycle_tu15.py','src/campaign/source_campaign_screen_native_tu15.cpp','src/campaign/native_campaign_screen_lifecycle_tu15.cpp','src/campaign/native_campaign_screen_lifecycle_tu15_binding.cpp']
 for name in names:
  target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
 core=(project/names[1]).read_text();mutations={}
 for name,needle,altered in [('omit_opacity_end','bits(add(opacity_start,opacity_end))','bits(opacity_start)'),('nan_equal','return equal&&ordered;','return equal;'),('omit_dof_retire','if(completed || no_ranges)','if(false)'),('wrong_capture','bank.publish(0x20469,0xa0,1,1)','bank.publish(0x20469,0xa0,0,1)'),('cached_duration','add_single(arguments.word(first+2u))','add_single(bits(duration))')]:
  assert core.count(needle)==1,(name,needle);path=project/f'src/campaign/{name}.cpp';path.write_text(core.replace(needle,altered));mutations[name]=path
 (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(x in r.stderr for x in ('original ordered store differs','original store count differs','original FP status differs','original ordered reads differ','original completion differs')),(r.returncode,r.stdout,r.stderr)
print('Expected physical-original mutation rejection:',r.stderr.strip()[-1000:])
''')
 cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeCampaignTimedScreen LANGUAGES CXX)
include(CTest)
option(HALO2_SANITIZERS "Enable address/undefined sanitizers" OFF)
if(HALO2_SANITIZERS)
 add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
 add_link_options(-fsanitize=address,undefined)
endif()
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeScreenTimed.cmake)
'''
 for name in mutations:
  cmake+=f'''add_executable(timed_{name} tests/native_campaign_screen_timed_tu15_tests.cpp src/campaign/{name}.cpp src/campaign/native_campaign_screen_timed_tu15_binding.cpp src/campaign/source_campaign_screen_native_tu15.cpp src/campaign/native_campaign_screen_lifecycle_tu15.cpp src/campaign/native_campaign_screen_lifecycle_tu15_binding.cpp)
target_include_directories(timed_{name} PRIVATE include)
target_compile_features(timed_{name} PRIVATE cxx_std_20)
target_compile_options(timed_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(timed_{name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME timed_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:timed_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_screen_timed_tu15.json.gz")
'''
 (project/'CMakeLists.txt').write_text(cmake);results=[];pins={name:sha(ROOT/name)for name in names};pins.update({str(path.relative_to(ROOT)):sha(path)for path in(ROOT/'include/halo2/native_campaign_screen_timed_tu15.hpp',ROOT/'include/halo2/native_campaign_screen_timed_tu15_binding.hpp',Path(__file__))})
 for kind,compiler,sanitizers in [('normal','g++',False),('sanitizers','g++',True),('clang-controls','clang++',False)]:
  if a.mode!='all' and kind!=a.mode:continue
  targets=['native_campaign_screen_timed_tu15_tests']+(['timed_'+n for n in mutations]if kind=='clang-controls'else[])
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--filter','^(native_campaign_screen_timed_tu15|timed_)','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
  for target in targets:command+=['--target',target]
  # Unbuilt mutation registrations are disabled for the focused ordinary runs.
  if kind!='clang-controls':command[command.index('--filter')+1]='^native_campaign_screen_timed_tu15$'
  if sanitizers:command+=['--sanitizers']
  r=subprocess.run(command,check=False);results.append(dict(kind=kind,exit_code=r.returncode,command=command))
  if r.returncode:break
 report=dict(passed=len(results)==(3 if a.mode=='all' else 1) and all(r['exit_code']==0 for r in results),commands=results,negative_controls=list(mutations),source_hashes=pins,source_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),owned_scratch=str(scratch),retained_large_artifacts=[])
 (a.output/('validation.json' if a.mode=='all' else 'validation-'+a.mode+'.json')).write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
