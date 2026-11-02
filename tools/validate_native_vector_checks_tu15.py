#!/usr/bin/env python3
"""Native B1720/AA200/AA270 against unchanged original96-site math family.

Source scalar argument storage is private to the ordinary reference caller;
native outputs are booleans and FP, with no saved-control/register postimages.
"""
import argparse,copy,inspect,os,random,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_camera_basis_tu15 as basis
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
BASE,FIRST,SECOND=0x1000000,0x1000100,0x1000200
CPP='src/physics/native_tu15_vector_checks.cpp';HPP='include/halo2/native_tu15_vector_checks.hpp'
BACKEND='unchanged-tu15-native-vector-checks-v1'
sha=basis.sha;save=basis.save;bits=basis.bits
def metadata(image):
 source=inspect.getsource(basis.metadata).replace('assert counts==[18,62]','assert counts==[46,30,20]');environment=dict(basis.__dict__,OWNERS=(0xb1720,0xaa200,0xaa270));exec(source,environment);return environment['metadata'](image)
def cases(image):
 rows=[]
 def make(name,kind='orthogonal',a=(1.,0.,0.),b=(0.,1.,0.),cw=0x27f,mx=0x1f80,raw=False,scalar_first=0,scalar_second=0,alias=False):
  raw_bank=bytearray(random.Random(len(rows)).randbytes(0x400));second=FIRST if alias else SECOND
  for p,v in((FIRST,a),(second,b)):
   for k,n in enumerate(v):struct.pack_into('<I',raw_bank,p-BASE+k*4,n if raw else bits(n))
  banks=[dict(address=BASE,before=raw_bank.hex())]+[dict(address=p,before=image.read(p,4).hex())for p in(0x46a0d0,0x46a180)]
  rows.append(dict(name=name,kind=kind,entry={'orthogonal':0xb1720,'unit':0xaa200,'scalar':0xaa270}[kind],receiver=0,entry_registers={'eax':FIRST,'esi':FIRST,'edi':second},arguments=[scalar_second]if kind=='scalar'else[],first=FIRST,second=second,first_bits=scalar_first,second_bits=scalar_second,banks=banks,before_fp=[cw,0x4520,mx],stop_pc=physical.STOP,boundary=None))
 vectors=((1.,0.,0.),(0.,1.,0.),(0.,0.,1.),(-1.,0.,0.),(0.,0.,0.),(2.,0.,0.),(.6,.8,0.),(.8,-.6,0.),(.70710677,.70710677,0.),(1.00001,0.,0.))
 for i,a in enumerate(vectors):
  for j,b in enumerate(vectors):make(f'orthogonal_{i}_{j}',a=a,b=b)
 for i,a in enumerate(vectors):
  for rounding in range(4):
   for precision in(0,2,3):make(f'unit_{i}_{rounding}_{precision}',kind='unit',a=a,cw=0x7f|(precision<<8)|(rounding<<10),mx=0x1f80|(rounding<<13))
 for value in(0,0x80000000,1,0x7fffff,bits(.0001),bits(-.0001),bits(1.),0x7f800000,0xff800000,0x7fc12345,0x7f812345):
  for mode in(0,0x40,0x8000,0x8040):
   make(f'scalar_{value:x}_{mode:x}',kind='scalar',scalar_first=value,mx=0x1f80|mode)
   make(f'vector_edge_{value:x}_{mode:x}',a=(value,bits(.8),0),b=(bits(0.),bits(0.),bits(1.)),raw=True,mx=0x1f80|mode)
 make('alias',alias=True);make('fault_seed_orthogonal');make('fault_seed_unit',kind='unit');make('fault_seed_scalar',kind='scalar');return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False,preserved_x87_depth_physically_tested=0,source_private_control_ledger_claimed=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);original_observer=physical.observer_source
  physical.observer_source=lambda:original_observer().replace('timestamps=J::array();','timestamps=J::array(),executed=J::array();').replace('const auto site=sites.find(pc);','executed.push_back(pc);const auto site=sites.find(pc);').replace('{"boundary_gp",gp(regs)}','{"executed_sites",executed},{"boundary_gp",gp(regs)}');physical.boundary=lambda c,o:None
  caller=inspect.getsource(physical.physical).splitlines(keepends=True);found=False
  for n,line in enumerate(caller):
   if line.startswith('  entry_setup='):
    caller.insert(n+1,"  if c['kind']=='scalar':entry_setup+='\\n mov $'+str(c['first_bits'])+',%eax\\n movd %eax,%xmm0'\n");found=True;break
  assert found;environment=dict(physical.__dict__);exec(''.join(caller),environment);run=environment['physical'];rows=run(image,meta,cases(image),scratch)
  for c in rows:assert c['boundary_gp'][4]==physical.SP+(8 if c['kind']=='scalar'else 4)
  prefixes=[]
  for seed in(c for c in rows if c['name'].startswith('fault_seed')):
   seen=set();counts={}
   for event in seed['reads']:
    key=event['address'],event['bytes'];counts[key]=counts.get(key,0)+1
    if event['instruction']in seen:continue
    seen.add(event['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_read_'+hex(event['instruction']),stop_pc=event['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=counts[key],access='read'));prefixes.append(c)
  rows+=run(image,meta,prefixes,scratch);fixture=scratch/'original.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=rows))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');source=(ROOT/CPP).read_text();(project/'native.cpp').write_text(source)
  driver=(ROOT/'tests/native_tu15_wrapped_contact_process_tests.cpp').read_text().replace('#include "halo2/native_tu15_wrapped_contact_process.hpp"','#include "halo2/native_tu15_vector_checks.hpp"').replace('unchanged-tu15-native-wrapped-contact-process-v1',BACKEND).replace('J::array({46,40,40})','J::array({46,30,20})');begin=driver.index('NativeTu15CompoundProcessServices services;');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+'bool boundary_hit{};std::uint32_t result{};\n'+driver[end:];driver=driver.replace('NativeTu15WrappedContactProcessStats stats;','')
  needle='native_tu15_process_wrapped_contact_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{{c.at("proxy")},c.at("geometry")},fp,services,observer,&stats);';assert driver.count(needle)==1;driver=driver.replace(needle,'if(c.at("kind")=="unit")result=native_tu15_vector_near_unit(m,c.at("first"),fp,0);else if(c.at("kind")=="scalar")result=native_tu15_scalar_near(m,c.at("first_bits"),c.at("second_bits"),fp,0);else result=native_tu15_vectors_orthonormal(m,c.at("first"),c.at("second"),fp,0);');driver=driver.replace('   check(failed==fault_case','   if(!fault_case)check(result==c.at("boundary_gp")[0],name+" vector boolean return differs");\n   check(failed==fault_case');(project/'driver.cpp').write_text(driver)
  mutants={'wrong_norm_order':('norm=add(norm,zz)','norm=sub(norm,zz)'),'wrong_tolerance':('word(m,0x46a180u)','word(m,0x46a0d0u)'),'wrong_dot':('dot=add(dot,mul(first_x,second_x))','dot=sub(dot,mul(first_x,second_x))')}
  for name,(before,after)in mutants.items():assert source.count(before)==1;(project/(name+'.cpp')).write_text(source.replace(before,after))
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "differ" in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint(r.stderr.strip())\n');cmake='cmake_minimum_required(VERSION 3.20)\nproject(checks LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  for name in('native',*mutants):
   cmake+=f'add_executable(check_{name} {name}.cpp $<TARGET_OBJECTS:driver>)\ntarget_include_directories(check_{name} PRIVATE include)\ntarget_compile_features(check_{name} PRIVATE cxx_std_20)\ntarget_compile_options(check_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME check_{name} COMMAND {command}$<TARGET_FILE:check_{name}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target','check_'+n]for n in('native',*mutants)),[]),'--filter','^check_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Native vector checks differ'
  executed=set(pc for c in rows for pc in c['executed_sites']);assert len(executed)==96,len(executed);files=(ROOT/CPP,ROOT/HPP,Path(__file__));report.update(passed=True,original_sha256=NEW_PIN,body_sites=meta['body_sites'],physically_executed_sites=len(executed),profiles=len(rows),read_fault_prefixes=len(prefixes),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),results=results,negative_controls=list(mutants),source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},listing_sha256=meta['listing_sha256'],scope=__doc__)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
