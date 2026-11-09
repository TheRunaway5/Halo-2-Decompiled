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
 def add(name,count,info=INFO,**extra):
  row=dict(name=name,entry=0x31f740,receiver=COLLECTOR,arguments=[info],user_words=[(COLLECTOR-U+0x94,count)],**extra);rows.append(row);return row
 values=(0,1,2,3,4,5,7,16,255,256,0x7ffffffe,0x7fffffff,0x80000000,0x80000001,0xfffffffe,0xffffffff)
 for value in (*values,*[random.Random(145*n+1).randrange(1<<32)for n in range(64)]):add('count_'+hex(value)+'_'+str(len(rows)),value)
 for offset in(0x88,0x8c,0x90,0x94):
  for value in(0,2,5,0x80000000):add('alias_'+hex(offset)+'_'+hex(value),value,info=COLLECTOR+offset)
 for site,value in((0x325e96,5),(0x325e9e,0x80000000),(0x325eb8,7)):
  add('current_count_'+hex(site),2,mutations=[dict(instruction=site,address=COLLECTOR+0x94,written=value.to_bytes(4,'little').hex())])
 for address,size,occurrence,writing in ((COLLECTOR+0x94,4,1,False),
    *((INFO+offset,4,n,True)for offset in(0,4,8,12)for n in(1,2)),
    *((INFO+offset,4,3,True)for offset in(4,8,12))):
  add('fault_'+hex(address)+'_'+str(occurrence)+'_'+str(writing),2,fault=dict(address=address,bytes=size,occurrence=occurrence,write=writing))
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 files=('include/halo2/native_tu15_extended_constraint_info.hpp','src/physics/native_tu15_extended_constraint_info.cpp','include/halo2/native_tu15_contact_collector_info.hpp','src/physics/native_tu15_contact_collector_info.cpp','include/halo2/native_tu15_island_partition_metadata.hpp','tools/validate_native_conveyor_constraint_info_tu15.py','tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp');pins={f:sha(ROOT/f)for f in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],supplied_original_outcomes=False,actual_live_input_claimed=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);assert int.from_bytes(image.read(0x41edb8,4),'little')==0x31f740;assert image.read(0x31f746,5).hex()=='e845670000';assert int.from_bytes(image.read(0x41f478+0x28,4),'little')==0x325e90;base.OWNERS=(0x31f740,0x325e90);all_cases=cases();fixture,authority=base.original(image,{},all_cases,scratch);assert all(c['declared_fault']for c in all_cases if 'fault'in c);save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=all_cases))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(ROOT/'src/physics/native_tu15_extended_constraint_info.cpp',project/'native.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_contact_collector_info.cpp',project/'collector.cpp');driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_extended_constraint_info.hpp"\n'+driver;begin=driver.index('      NativeTu15ArrayGrowthMemory array');end=driver.index('      const auto host_sw=',begin);driver=driver[:begin]+'''      NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
'''+driver[end:];begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin);driver=driver[:begin]+'''      try{result=native_tu15_get_conveyor_constraint_info(memory,c.at("receiver"),{c.at("arguments")[0]},observe);
'''+driver[end:];driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','');needle='      if(!failed&&!required&&';assert driver.count(needle)==1;driver=driver.replace(needle,'      if(!failed&&!required)check(result==c.at("return_word"),name+" actual info result differs");\n'+needle);(project/'driver.cpp').write_text(driver);(project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(collector_info LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(info driver.cpp native.cpp collector.cpp)\ntarget_include_directories(info PRIVATE include)\ntarget_compile_features(info PRIVATE cxx_std_20)\ntarget_link_libraries(info PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME collector_info COMMAND info "'+str(fixture/'original.json')+'")\n');results=[];commands=[]
  def run(label,compiler):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),'--target','info','--filter','^collector_info$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];commands.append(command);r=subprocess.run(command);results.append(dict(label=label,exit_code=r.returncode));return r.returncode
  for compiler in a.compilers:assert run(compiler,compiler)==0,'Native collector info differs'
  kernel=project/'native.cpp';text=kernel.read_text();mutants=[('wrong_extension','0x31f74bu,0x28u','0x31f74bu,0x20u'),('wrong_info_identity','return info.identity;','return info.identity+4u;')]
  for name,before,after in mutants:
   assert text.count(before)==1;kernel.write_text(text.replace(before,after));assert run('negative-'+name,a.compilers[0])!=0,'Original failed to reject '+name;kernel.write_text(text)
  executed={pc for c in all_cases for pc in c['executed_sites']};code=_Code(image,'.text');report.update(passed=all(sha(ROOT/f)==v for f,v in pins.items()),original_sha256=NEW_PIN,authority=authority,original_cases=len(all_cases),ordered_reads=sum(len(c['reads'])for c in all_cases),ordered_stores=sum(len(c['writes'])for c in all_cases),fault_prefixes=sum(c['declared_fault']for c in all_cases),declared_input_mutations=sum(c['mutations_applied']for c in all_cases),alias_cases=sum(c['name'].startswith('alias_')for c in all_cases),body_coverage=dict(total=len(base.complete_body(code,0x31f740).code),executed=len(set(base.complete_body(code,0x31f740).code)&executed)),results=results,commands=commands,negative_controls=[m[0]for m in mutants],scope='Wholepublic7-site31F740 composedwithsole31-site325E90, signedwrapping/alias/mutation/faultprefixes, orderedDATA/FP. Currentoutput+8 extension28h, originalEAXsuppliedinfoidentity. ActualselectednativeRegroupcaller/live input qualification separate.')
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
