#!/usr/bin/env python3
"""Whole88-site E05D0 native camera basis, including alias and fault phases."""
import argparse,copy,inspect,os,random,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_camera_basis_tu15 as base
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
BASE,DIRECTION,FALLBACK,FIRST,SECOND,DEFAULT=0x1000000,0x1000100,0x1000200,0x1000300,0x1000400,0x1000500
CPP='src/physics/native_tu15_camera_horizontal_basis.cpp';HPP='include/halo2/native_tu15_camera_horizontal_basis.hpp';BACKEND='unchanged-tu15-native-horizontal-camera-basis-v1'
sha=base.sha;save=base.save;bits=base.bits
def metadata(image):
 source=inspect.getsource(base.metadata).replace('assert counts==[18,62]','assert counts==[88]');environment=dict(base.__dict__,OWNERS=(0xe05d0,));exec(source,environment);return environment['metadata'](image)
def cases(image):
 rows=[]
 def make(name,vector=(1.25,-2.5,3.75),fallback=(.6,.8),cw=0x27f,mx=0x1f80,raw=False,first=FIRST,second=SECOND):
  bank=bytearray(random.Random(len(rows)).randbytes(0x600))
  for k,n in enumerate(vector):struct.pack_into('<I',bank,DIRECTION-BASE+k*4,n if raw else bits(n))
  for p,v in((FALLBACK,fallback),(DEFAULT,(.25,-.75))):
   for k,n in enumerate(v):struct.pack_into('<I',bank,p-BASE+k*4,bits(n))
  banks=[dict(address=BASE,before=bank.hex()),dict(address=0x474cd8,before=struct.pack('<I',DEFAULT).hex())]+[dict(address=p,before=image.read(p,4).hex())for p in(0x46a0d0,0x46a0ec)]
  rows.append(dict(name=name,entry=0xe05d0,receiver=first,entry_registers={'edx':DIRECTION,'esi':FALLBACK,'edi':second},arguments=[],direction=DIRECTION,fallback=FALLBACK,first=first,second=second,banks=banks,before_fp=[cw,0x4520,mx],stop_pc=physical.STOP,boundary=None))
 for vi,v in enumerate(((1.25,-2.5,3.75),(0.,0.,0.),(0.,0.,1.),(0.,0.,-1.),(1e-12,0.,.5),(1e18,-1e18,0.))):
  for rounding in range(4):
   for precision in(0,2,3):make(f'basis_{vi}_{rounding}_{precision}',vector=v,cw=0x7f|(precision<<8)|(rounding<<10),mx=0x1f80|(rounding<<13))
 for fallback in((0.,0.),(1e-12,-1e-12),(.6,.8),(-1.,0.)):
  for z in(-1.,0.,1.):make(f'fallback_{fallback}_{z}',vector=(0.,0.,z),fallback=fallback)
 for value in(0x80000000,1,0x7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
  for mode in(0,0x40,0x8000,0x8040):make(f'edge_{value:x}_{mode:x}',vector=(value,bits(.5),bits(-.75)),raw=True,mx=0x1f80|mode)
 for first,second in((DIRECTION,SECOND),(DIRECTION-4,SECOND),(FALLBACK,SECOND),(FIRST,FIRST),(FIRST,FIRST+4),(DIRECTION,DIRECTION+4)):
  make(f'alias_{first:x}_{second:x}',first=first,second=second)
 make('fault_seed_normal');make('fault_seed_fallback',vector=(0.,0.,1.));make('fault_seed_default',vector=(0.,0.,-1.),fallback=(0.,0.));return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False,preserved_x87_depth_physically_tested=0)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);original_observer=physical.observer_source
  physical.observer_source=lambda:original_observer().replace('timestamps=J::array();','timestamps=J::array(),executed=J::array();').replace('const auto site=sites.find(pc);','executed.push_back(pc);const auto site=sites.find(pc);').replace('{"boundary_gp",gp(regs)}','{"executed_sites",executed},{"boundary_gp",gp(regs)}');physical.boundary=lambda c,o:None;rows=physical.physical(image,meta,cases(image),scratch);assert all(c['boundary_gp'][4]==physical.SP+4 for c in rows);prefixes=[];code=_Code(image,'.text')
  for seed in(c for c in rows if c['name'].startswith('fault_seed')):
   for category in('reads','writes'):
    seen=set();counts={}
    for event in seed[category]:
     key=event['address'],event['bytes']if category=='reads'else len(event['written'])//2;counts[key]=counts.get(key,0)+1
     if event['instruction']in seen:continue
     seen.add(event['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_'+category+'_'+hex(event['instruction']),stop_pc=event['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=counts[key],access='read'if category=='reads'else'write'));prefixes.append(c)
   for n,event in enumerate(seed['writes']):
    c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+str(n),stop_pc=event['instruction']+code.at(event['instruction']).size,observer_fault=n+1);prefixes.append(c)
  rows+=physical.physical(image,meta,prefixes,scratch);fixture=scratch/'original.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');source=(ROOT/CPP).read_text();(project/'native.cpp').write_text(source)
  driver=(ROOT/'tests/native_tu15_wrapped_contact_process_tests.cpp').read_text().replace('#include "halo2/native_tu15_wrapped_contact_process.hpp"','#include "halo2/native_tu15_camera_horizontal_basis.hpp"').replace('unchanged-tu15-native-wrapped-contact-process-v1',BACKEND).replace('J::array({46,40,40})','J::array({88})');begin=driver.index('NativeTu15CompoundProcessServices services;');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+'bool boundary_hit{};\n'+driver[end:];driver=driver.replace('NativeTu15WrappedContactProcessStats stats;','')
  needle='native_tu15_process_wrapped_contact_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{{c.at("proxy")},c.at("geometry")},fp,services,observer,&stats);';assert driver.count(needle)==1;driver=driver.replace(needle,'native_tu15_camera_horizontal_basis(m,c.at("direction"),c.at("fallback"),c.at("first"),c.at("second"),fp,0,observer);');(project/'driver.cpp').write_text(driver)
  mutants={'wrong_perpendicular':('perpendicular,bits(sub(zero,scalar(read(m,first+4u))))','perpendicular,bits(sub(scalar(read(m,first+4u)),zero))'),'wrong_fallback_sign':('above(scalar(read(m,direction+8u)),zero)','above(zero,scalar(read(m,direction+8u)))'),'wrong_default':('read(m,basis+4u)','read(m,basis)')}
  for name,(before,after)in mutants.items():assert source.count(before)==1;(project/(name+'.cpp')).write_text(source.replace(before,after))
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "differ" in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint(r.stderr.strip())\n');cmake='cmake_minimum_required(VERSION 3.20)\nproject(basis LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  for name in('native',*mutants):
   cmake+=f'add_executable(basis_{name} {name}.cpp $<TARGET_OBJECTS:driver>)\ntarget_include_directories(basis_{name} PRIVATE include)\ntarget_compile_features(basis_{name} PRIVATE cxx_std_20)\ntarget_compile_options(basis_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME basis_{name} COMMAND {command}$<TARGET_FILE:basis_{name}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target','basis_'+n]for n in('native',*mutants)),[]),'--filter','^basis_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native horizontal camera basis differs'
  executed=set(pc for c in rows for pc in c['executed_sites']);assert len(executed)==88,len(executed);files=(ROOT/CPP,ROOT/HPP,Path(__file__));report.update(passed=True,original_sha256=NEW_PIN,body_sites=meta['body_sites'],physically_executed_sites=len(executed),profiles=len(rows),fault_or_observer_profiles=len(prefixes),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),results=results,negative_controls=list(mutants),source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},listing_sha256=meta['listing_sha256'],scope=__doc__)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
