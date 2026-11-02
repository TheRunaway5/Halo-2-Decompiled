#!/usr/bin/env python3
"""Unchanged physical316AA0/3223D0 and native array/pool composition proof."""
import argparse,copy,hashlib,inspect,json,os,shutil,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
U=base.UI
WORLD,COLLECTOR,OTHER,FIRST,SECOND,ISLAND1,ISLAND2=U+0x100,U+0x500,U+0x600,U+0x800,U+0x900,U+0xb00,U+0xc00
TABLE,LTABLE,ARRAY,LISTENER,QUEUE,RESULT=U+0xd00,U+0xd20,U+0x1000,U+0x1800,U+0x4000,U+0x7000
OWNERS=(0x316aa0,0x3223d0,0x2e3250,0x2e2e30,0x149da0,0x149e00,0x233250,0x233420,0x2e4d60)
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def cases():
 rows=[]
 def add(name,entry=0x316aa0,flag=0,pattern=(),fallback=False,**extra):
  words=[(WORLD-U+0x12c,flag),(WORLD-U+0x60,ARRAY),(WORLD-U+0x64,len(pattern)),(WORLD-U+0xfc,QUEUE),(WORLD-U+0x100,0),(WORLD-U+0x104,0x80000008),
         (COLLECTOR-U+4,3<<16),(COLLECTOR-U+0xc,FIRST),(COLLECTOR-U+0x10,SECOND),(OTHER-U+4,7<<16),(FIRST-U+0x44,ISLAND1),(SECOND-U+0x44,ISLAND2),
         (ISLAND1-U,TABLE),(ISLAND1-U+0x24,0xffffffff if fallback else 0),(ISLAND2-U,TABLE),(TABLE-U+4,0x2e4d60),(LTABLE-U+0xc,0x2e4d60)]
  for n,present in enumerate(pattern):words +=[(ARRAY-U+n*4,LISTENER+n*0x10 if present else 0),(LISTENER-U+n*0x10,LTABLE)]
  row=dict(name=name,entry=entry,receiver=WORLD,arguments=[RESULT,COLLECTOR]if entry==0x316aa0 else[WORLD,COLLECTOR],user_words=words,**extra);rows.append(row);return row
 patterns=((),(0,),(1,),(0,1),(1,0),(0,1,0,1),(1,0,1,0),(0,0,0),(1,1,1))
 for n,pattern in enumerate(patterns):
  add('notify_'+str(n),entry=0x3223d0,pattern=pattern)
  for fallback in(False,True):add('immediate_'+str(n)+'_'+str(int(fallback)),pattern=pattern,fallback=fallback,flag=(0,2,0xff)[n%3])
 for count in(0,1,3,4):
  for spare in(0,2):
   row=add('deferred_borrowed_'+str(count)+'_'+str(spare),flag=1,count=count,capacity=0x80000000|count+spare)
   row['user_words'] +=[(WORLD-U+0x100,count),(WORLD-U+0x104,0x80000000|count+spare)]
 for count in(1,3,4):
  row=add('deferred_owned_'+str(count),flag=1,count=count,capacity=count,prepare_bytes=count*4)
  row['user_words'] +=[(WORLD-U+0x100,count),(WORLD-U+0x104,count)]
 def mutation(site,address,value):return dict(instruction=site,address=address,written=value.to_bytes(4,'little').hex())
 row=add('callback_backing_current',pattern=(1,1,1));row['user_words'] +=[(0x1400,LISTENER),(0x1404,0),(0x1408,LISTENER+0x20)];row['mutations']=[mutation(0x2e4d60,WORLD+0x60,U+0x1400)]
 row=add('callback_island_cached',pattern=(1,));row['mutations']=[mutation(0x2e4d60,FIRST+0x44,ISLAND2)]
 row=add('callback_result_formal_current',pattern=(1,),retained_arguments=True);row['mutations']=[mutation(0x2e4d60,base.SP+4,RESULT+4)]
 row=add('growth_collector_formal_current',flag=1,count=1,capacity=0x80000001,retained_arguments=True);row['user_words'] +=[(WORLD-U+0x100,1),(WORLD-U+0x104,0x80000001)];row['mutations']=[mutation(0x23328f,base.SP+8,OTHER)]
 row=add('compaction_count_current',entry=0x3223d0,pattern=(0,1,1,1));row['mutations']=[mutation(0x32242a,WORLD+0x64,2)]
 for name,pattern in(('listener',(1,)),('island',())):add('unbound_'+name,pattern=pattern,required_owner=True,stop_boundary=0x2e4d60)
 for name,flag,address,size,writing in (
  ('flag',0,WORLD+0x12c,1,False),('first_body',0,COLLECTOR+0xc,4,False),('first_island',0,FIRST+0x44,4,False),('island_index',0,ISLAND1+0x24,4,False),
  ('world_count',0,WORLD+0x64,4,False),('island_table',0,ISLAND1,4,False),('island_owner',0,TABLE+4,4,False),('result_immediate',0,RESULT,1,True),
  ('capacity',1,WORLD+0x104,4,False),('queue_count',1,WORLD+0x100,4,False),('queue_data',1,WORLD+0xfc,4,False),('queue_write',1,QUEUE,4,True),
  ('collector_ref',1,COLLECTOR+6,2,False),('result_deferred',1,RESULT,1,True)):
  add('fault_'+name,flag=flag,fault=dict(address=address,bytes=size,occurrence=1,write=writing))
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 files=('include/halo2/native_tu15_contact_world_update.hpp','src/physics/native_tu15_contact_world_update.cpp','src/physics/native_tu15_array_growth.cpp','src/physics/native_tu15_array_allocator.cpp','tools/validate_native_contact_world_update_tu15.py','tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp');pins={f:sha(ROOT/f)for f in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],supplied_original_outcomes=False,actual_live_call_claimed=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);banks,inputs=base.reference_inputs(image);base.OWNERS=OWNERS;base.HEADER=WORLD+0xfc;base.DATA=QUEUE
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text();anchor='if(pc==stop)break;';assert observer.count(anchor)==1
  record='''if(pc==0x2e4d60u){const auto b=memory(child.pid,std::uint32_t(regs.rsp)+4u,4);std::uint32_t value{};for(unsigned k=0;k<4;++k)value|=std::to_integer<unsigned>(b[k])<<(8*k);allocator_calls.push_back(J{{"owner",pc},{"receiver",std::uint32_t(regs.rcx)},{"collector",value}});}'''
  observer=observer.replace(anchor,record+anchor).replace('pc==mutation.at("instruction")&&!pending.empty()','pc==mutation.at("instruction")&&(!pending.empty()||pc==0x2e4d60u)');observer_path=scratch/'observer.cpp';observer_path.write_text(observer)
  source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')",'str(observer_path)');source=source.replace('case.get("count",0)*case["arguments"][1]','case.get("count",0)*4');source=source.replace('hex(STOP)]','hex(case.get("stop_boundary",STOP))]');environment=dict(base.__dict__,observer_path=observer_path);exec(source,environment);all_cases=cases();fixture,authority=environment['original'](image,banks,all_cases,scratch);assert all(c['declared_fault']for c in all_cases if 'fault'in c);save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=all_cases))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for name in('native_tu15_contact_world_update.cpp','native_tu15_array_growth.cpp','native_tu15_array_allocator.cpp'):shutil.copyfile(ROOT/'src/physics'/name,project/name)
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_contact_world_update.hpp"\n'+driver;driver=driver.replace('NativeTu15ArrayGrowthMemory array(memory,{c.value("header",0u)},observe);','');anchor='      const auto host_sw=';index=driver.index(anchor)
  extra='''      NativeTu15ContactWorldUpdateServices services;services.allocation=growth;
      const auto callback=[&](auto receiver,auto owner,auto collector,auto&){calls.push_back(J{{"owner",owner.identity},{"receiver",receiver},{"collector",collector}});check(owner.identity==0x2e4d60u,"Unqualified world update callback");if(c.value("required_owner",false))throw Required{};unsigned n{};for(const auto&change:c.value("mutations",J::array())){if(change.at("instruction")==owner.identity&&applied.insert(n).second){memory.write(change.at("address"),unhex(change.at("written")));++mutations;}++n;}};
      services.listener=callback;services.remove_from_island=callback;
      const auto formal=[&](unsigned offset){std::array<std::byte,4>b{};memory.read(0x200f000u+offset,b);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(b[n])<<(8*n);return value;};
''';driver=driver[:index]+extra+driver[index:];begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin);driver=driver[:begin]+'''      try{const auto entry=c.at("entry").get<std::uint32_t>(),world=c.at("receiver").get<std::uint32_t>();const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();
        if(entry==0x3223d0u)native_tu15_notify_contact_world_update(memory,args[0],args[1],fp,services,observe);
        else if(c.value("retained_arguments",false))native_tu15_update_contact_world(memory,world,{[&]{return formal(4);},[&]{return formal(8);}},fp,services,observe);
        else native_tu15_update_contact_world(memory,world,args[0],args[1],fp,services,observe);
'''+driver[end:];driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','');(project/'driver.cpp').write_text(driver);(project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(contact_world_update LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(update driver.cpp native_tu15_contact_world_update.cpp native_tu15_array_growth.cpp native_tu15_array_allocator.cpp)\ntarget_include_directories(update PRIVATE include)\ntarget_compile_features(update PRIVATE cxx_std_20)\ntarget_link_libraries(update PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME contact_update COMMAND update "'+str(fixture/'original.json')+'")\n');results=[];commands=[]
  def run(label,compiler):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),'--target','update','--filter','^contact_update$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];commands.append(command);r=subprocess.run(command);results.append(dict(label=label,exit_code=r.returncode));return r.returncode
  for compiler in a.compilers:assert run(compiler,compiler)==0,'Native contact world update differs'
  kernel=project/'native_tu15_contact_world_update.cpp';text=kernel.read_text();mutants=(('wrong_deferred_stride','native_tu15_grow_array(array,4u','native_tu15_grow_array(array,8u'),('wrong_output','result,1,1','result,0,1'),('wrong_compaction','current_data+destination*4u,next','current_data+destination*4u+4u,next'))
  for name,before,after in mutants:
   assert text.count(before)==1;kernel.write_text(text.replace(before,after));assert run('negative-'+name,a.compilers[0])!=0,'Original failed to reject '+name;kernel.write_text(text)
  executed={pc for c in all_cases for pc in c['executed_sites']};code=_Code(image,'.text');report.update(passed=all(sha(ROOT/f)==v for f,v in pins.items()),original_sha256=NEW_PIN,authority=authority,input_provenance=inputs,original_cases=len(all_cases),ordered_reads=sum(len(c['reads'])for c in all_cases),ordered_stores=sum(len(c['writes'])for c in all_cases),genuine_service_entries=sum(len(c['allocator_calls'])for c in all_cases),fault_prefixes=sum(c['declared_fault']for c in all_cases),declared_input_mutations=sum(c['mutations_applied']for c in all_cases),unbound_callback_prefixes=sum(c.get('required_owner',False)for c in all_cases),body_coverage={hex(owner):dict(total=len(base.complete_body(code,owner).code),executed=len(set(base.complete_body(code,owner).code)&executed))for owner in OWNERS},results=results,commands=commands,negative_controls=[m[0]for m in mutants],scope='Declared valid world/body/island/collector DATA; full strict ordered operands/preimages/banks/FP. Real native149DA0/E00/233250/233420/growth composition. Genuine RET4 component callbacks only; actual3201B0 or unknown listeners remain explicit. RMW prewrite instruction atomicity is not claimed.')
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
