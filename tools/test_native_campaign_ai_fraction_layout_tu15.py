#!/usr/bin/env python3
"""Owned focused layout admission validation; no mission or cache rewrite."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'layout';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
 names=['src/assets/cache.cpp','src/campaign/scripting/mcc_script_layout.cpp','src/campaign/scripting/mcc_script_layout.inc','src/campaign/mcc_cairo_vanilla_scripts.inc','tests/native_campaign_ai_fraction_layout_tu15_tests.cpp']
 for name in names:
  dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
 (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(NativeFractionLayout LANGUAGES CXX)
include(CTest)
option(HALO2_SANITIZERS "Instrument focused fraction layout" OFF)
if(HALO2_SANITIZERS)
 add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
 add_link_options(-fsanitize=address,undefined)
endif()
add_executable(fraction_layout tests/native_campaign_ai_fraction_layout_tu15_tests.cpp src/campaign/scripting/mcc_script_layout.cpp src/assets/cache.cpp)
target_include_directories(fraction_layout PRIVATE include src)
target_compile_features(fraction_layout PRIVATE cxx_std_20)
target_compile_options(fraction_layout PRIVATE -ffunction-sections -fdata-sections)
target_link_options(fraction_layout PRIVATE -Wl,--gc-sections)
add_test(NAME fraction_layout COMMAND fraction_layout)
''');results=[]
 for name,sanitizers in [('normal',False),('sanitizers',True)]:
  cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/name),'--target','fraction_layout','--filter','^fraction_layout$','--jobs','2','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log']
  if sanitizers:cmd+=['--sanitizers']
  r=subprocess.run(cmd,check=False);results.append(dict(kind=name,exit_code=r.returncode,command=cmd))
  if r.returncode:break
 generator=subprocess.run([sys.executable,str(ROOT/'tools/generate_mcc_script_layout.py'),'--check'],check=False)
 report=dict(passed=len(results)==2 and all(x['exit_code']==0 for x in results)and generator.returncode==0,results=results,generator_check=generator.returncode,source_hashes={n:sha(ROOT/n)for n in names+['tools/mcc-script-layout.json','tools/generate_mcc_script_layout.py',str(Path(__file__).relative_to(ROOT))]},owned_scratch=str(scratch),retained_large_artifacts=[])
 (a.output/'layout.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
