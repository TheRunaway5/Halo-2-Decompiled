#!/usr/bin/env python3
"""Strict physical221-site gameplay contact-removal DATA composition."""
import argparse,hashlib,inspect,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
U=base.UI
FIRST,SECOND,FP,SPROP,EVENT,OWNER,DATA,COMPONENT,ROWS,PARTS,CONSTRAINT_OWNER,CONSTRAINTS=U+0x100,U+0x300,U+0x500,U+0x600,U+0x800,U+0x1000,U+0x2000,U+0x20a0,U+0x4000,U+0x7000,U+0x1800,U+0x8000
OWNERS=(0x281d40,0x1d8880,0x1d8970,0x22e1b0)
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def cases():
 rows=[]
 def add(name,entry=0x281d40,count=3,index=1,subject_first=True,effect=False,part_release=False,no_match=False,**extra):
  first_datum,second_datum=0xabcd0001,0xbcde0002
  subject,other=(FIRST,SECOND)if subject_first else(SECOND,FIRST)
  other_datum=second_datum if subject_first else first_datum
  component=COMPONENT if subject_first else DATA+2*160
  words=[(FIRST-U+0x30,FP),(FIRST-U+0x34,3),(SECOND-U+0x30,SPROP),(SECOND-U+0x34,3),
         (FP-U,0x2003),(FP-U+4,0),(FP-U+8,0x2001),(FP-U+12,first_datum),(FP-U+16,0x2002),(FP-U+20,0),
         (SPROP-U,0x2002),(SPROP-U+4,0),(SPROP-U+8,0x2003),(SPROP-U+12,0),(SPROP-U+16,0x2001),(SPROP-U+20,second_datum),
         (EVENT-U,0xa55a0007),(EVENT-U+4,U+0xa00),(EVENT-U+8,FIRST),(EVENT-U+12,SECOND),(EVENT-U+16,subject),
         (OWNER-U+0x44,DATA),(CONSTRAINT_OWNER-U+0x44,CONSTRAINTS),(CONSTRAINTS-U+160+8,2)]
  for n in range(3):words +=[(DATA-U+n*160+4,1),(DATA-U+n*160+0x88,ROWS),(DATA-U+n*160+0x8c,count),(DATA-U+n*160+0x70,PARTS)]
  for n in range(max(count,1)):
   row=ROWS+n*72;words +=[(row-U,0x11110000|(7 if n==index and not no_match else 9)),(row-U+8,0xa5a50001 if effect and n==index else 0xffffffff),(row-U+0x14,other_datum),(row-U+0x44,0x20 if part_release and n==index else 0)]
  args=[EVENT];receiver=0;incoming=0
  if entry==0x1d8880:args=[component,EVENT];receiver=subject;incoming=other
  elif entry==0x1d8970:args=[component,index]
  row=dict(name=name,entry=entry,receiver=receiver,incoming_eax=incoming,arguments=args,user_words=words,**extra)
  if part_release:row.update(required_owner=True,stop_boundary=0x1e0a60)
  rows.append(row);return row
 for entry in(0x281d40,0x1d8880):
  for count in(0,1,3,5):
   for index in(range(count)if count else(0,)):
    add(hex(entry)+'_'+str(count)+'_'+str(index),entry=entry,count=count,index=index)
    if count:add('reverse_'+hex(entry)+'_'+str(count)+'_'+str(index),entry=entry,count=count,index=index,subject_first=False)
  add('missing_'+hex(entry),entry=entry,no_match=True)
 for count in(1,2,3,5):
  for index in range(count):
   add('row_'+str(count)+'_'+str(index),entry=0x1d8970,count=count,index=index)
   add('row_effect_'+str(count)+'_'+str(index),entry=0x1d8970,count=count,index=index,effect=True)
 for entry in(0x281d40,0x1d8880,0x1d8970):
  add('part_prefix_'+hex(entry),entry=entry,part_release=True)
  add('effect_part_prefix_'+hex(entry),entry=entry,part_release=True,effect=True)
  row=add('reference_prefix_'+hex(entry),entry=entry,effect=True);row.update(required_owner=True,unbound_reference=True,stop_boundary=0x22e1b0)
 for entry in(0x281d40,0x1d8880):
  row=add('negative_parts_'+hex(entry),entry=entry);row['user_words'] +=[(FP-U+20,0xffffffff),(SPROP-U+4,0xffffff80)]
  for n in range(3):row['user_words'] +=[(ROWS-U+n*72+0x44,0x0080ff00)]
  for count in(0,0xffffffff):
   row=add('empty_first_properties_'+hex(entry)+'_'+str(count),entry=entry);row['user_words'] +=[(FIRST-U+0x34,count)]
  row=add('inactive_'+hex(entry),entry=entry);row['user_words'] +=[(COMPONENT-U+4,0)]
  for subject_first in(True,False):
   row=add('second_empty_properties_'+hex(entry)+'_'+str(subject_first),entry=entry,subject_first=subject_first);row['user_words'] +=[(SECOND-U+0x34,0)]
   row=add('second_missing_datum_'+hex(entry)+'_'+str(subject_first),entry=entry,subject_first=subject_first);row['user_words'] +=[(SECOND-U+0x34,1)]
   row=add('second_missing_part_'+hex(entry)+'_'+str(subject_first),entry=entry,subject_first=subject_first);row['user_words'] +=[(SECOND-U+0x34,1),(SPROP-U,0x2001),(SPROP-U+4,0xbcde0002)]
 def mutation(pc,p,v):return dict(instruction=pc,address=p,written=v.to_bytes(4,'little').hex())
 row=add('current_backing_after_release',entry=0x1d8970,effect=True);row['mutations']=[mutation(0x22e201,COMPONENT+0x88,ROWS+0x400)]
 row=add('copy_backing_cached',entry=0x1d8970);row['mutations']=[mutation(0x1d89f5,COMPONENT+0x88,ROWS+0x800)]
 for name,entry,address,size,write in(
  ('event_first',0x281d40,EVENT+8,4,False),('first_count',0x281d40,FIRST+0x34,4,False),('event_second',0x281d40,EVENT+12,4,False),
  ('first_properties',0x281d40,FIRST+0x30,4,False),('property_key',0x281d40,FP,4,False),('property_value',0x281d40,FP+12,4,False),
  ('event_subject',0x281d40,EVENT+16,4,False),('component_owner',0x281d40,0x52be5c,4,False),('component_data',0x281d40,OWNER+0x44,4,False),('component_flag',0x281d40,COMPONENT+4,1,False),
  ('contact_count',0x1d8880,COMPONENT+0x8c,4,False),('point16',0x1d8880,EVENT,2,False),('contact_backing',0x1d8880,COMPONENT+0x88,4,False),('row_point',0x1d8880,ROWS,2,False),
  ('row_flags',0x1d8970,ROWS+72+0x44,1,False),('row_part',0x1d8970,ROWS+72+0x45,1,False),('row_effect',0x1d8970,ROWS+72+8,4,False),('row_publish_count',0x1d8970,COMPONENT+0x8c,4,True),('row_copy_source',0x1d8970,ROWS+144,4,False)):
  add('fault_'+name,entry=entry,fault=dict(address=address,bytes=size,occurrence=1,write=write))
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 native=('native_tu15_gameplay_contact_removal.cpp','native_tu15_constraint_reference_release.cpp','native_tu15_constraint_body_slots.cpp','native_tu15_constraint_scalar_cache.cpp');files=[*['src/physics/'+n for n in native],'include/halo2/native_tu15_gameplay_contact_removal.hpp','tools/validate_native_gameplay_contact_removal_tu15.py','tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp'];pins={f:sha(ROOT/f)for f in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],supplied_original_outcomes=False,actual_live_input_claimed=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);base.OWNERS=OWNERS;banks={0x52be5c:struct.pack('<I',OWNER),0x52d68c:struct.pack('<I',CONSTRAINT_OWNER)}
  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text();anchor='if(pc==stop)break;';assert observer.count(anchor)==1
  record='''if(pc==0x22e1b0u)allocator_calls.push_back(J{{"owner",pc},{"instance",std::uint32_t(regs.rax)},{"datum",std::uint32_t(regs.rsi)}});if(pc==0x1e0a60u){const auto arg=[&](unsigned n){const auto b=memory(child.pid,std::uint32_t(regs.rsp)+4u+4*n,4);std::uint32_t v{};for(unsigned k=0;k<4;++k)v|=std::to_integer<unsigned>(b[k])<<(8*k);return v;};allocator_calls.push_back(J{{"owner",pc},{"slot",arg(0)},{"part",arg(1)},{"component",arg(2)}});}'''
  observer=observer.replace(anchor,record+anchor);observer_path=scratch/'observer.cpp';observer_path.write_text(observer);source=inspect.getsource(base.original).replace("str(ROOT/'tools/observe_native_array_growth_tu15.cpp')",'str(observer_path)').replace('hex(STOP)]','hex(case.get("stop_boundary",STOP))]');source=source.replace(" mov ${case['receiver']},%ecx", " mov ${case.get('incoming_eax',0)},%eax\n mov ${case['receiver']},%ecx");environment=dict(base.__dict__,observer_path=observer_path);exec(source,environment);all_cases=cases();fixture,authority=environment['original'](image,banks,all_cases,scratch);assert all(c['declared_fault']for c in all_cases if 'fault'in c);save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=all_cases))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for name in native:shutil.copyfile(ROOT/'src/physics'/name,project/name)
  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text();driver='#include "halo2/native_tu15_gameplay_contact_removal.hpp"\n#include "halo2/native_tu15_constraint_reference_release.hpp"\n'+driver;begin=driver.index('      NativeTu15ArrayGrowthMemory array');end=driver.index('      const auto host_sw=',begin)
  replacement='''      NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};NativeTu15GameplayContactRemovalServices services;
      services.release_constraint_reference=[&](auto instance,auto datum,auto&state){calls.push_back(J{{"owner",0x22e1b0u},{"instance",instance},{"datum",datum}});if(c.value("unbound_reference",false))throw Required{};native_tu15_release_constraint_reference(memory,instance,datum,state,0,{},observe);};
      services.release_part_contact=[&](auto slot,auto part,auto component,auto&){calls.push_back(J{{"owner",0x1e0a60u},{"slot",slot},{"part",std::bit_cast<std::uint32_t>(part)},{"component",component}});throw Required{};};
''';driver=driver[:begin]+replacement+driver[end:];begin=driver.index('      try{const auto entry=');end=driver.index('      }catch(const Fault&)',begin);driver=driver[:begin]+'''      try{const auto entry=c.at("entry").get<std::uint32_t>();const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();
        if(entry==0x281d40u)native_tu15_notify_gameplay_contact_removed(memory,args[0],fp,services,observe);
        else if(entry==0x1d8880u)native_tu15_remove_matching_gameplay_contact(memory,c.at("incoming_eax"),c.at("receiver"),args[0],args[1],fp,services,observe);
        else native_tu15_remove_gameplay_contact_row(memory,args[0],args[1],fp,services,observe);
'''+driver[end:];driver=driver.replace('NativeTu15ArrayGrowthActivity activity;','');(project/'driver.cpp').write_text(driver);(project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(gameplay_contact_remove LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(remove driver.cpp '+' '.join(native)+')\ntarget_include_directories(remove PRIVATE include)\ntarget_compile_features(remove PRIVATE cxx_std_20)\ntarget_link_libraries(remove PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME gameplay_remove COMMAND remove "'+str(fixture/'original.json')+'")\n');results=[];commands=[]
  def run(label,compiler):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),'--target','remove','--filter','^gameplay_remove$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];commands.append(command);r=subprocess.run(command);results.append(dict(label=label,exit_code=r.returncode));return r.returncode
  for compiler in a.compilers:assert run(compiler,compiler)==0,'Native gameplay contact removal differs'
  kernel=project/native[0];text=kernel.read_text();mutants=(('wrong_point_width','m.word(event,2)','m.word(event)'),('wrong_row_stride','offset=index*72u','offset=index*64u'),('wrong_part_sign','return std::bit_cast<std::int8_t>(std::uint8_t(value));','return std::int32_t(value);'))
  for name,before,after in mutants:
   assert text.count(before)==1;kernel.write_text(text.replace(before,after));assert run('negative-'+name,a.compilers[0])!=0,'Original failed to reject '+name;kernel.write_text(text)
  executed={pc for c in all_cases for pc in c['executed_sites']};code=_Code(image,'.text');report.update(passed=all(sha(ROOT/f)==v for f,v in pins.items()),original_sha256=NEW_PIN,authority=authority,original_cases=len(all_cases),ordered_reads=sum(len(c['reads'])for c in all_cases),ordered_stores=sum(len(c['writes'])for c in all_cases),genuine_service_entries=sum(len(c['allocator_calls'])for c in all_cases),fault_prefixes=sum(c['declared_fault']for c in all_cases),declared_input_mutations=sum(c['mutations_applied']for c in all_cases),unbound_service_prefixes=sum(c.get('required_owner',False)for c in all_cases),body_coverage={hex(owner):dict(total=len(base.complete_body(code,owner).code),executed=len(set(base.complete_body(code,owner).code)&executed))for owner in OWNERS},results=results,commands=commands,negative_controls=[m[0]for m in mutants],scope='Declared valid bodies/properties/component/72B rows. Strict DATA/ordered reads/stores/preimages/banks/FP; native22E1B0 ref2 path and genuine1E0A60 entry prefixes. No source GP postimages or event padding reads. Runtime constraint removal and part-contact child remain explicit requirements; no full live claim.')
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
