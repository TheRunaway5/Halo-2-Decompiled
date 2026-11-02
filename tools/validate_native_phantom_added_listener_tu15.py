#!/usr/bin/env python3
"""Qualify actual282700/nativeB1870 with explicit genuine BA570 entry stops."""
import argparse,hashlib,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
U=base.UI;EVENT=U+0x100;WRAPPER=U+0x200;BODY=U+0x300;PROPS=U+0x400;OWNER=U+0x600;COMPONENTS=U+0x800;MODE=U+0x1800;OBJECT_OWNER=U+0x1a00;ENTRIES=U+0x1b00;OBJECT=U+0x1c00
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def cases():
 rows=[]
 def add(name,kind=1,body=BODY,count=4,index=1,datum=0xabcd0001,mode=4,protected=True,entry=0x282700,**extra):
  words=[(EVENT-U+4,WRAPPER),(WRAPPER-U+0x18,kind),(WRAPPER-U+0x20,body),(BODY-U+0x34,count),(BODY-U+0x30,PROPS),(OWNER-U+0x44,COMPONENTS),(MODE-U+0xc,mode),(OBJECT_OWNER-U+0x44,ENTRIES)]
  for n in range(5):words.extend([(PROPS-U+n*8,0x2001 if n==index else 0x1000+n),(PROPS-U+n*8+4,datum if n==index else 0xf0000000+n),(COMPONENTS-U+n*160+4,0x55aa0001),(COMPONENTS-U+n*160+8,0xbeef0001),(ENTRIES-U+n*12+8,OBJECT+n*0x100),(OBJECT-U+n*0x100+0xd4,0x12340000+n if protected else 0xffffffff)])
  selected=kind==1 and body and datum!=0xffffffff
  required=entry==0x282700 and selected and not(mode==4 and protected)
  row=dict(name=name,entry=entry,receiver=0xa5a5a5a5,incoming_eax=0xbeef0001,arguments=[EVENT]if entry==0x282700 else[],user_words=words,required_owner=bool(required),**extra)
  if required:row['stop_boundary']=0xba570
  rows.append(row)
 for kind in(0,1,2,0xffffffff):add('wrapper_'+str(kind),kind=kind)
 add('null_body',body=0)
 for count in(0,0xffffffff,0x80000000,1,2,4,5):
  for index in(0,1,3):add(f'property_{count}_{index}',count=count,index=index)
 for datum in(0,0x12340000,0xabcd0001,0xbeef0003,0xffffffff):add('datum_'+hex(datum),datum=datum)
 for mode in(0,1,2,3,4,5,255):
  for protected in(False,True):add(f'listener_mode_{mode}_{protected}',mode=mode,protected=protected)
  for protected in(False,True):add(f'predicate_mode_{mode}_{protected}',entry=0xb1870,mode=mode,protected=protected)
 faults=[('event',EVENT+4,4,False),('kind',WRAPPER+0x18,4,False),('body',WRAPPER+0x20,4,False),('count',BODY+0x34,4,False),('backing',BODY+0x30,4,False),('property_key',PROPS,4,False),('property_value',PROPS+12,4,False),('component_owner',0x52be5c,4,False),('component_data',OWNER+0x44,4,False),('flags',COMPONENTS+164,4,False),('cached_object',COMPONENTS+168,4,False),('flag_store',COMPONENTS+164,4,True),('mode_global',0x4f3218,4,False),('mode_byte',MODE+0xc,1,False),('objects',0x4ecbd0,4,False),('object_backing',OBJECT_OWNER+0x44,4,False),('object_pointer',ENTRIES+20,4,False),('object_word',OBJECT+0x100+0xd4,4,False)]
 for name,address,width,writing in faults:add('fault_'+name,fault=dict(address=address,bytes=width,occurrence=1,write=writing))
 for name,address,value in(('cached_object',COMPONENTS+168,0xbeef0003),('mode_reread',MODE+0xc,3),('object_backing',OBJECT_OWNER+0x44,ENTRIES+12)):
  add('alias_'+name,mutations=[dict(instruction=0x282763,address=address,written=struct.pack('<I',value).hex())])
  if name=='mode_reread':rows[-1].update(required_owner=True,stop_boundary=0xba570)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();files=('include/halo2/native_tu15_phantom_added_listener.hpp','src/physics/native_tu15_phantom_added_listener.cpp','tools/validate_native_phantom_added_listener_tu15.py',*base.FILES);pins={n:sha(ROOT/n)for n in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],source_private_raw_ledger_claimed=False,BA570_implemented=False,actual_live_profile_claimed=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=(0x282700,0xb1870);banks={0x52be5c:struct.pack('<I',OWNER),0x4f3218:struct.pack('<I',MODE),0x4ecbd0:struct.pack('<I',OBJECT_OWNER)}
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text();anchor='if(pc==stop)break;';assert observer.count(anchor)==1
  record='''if(pc==0xba570u){const auto raw=memory(child.pid,std::uint32_t(regs.rsp)+4u,4);std::uint32_t object{};for(unsigned n=0;n<4;++n)object|=std::to_integer<unsigned>(raw[n])<<(8u*n);allocator_calls.push_back(J{{"owner",pc},{"datum",object}});}'''
  observer=observer.replace(anchor,record+anchor).replace('{"return_word",std::uint32_t(regs.rax)}','{"return_word",std::uint32_t(regs.rax)},{"after_esp",std::uint32_t(regs.rsp)}');observer_path=scratch/'observer.cpp';observer_path.write_text(observer)
  source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')",'str(observer_path)').replace('hex(STOP)]','hex(case.get("stop_boundary",STOP))]').replace(" mov ${case['receiver']},%ecx"," mov ${case.get('incoming_eax',0)},%eax\n mov ${case['receiver']},%ecx")
  environment=dict(base.__dict__,observer_path=observer_path);exec(source,environment);all_cases=cases();fixture,authority=environment['original'](image,banks,all_cases,scratch);assert all(c['declared_fault']for c in all_cases if'fault'in c)
  for c in all_cases:
   if not c.get('required_owner')and not c.get('fault'):assert c['after_esp']==base.SP+(8 if c['entry']==0x282700 else 4)
  save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=all_cases));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(ROOT/files[1],project/'kernel.cpp')
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_phantom_added_listener.hpp"\n'+driver;begin=driver.index('      NativeTu15ArrayGrowthMemory array');end=driver.index('      const auto host_sw=',begin)
  setup='''      NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};NativeTu15PhantomAddedListenerStats listener_activity;NativeTu15PhantomAddedListenerServices services;
      services.delete_object=[&](auto datum,auto&){calls.push_back(J{{"owner",0xba570u},{"datum",datum}});throw Required{};};
''';driver=driver[:begin]+setup+driver[end:];begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin);driver=driver[:begin]+'''      try{if(c.at("entry")==0x282700u)native_tu15_notify_gameplay_phantom_added(memory,{c.at("arguments")[0]},fp,services,observe,&listener_activity);
        else result=native_tu15_object_phantom_delete_protected(memory,c.at("incoming_eax"));
'''+driver[end:];driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','');driver=driver.replace('(c.at("entry")==0x233250u||c.at("entry")==0x149da0u))check(result==c.at("return_word")','c.at("entry")==0xb1870u)check(result==(c.at("return_word").get<unsigned>()&0xffu)')
  driver=driver.replace('      check(calls==c.at("allocator_calls"),name+" actual allocator request order differs");','      check(calls==c.at("allocator_calls"),name+" actual delete child boundary differs");')
  (project/'driver.cpp').write_text(driver);(project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(phantomAdded LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(listener driver.cpp kernel.cpp)\ntarget_include_directories(listener PRIVATE include)\ntarget_compile_features(listener PRIVATE cxx_std_20)\ntarget_link_libraries(listener PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME phantom_added COMMAND listener "'+str(fixture/'original.json')+'")\n');results=[];commands=[]
  def run(label,compiler):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),'--target','listener','--filter','^phantom_added$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];commands.append(command);r=subprocess.run(command);results.append(dict(label=label,exit_code=r.returncode));return r.returncode
  for compiler in a.compilers:assert run(compiler,compiler)==0,'Native phantom added listener differs'
  target=project/'kernel.cpp';text=target.read_text();mutants=(('wrong_flag','flags|0x1000u','flags|0x2000u'),('wrong_null','datum==0xffffffffu','datum==0u'),('wrong_mode','header+0xcu,1)!=4u','header+0xcu,1)==4u'))
  for name,before,after in mutants:
   assert text.count(before)==1;target.write_text(text.replace(before,after));assert run('negative-'+name,a.compilers[0])!=0,'Original failed to reject '+name;target.write_text(text)
  code=_Code(image,'.text');executed={pc for c in all_cases for pc in c['executed_sites']};report.update(passed=all(sha(ROOT/n)==v for n,v in pins.items()),original_sha256=NEW_PIN,authority=authority,original_cases=len(all_cases),ordered_reads=sum(len(c['reads'])for c in all_cases),ordered_stores=sum(len(c['writes'])for c in all_cases),genuine_BA570_entry_prefixes=sum(c.get('required_owner',False)for c in all_cases),fault_prefixes=sum(c['declared_fault']for c in all_cases),input_mutations=sum(c['mutations_applied']for c in all_cases),body_coverage={hex(o):dict(total=len(base.complete_body(code,o).code),executed=len(set(base.complete_body(code,o).code)&executed))for o in base.OWNERS},commands=commands,results=results,mutants_rejected=[m[0]for m in mutants],B1870_return_contract='AL boolean; the actual282700 caller testsAL and ignores upperEAX',scope=__doc__)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
