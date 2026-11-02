#!/usr/bin/env python3
"""Whole92-site18A0C0 native constructor, declared source DATA and full FP.

Every zero/redundant store, current direction reread and signed field copy is
compared against unchanged physical TU1.5. No original child/result is supplied.
"""
import argparse,copy,hashlib,inspect,json,os,random,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_camera_basis_tu15 as basis
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
BASE,INPUTS,OBJECTS,HEADERS,UNIT=0x1000000,0x1000100,0x1001000,0x1001100,0x1002000
CPP='src/physics/native_tu15_look_initialization.cpp'
HPP='include/halo2/native_tu15_look_initialization.hpp'
BACKEND='unchanged-tu15-native-look-initialization-v1'
sha=basis.sha;bits=basis.bits;save=basis.save
def metadata(image):
 source=inspect.getsource(basis.metadata).replace('assert counts==[18,62]','assert counts==[92]')
 environment=dict(basis.__dict__,OWNERS=(0x18a0c0,));exec(source,environment);return environment['metadata'](image)
def cases(image):
 rows=[]
 def make(name,index=0,unit=0xaabb0001,initialized=0,same=False,vector=(1.25,-2.5,3.75),cw=0x27f,mx=0x1f80,raw=False,alias=0):
  data=bytearray(random.Random(len(rows)).randbytes(0x5000))
  def put(p,v,n=4):data[p-BASE:p-BASE+n]=(v&((1<<(n*8))-1)).to_bytes(n,'little')
  row=INPUTS+(index if index!=0xffffffff else 0)*0x94+0x14;body=row-0x140+alias if alias else UNIT
  put(INPUTS,initialized,1);put(row,unit if same else unit^0x10000);put(OBJECTS+0x44,HEADERS);put(HEADERS+(unit&0xffff)*12+8,body)if unit!=0xffffffff else None
  for n,v in enumerate(vector):put(body+0x150+n*4,v if raw else bits(v))
  put(body+0x214,0x1234abcd);put(body+0x23d,0x80,1);put(body+0x241,0xff,1)
  banks=[dict(address=BASE,before=data.hex()),dict(address=0x4f9b54,before=struct.pack('<I',INPUTS).hex()),dict(address=0x4ecbd0,before=struct.pack('<I',OBJECTS).hex())]
  for p in(0x46a58c,0x46a588,0x44923c):banks.append(dict(address=p,before=image.read(p,4).hex()))
  rows.append(dict(name=name,entry=0x18a0c0,receiver=unit,entry_registers={'eax':index},arguments=[],index=index,unit=unit,banks=banks,before_fp=[cw,0x4520,mx],stop_pc=physical.STOP,boundary=None))
 for index in(0,1,3,0xffffffff):
  for initialized in(0,1,255):
   for same in(False,True):
    for unit in(0xffffffff,0xaabb0001):make(f'control_{index:x}_{initialized}_{same}_{unit:x}',index=index,unit=unit,initialized=initialized,same=same)
 for vi,vector in enumerate(((0.,0.,0.),(1.,0.,0.),(-1.,0.,0.),(0.,1.,0.),(0.,-1.,0.),(.25,-.5,-4.),(1e-18,-1e-18,1e-18),(1e18,-1e18,1e18))):
  for rounding in range(4):
   for precision in(0,2,3):make(f'direction_{vi}_{rounding}_{precision}',vector=vector,cw=0x7f|(precision<<8)|(rounding<<10),mx=0x1f80|(rounding<<13))
 for value in(0x80000000,1,0x7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
  for mode in(0,0x40,0x8000,0x8040):make(f'edge_{value:x}_{mode:x}',vector=(value,bits(.5),bits(-.75)),raw=True,mx=0x1f80|mode)
 for alias in(1,4,8):make(f'direction_alias_{alias}',alias=alias,index=3)
 make('fault_seed_unit');make('fault_seed_none',unit=0xffffffff);return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--production-binding',action='store_true');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False,production_applied=False,preserved_x87_depth_physically_tested=0,production_binding=a.production_binding)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);original_observer=physical.observer_source
  physical.observer_source=lambda:original_observer().replace('timestamps=J::array();','timestamps=J::array(),executed=J::array();').replace('const auto site=sites.find(pc);','executed.push_back(pc);const auto site=sites.find(pc);').replace('{"boundary_gp",gp(regs)}','{"executed_sites",executed},{"boundary_gp",gp(regs)}')
  physical.boundary=lambda c,o:None;rows=physical.physical(image,meta,cases(image),scratch);assert all(c['boundary_gp'][4]==physical.SP+4 for c in rows)
  prefixes=[];code=_Code(image,'.text')
  for seed in(c for c in rows if c['name'].startswith('fault_seed')):
   for category in('reads','writes'):
    seen=set();counts={}
    for event in seed[category]:
     key=event['address'],event['bytes']if category=='reads'else len(event['written'])//2;counts[key]=counts.get(key,0)+1
     if event['instruction']in seen:continue
     seen.add(event['instruction']);case=copy.deepcopy(seed);case.update(name=seed['name']+'_'+category+'_'+hex(event['instruction']),stop_pc=event['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=counts[key],access='read'if category=='reads'else'write'));prefixes.append(case)
   seen=set()
   for n,event in enumerate(seed['writes']):
    if event['instruction']in seen:continue
    seen.add(event['instruction']);case=copy.deepcopy(seed);case.update(name=seed['name']+'_observer_'+str(n),stop_pc=event['instruction']if event['instruction']==0x18a0fe else event['instruction']+code.at(event['instruction']).size,observer_fault=n+1)
    if event['instruction']!=0x18a0fe:prefixes.append(case)
  rows+=physical.physical(image,meta,prefixes,scratch);fixture=scratch/'original.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=rows))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');source=(ROOT/CPP).read_text();(project/'native.cpp').write_text(source)
  driver=(ROOT/'tests/native_tu15_wrapped_contact_process_tests.cpp').read_text().replace('#include "halo2/native_tu15_wrapped_contact_process.hpp"','#include "halo2/native_tu15_look_initialization.hpp"').replace('unchanged-tu15-native-wrapped-contact-process-v1',BACKEND).replace('J::array({46,40,40})','J::array({92})')
  begin=driver.index('NativeTu15CompoundProcessServices services;');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+'bool boundary_hit{};\n'+driver[end:];driver=driver.replace('NativeTu15WrappedContactProcessStats stats;','NativeTu15LookInitializationStats stats;')
  needle='native_tu15_process_wrapped_contact_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{{c.at("proxy")},c.at("geometry")},fp,services,observer,&stats);';assert driver.count(needle)==1;driver=driver.replace(needle,'native_tu15_initialize_look_controller(m,c.at("index"),c.at("unit"),fp,0,observer,&stats);')
  if a.production_binding:
   driver=driver.replace('#include "halo2/native_tu15_look_initialization.hpp"','#include "halo2/native_tu15_look_initialization_binding.hpp"').replace('NativeTu15LookInitializationStats stats;','NativeTu15LookInitializationBindingActivity activity;auto&stats=activity.initialization;').replace('native_tu15_initialize_look_controller(m,c.at("index"),c.at("unit"),fp,0,observer,&stats);','native_tu15_initialize_bound_look_controller(m,c.at("index"),c.at("unit"),{fp.control_word,fp.status,fp.mxcsr,0},true,observer,&activity);')
   driver=driver.replace('   check(failed==fault_case','   check(activity.attempts==1&&activity.completed==unsigned(!failed&&!observer_failed)&&activity.refusals==unsigned(failed||observer_failed),name+" constructor binding counters differ");\n   check(failed==fault_case')
  (project/'driver.cpp').write_text(driver)
  mutants={'missing_clear_word':('n<0x25u','n<0x24u'),'wrong_direction_sign':('asm volatile("fpatan"','asm volatile("fchs;fpatan"'),'wrong_signed_field':('record+0x2cu,first,2','record+0x2cu,first&0xffu,2')}
  for name,(before,after)in mutants.items():assert source.count(before)==1;(project/(name+'.cpp')).write_text(source.replace(before,after))
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(w in r.stderr for w in("differ","differs")),(r.returncode,r.stdout,r.stderr)\nprint(r.stderr.strip())\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(initialization LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  for name in('native',*mutants):
   cmake+=f'add_executable(init_{name} {name}.cpp $<TARGET_OBJECTS:driver>)\ntarget_include_directories(init_{name} PRIVATE include)\ntarget_compile_features(init_{name} PRIVATE cxx_std_20)\ntarget_compile_options(init_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME init_{name} COMMAND {command}$<TARGET_FILE:init_{name}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target','init_'+n]for n in('native',*mutants)),[]),'--filter','^init_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native look initialization differs'
  executed=set(pc for c in rows for pc in c['executed_sites']);assert len(executed)==92,len(executed)
  files=(ROOT/CPP,ROOT/HPP,ROOT/'include/halo2/native_tu15_look_initialization_binding.hpp',Path(__file__));report.update(passed=True,original_sha256=NEW_PIN,body_sites=meta['body_sites'],physically_executed_sites=len(executed),profiles=len(rows),fault_or_observer_profiles=len(prefixes),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),results=results,negative_controls=list(mutants),source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},listing_sha256=meta['listing_sha256'],scope=__doc__)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
