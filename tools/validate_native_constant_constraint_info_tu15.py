#!/usr/bin/env python3
"""Physical31-site325E90 info fill: all DATA/FP/alias/fault prefixes."""
import argparse,hashlib,inspect,json,os,random,shutil,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
U,COLLECTOR,INFO=base.UI,base.UI+0x100,base.UI+0x400
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def cases():
 rows=[]
 for n in range(64):rows.append(dict(name='constant_'+str(n),entry=0x31d630,receiver=COLLECTOR,arguments=[INFO],user_words=[(COLLECTOR-U+0x94,random.Random(9*n).randrange(1<<32))]))
 for offset in(0,1,4,8,0x90,0x94):rows.append(dict(name='alias_'+hex(offset),entry=0x31d630,receiver=COLLECTOR,arguments=[COLLECTOR+offset],user_words=[]))
 for offset in(0,4,8,12):rows.append(dict(name='fault_'+str(offset),entry=0x31d630,receiver=COLLECTOR,arguments=[INFO],user_words=[],fault=dict(address=INFO+offset,bytes=4,occurrence=1,write=True)))
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 files=('include/halo2/native_tu15_constant_constraint_info.hpp','src/physics/native_tu15_constant_constraint_info.cpp','include/halo2/native_tu15_island_partition_metadata.hpp','tools/validate_native_constant_constraint_info_tu15.py','tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp');pins={f:sha(ROOT/f)for f in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],supplied_original_outcomes=False,actual_live_input_claimed=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);assert int.from_bytes(image.read(0x41f478+0x28,4),'little')==0x325e90;base.OWNERS=(0x31d630,);all_cases=cases();fixture,authority=base.original(image,{},all_cases,scratch);assert all(c['declared_fault']for c in all_cases if 'fault'in c);save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=all_cases))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(ROOT/'src/physics/native_tu15_constant_constraint_info.cpp',project/'native.cpp');driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_constant_constraint_info.hpp"\n'+driver;begin=driver.index('      NativeTu15ArrayGrowthMemory array');end=driver.index('      const auto host_sw=',begin);driver=driver[:begin]+'''      NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
'''+driver[end:];begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin);driver=driver[:begin]+'''      try{native_tu15_get_constant_constraint_info(memory,{c.at("arguments")[0]},observe);
'''+driver[end:];driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','');(project/'driver.cpp').write_text(driver);(project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(collector_info LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(info driver.cpp native.cpp)\ntarget_include_directories(info PRIVATE include)\ntarget_compile_features(info PRIVATE cxx_std_20)\ntarget_link_libraries(info PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME collector_info COMMAND info "'+str(fixture/'original.json')+'")\n');results=[];commands=[]
  def run(label,compiler):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),'--target','info','--filter','^collector_info$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];commands.append(command);r=subprocess.run(command);results.append(dict(label=label,exit_code=r.returncode));return r.returncode
  for compiler in a.compilers:assert run(compiler,compiler)==0,'Native collector info differs'
  kernel=project/'native.cpp';text=kernel.read_text();mutants=[('wrong_info_extent','8u,0x44u','8u,0x40u'),('wrong_info_count','12u,7u','12u,6u')]
  for name,before,after in mutants:
   assert text.count(before)==1;kernel.write_text(text.replace(before,after));assert run('negative-'+name,a.compilers[0])!=0,'Original failed to reject '+name;kernel.write_text(text)
  executed={pc for c in all_cases for pc in c['executed_sites']};code=_Code(image,'.text');report.update(passed=all(sha(ROOT/f)==v for f,v in pins.items()),original_sha256=NEW_PIN,authority=authority,original_cases=len(all_cases),ordered_reads=sum(len(c['reads'])for c in all_cases),ordered_stores=sum(len(c['writes'])for c in all_cases),fault_prefixes=sum(c['declared_fault']for c in all_cases),declared_input_mutations=sum(c['mutations_applied']for c in all_cases),alias_cases=sum(c['name'].startswith('alias_')for c in all_cases),body_coverage=dict(total=len(base.complete_body(code,0x31d630).code),executed=len(set(base.complete_body(code,0x31d630).code)&executed)),results=results,commands=commands,negative_controls=[m[0]for m in mutants],scope='Wholepublic6-site31D630 constantinfo, exactstoreorder0/+8/+4/+C, unusedreceiver/unaligned/alias/faultprefixes, DATA/FP. ActualnativeRegroupcaller composition separate.')
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
