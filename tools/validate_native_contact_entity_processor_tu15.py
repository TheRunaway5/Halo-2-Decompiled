#!/usr/bin/env python3
"""Unchanged TU1.5 entity contact matching; genuine update entry boundaries."""
import argparse,copy,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_contact_entity_notification_tu15 as notify
import validate_native_island_activation_tu15 as activation
import validate_native_geometry_tu15 as ref
ROOT=ref.ROOT
BASE,EVENT,INDIRECT,DESCRIPTOR,CONTACTS,COMPONENT,ROWS,FIRST,SECOND=tuple(0x1000000+n for n in(0,0x100,0x200,0x300,0x400,0x800,0x1000,0x1800,0x1900))
def cases(image):
 rows=[]
 def make(contacts=2,count=3,match=1,material=True,other=True,own_slot=3,other_slot=7,kind=1,body=0x1001a00,subject=0,seed=False):
  raw=bytearray(0x4000)
  def put(p,v,n=4):raw[p-BASE:p-BASE+n]=(v&((1<<(n*8))-1)).to_bytes(n,'little')
  put(EVENT,0x12345678);put(EVENT+4,FIRST);put(EVENT+8,SECOND);put(EVENT+12,INDIRECT);put(EVENT+16,subject)
  put(INDIRECT,DESCRIPTOR);put(DESCRIPTOR,CONTACTS);put(DESCRIPTOR+4,contacts);put(COMPONENT+0x88,ROWS);put(COMPONENT+0x8c,count)
  put(FIRST,0x11112222);put(SECOND,0x33334444);put(FIRST+0x18,kind);put(FIRST+0x20,body)
  for k in range(3):put(CONTACTS+k*0x30+0x20,0x2345+k,2)
  for k in range(4):
   row=ROWS+k*0x48;put(row,0x2345 if material and k==match else 0x9876,2);put(row+0x14,0xeeee0002 if other else 0xeeee0003)
   put(row+0x45,own_slot,1);put(row+0x46,other_slot,1)
  n=len(rows);gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=ref.SP;gp[6]=EVENT
  c=dict(id=n,name=('fault_seed_'if seed else'processor_')+str(n),entry=0x1d8a20,world=0,island=0,body=0,event=EVENT,component=COMPONENT,own=0xdddd0001,other=0xeeee0002,own_slot=own_slot,other_slot=other_slot,arguments=[COMPONENT,0xdddd0001,0xeeee0002,own_slot,other_slot],gp=gp,banks=[dict(address=BASE,before=raw.hex())],before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|((n%4)<<8)|(n*7%64),0x1f80|((n%4)<<13)],live_x87=[],alignment_fault=False)
  if contacts>0 and count>match>=0 and material and other and kind==1 and body:c['required_owner']=dict(owner=0x1d77d0,argument_count=4)
  rows.append(c)
 for contacts in(-1,0,1,2,3):
  for count in(-1,0,1,3):
   for match in(-1,0,2):make(contacts=contacts,count=count,match=match)
 for material in(False,True):
  for other in(False,True):
   for own_slot,other_slot in((3,7),(0xffffffff,0xffffff80),(0,0),(127,127)):
    for kind,body,subject in((0,0x1001a00,0),(1,0,0),(1,0x1001a00,0),(1,0x1001a00,0x1001a00)):
     make(material=material,other=other,own_slot=own_slot,other_slot=other_slot,kind=kind,body=body,subject=subject)
 make(seed=True)
 return rows
def driver():
 s=activation.DRIVER.replace('J::array({45,57,18,98,37,1})','J::array({84})');start=s.index('NativeTu15IslandActivationStats stats;');end=s.index('   Host host;',start)
 s=s[:start]+'''NativeTu15ContactEntityProcessorStats stats;NativeTu15ContactEntityProcessorServices services;J required=nullptr;
   services.update_contact=[&](auto manager,auto row,bool reversed,auto own,auto slot,auto&){required=J{{"owner",0x1d77d0},{"manager",manager},{"receiver",row},{"arguments",J::array({0,reversed?1:0,own,slot})}};throw OwnerStop{};};
'''+s[end:]
 start=s.index('switch(entry){');end=s.index('   }catch(const Prefix&)',start)
 s=s[:start]+'''native_tu15_process_entity_contacts(m,{test.at("event")},test.at("component"),test.at("own"),test.at("other"),test.at("own_slot"),test.at("other_slot"),fp,services,&stats);
'''+s[end:]
 old='required.at("arguments")==boundary.at("arguments")';new='required.at("arguments")[0]==boundary.at("arguments")[0]&&required.at("arguments")[1]==(boundary.at("arguments")[1].get<unsigned>()&255u)&&required.at("arguments")[2]==boundary.at("arguments")[2]&&required.at("arguments")[3]==boundary.at("arguments")[3]&&required.at("manager")==boundary.at("manager")';assert s.count(old)==1;s=s.replace(old,new)
 start=s.index('   if(test.at("entry")==0x283450');end=s.index('\n',start);s=s[:start]+s[end:]
 return s
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 source=ROOT/'src/physics/native_tu15_contact_entity_processor.cpp';files=[source,ROOT/'include/halo2/native_tu15_contact_entity_processor.hpp',Path(__file__)];pins={str(p.relative_to(ROOT)):ref.sha(p)for p in files};report=dict(passed=False,source_hashes=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert ref.sha(ref.NEW_DEFAULT)==ref.NEW_PIN;image=ref.Xbe(ref.NEW_DEFAULT);old=notify.ENTRIES;notify.ENTRIES={0x1d8a20:84}
  try:meta=notify.metadata(image)
  finally:notify.ENTRIES=old
  assert all(o['read']or o['write']for s in meta['sites']for o in s['operands']);owned=activation.prepare(image,meta,scratch)
  observer=owned/'observer.cpp';s=observer.read_text();needle='{"receiver",std::uint32_t(regs.rcx)},{"arguments",arguments}';assert s.count(needle)==1;observer.write_text(s.replace(needle,'{"receiver",std::uint32_t(regs.rcx)},{"manager",std::uint32_t(regs.rax)},{"arguments",arguments}'))
  subprocess.run(['c++','-std=c++20','-O2',str(observer),'-o',str(owned/'observer')],check=True,capture_output=True)
  rows=ref.physical(owned,cases(image),'complete');faults=[]
  for seed in[c for c in rows if c['name'].startswith('fault_seed')]:
   for n,event in enumerate(seed['reads']):
    c=copy.deepcopy(seed);c.update(id=len(rows)+len(faults),name=seed['name']+'_read_'+str(n),fault=dict(phase='before_read',ordinal=n));faults.append(c)
  rows+=ref.physical(owned,faults,'prefix');fixture=scratch/'processor.json';fixture.write_text(json.dumps(dict(image_sha256=ref.NEW_PIN,body_sites=meta['body_sites'],cases=rows),separators=(',',':')))
  project=scratch/'project';project.mkdir();prefix=(ROOT/'tests/native_tu15_shape_bounds_tests.cpp').read_text().split('int main(int argc,char**argv)')[0];(project/'driver.cpp').write_text('#include "halo2/native_tu15_contact_entity_processor.hpp"\n'+prefix+driver());shutil.copyfile(source,project/'native.cpp');text=source.read_text();variants={}
  for name,needle,replacement in(('wrong_material','word(m,row,2)==word(m,contact+0x20u,2)','word(m,row,2)!=word(m,contact+0x20u,2)'),('wrong_side','body!=word(m,event.identity+0x10u)','body==word(m,event.identity+0x10u)'),('wrong_stride','row_offset+=0x48u','row_offset+=0x40u')):
   assert text.count(needle)==1,(name,text.count(needle));variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.returncode,r.stdout,r.stderr)\nprint("Original rejects:",r.stderr[-500:])\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeEntityProcessor LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\n'+f'target_include_directories(driver PRIVATE "{ROOT}/include")\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
  for name in('native',*variants):
   cmake+=f'add_executable(processor_{name} $<TARGET_OBJECTS:driver> {name}.cpp)\ntarget_include_directories(processor_{name} PRIVATE "{ROOT}/include")\ntarget_compile_features(processor_{name} PRIVATE cxx_std_20)\n'
   command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:processor_{name}> "{fixture}"';cmake+=f'add_test(NAME processor_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','processor_native',*sum((['--target','processor_'+n]for n in variants),[]),'--filter','^processor_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(ref.sha(ROOT/f)==v for f,v in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),genuine_update_entry_stops=sum(bool(c.get('required_owner'))and not c.get('fault')for c in rows),ordered_original_reads=sum(len(c['reads'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],negative_controls=list(variants),source_snapshot_unchanged=unchanged,image_sha256=ref.NEW_PIN,instruction_patches=[],supplied_original_outcomes=False,remaining=['Genuine1D77D0 native conversion','Full child return composition','Live binding','Callback-mutable counts/banks'])
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
