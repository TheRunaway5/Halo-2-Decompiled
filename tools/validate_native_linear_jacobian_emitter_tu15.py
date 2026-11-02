#!/usr/bin/env python3
"""Pure native3294E0 against unchanged TU1.5, including alias/fault/FP phases."""
import argparse,copy,inspect,os,random,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_camera_basis_tu15 as base
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
BASE,FIRST,SECOND,INPUT,ACCUM_A,ACCUM_B,PACKET,OUTPUT,SCHEMAS=0x1000000,0x1000100,0x1000200,0x1000300,0x1000400,0x1000500,0x1000600,0x1000800,0x1000a00
CPP='src/physics/native_tu15_linear_jacobian_emitter.cpp';HPP='include/halo2/native_tu15_linear_jacobian_emitter.hpp';BACKEND='unchanged-tu15-native-linear-jacobian-emitter-v1';sha=base.sha;save=base.save;bits=base.bits

def metadata(image):
 source=inspect.getsource(base.metadata).replace('assert counts==[18,62]','assert counts==[155]');env=dict(base.__dict__,OWNERS=(0x3294e0,));exec(source,env);return env['metadata'](image)
def cases(image):
 out=[]
 def make(name,first_local=0,second_local=0,scale=.25,cw=0x27f,mx=0x1f80,first=FIRST,second=SECOND,output=OUTPUT,schemas=SCHEMAS,packet=PACKET,edge=None):
  raw=bytearray(random.Random(len(out)).randbytes(0x2000))
  def put(p,n,width=4):raw[p-BASE:p-BASE+width]=(n&((1<<(width*8))-1)).to_bytes(width,'little')
  def vector(p,v):
   for k,n in enumerate(v):put(p+k*4,bits(n))
  vector(FIRST,(2.,3.,-1.,.2));vector(SECOND,(-4.,2.,5.,-.8))
  for accumulator,local,center,mass in((ACCUM_A,first_local,(1.,2.,3.,4.),.5),(ACCUM_B,second_local,(5.,6.,7.,8.),1.)):
   put(accumulator+0xc,local,1);put(accumulator+0x3c,bits(mass));vector(accumulator+0x30,(.2,.3,.4,mass));vector(accumulator+0x40,center)
   for off,v in((0x50,(1.1,.2,.3,.4)),(0x60,(-.5,1.2,.6,.7)),(0x70,(.8,-.9,1.3,.1))):vector(accumulator+off,v)
  put(INPUT,ACCUM_A);put(INPUT+4,ACCUM_B);put(INPUT+0xc,bits(scale));put(packet,output);put(packet+4,schemas)
  if edge is not None:put(FIRST,edge)
  banks=[dict(address=BASE,before=raw.hex())]+[dict(address=p,before=image.read(p,n).hex())for p,n in((0x46e400,48),(0x41f644,4),(0x46a0d0,4))]
  out.append(dict(name=name,entry=0x3294e0,receiver=0,arguments=[first,second,INPUT,packet],first=first,second=second,input=INPUT,packet=packet,banks=banks,before_fp=[cw,0x4520,mx],stop_pc=physical.STOP,boundary=None))
 for a in(0,1):
  for b in(0,1):
   for rounding in range(4):
    for precision in(0,2,3):make(f'ordinary_{a}_{b}_{rounding}_{precision}',first_local=a,second_local=b,cw=0x7f|(precision<<8)|(rounding<<10),mx=0x1f80|(rounding<<13))
 for value in(0,0x80000000,1,0x7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
  for mode in(0,0x40,0x8000,0x8040):make(f'edge_{value:x}_{mode:x}',edge=value,mx=0x1f80|mode)
 for first,second,output,schemas,packet in((FIRST,FIRST,OUTPUT,SCHEMAS,PACKET),(OUTPUT,SECOND,OUTPUT,SCHEMAS,PACKET),(FIRST,OUTPUT+16,OUTPUT,SCHEMAS,PACKET),(FIRST,SECOND,ACCUM_A+0x30,SCHEMAS,PACKET),(FIRST,SECOND,INPUT,SCHEMAS,PACKET),(FIRST,SECOND,PACKET,SCHEMAS,PACKET),(FIRST,SECOND,OUTPUT,OUTPUT+16,PACKET)):
  make(f'alias_{first:x}_{second:x}_{output:x}_{schemas:x}_{packet:x}',first=first,second=second,output=output,schemas=schemas,packet=packet)
 for scale in(0.,-0.,-1.,1e-12,1e12):make(f'scale_{scale}',scale=scale)
 make('fault_seed_rotated');make('fault_seed_local',first_local=1,second_local=1);return out

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);owned=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(owned),retained_large_artifacts=[],native_cpu_callbacks=False,supplied_original_outcomes=False,preserved_x87_depth_physically_tested=0)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);original=physical.observer_source;physical.observer_source=lambda:original().replace('timestamps=J::array();','timestamps=J::array(),executed=J::array();').replace('const auto site=sites.find(pc);','executed.push_back(pc);const auto site=sites.find(pc);').replace('{"boundary_gp",gp(regs)}','{"executed_sites",executed},{"boundary_gp",gp(regs)}');physical.boundary=lambda c,o:None
  old_observer=physical.observer_source
  physical.observer_source=lambda:old_observer().replace('unsigned count=0;','unsigned count=0,stop_visits=0;').replace('if(pc==stop||','if((pc==stop&&++stop_visits==m.value("stop_occurrence",1u))||')
  physical_source=inspect.getsource(physical.physical).replace("specific=dict(m);specific['regions']=","specific=dict(m);specific['stop_occurrence']=c.get('stop_occurrence',1);specific['regions']=")
  namespace=dict(physical.__dict__);exec(physical_source,namespace);execute=namespace['physical'];rows=execute(image,meta,cases(image),owned);assert all(c['boundary_gp'][4]==physical.SP+4 for c in rows);code=_Code(image,'.text');prefixes=[]
  for seed in(c for c in rows if c['name'].startswith('fault_seed')):
   for category in('reads','writes'):
    counts={};seen=set()
    for e in seed[category]:
     key=e['address'],e['bytes']if category=='reads'else len(e['written'])//2;counts[key]=counts.get(key,0)+1
     if e['instruction']in seen:continue
     seen.add(e['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_'+category+'_'+hex(e['instruction']),stop_pc=e['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=counts[key],access='read'if category=='reads'else'write'));prefixes.append(c)
   observer_occurrences={}
   for n,e in enumerate(seed['writes']):
    observer_occurrences[e['instruction']]=observer_occurrences.get(e['instruction'],0)+1
    c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+str(n),stop_pc=e['instruction']+code.at(e['instruction']).size,observer_fault=n+1,stop_occurrence=observer_occurrences[e['instruction']]);prefixes.append(c)
  rows+=execute(image,meta,prefixes,owned);fixture=owned/'fixture.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=[155],cases=rows));project=owned/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');source=(ROOT/CPP).read_text();(project/'native.cpp').write_text(source)
  driver=(ROOT/'tests/native_tu15_wrapped_contact_process_tests.cpp').read_text().replace('#include "halo2/native_tu15_wrapped_contact_process.hpp"','#include "halo2/native_tu15_linear_jacobian_emitter.hpp"').replace('unchanged-tu15-native-wrapped-contact-process-v1',BACKEND).replace('J::array({46,40,40})','J::array({155})');begin=driver.index('NativeTu15CompoundProcessServices services;');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+'bool boundary_hit{};\n'+driver[end:];driver=driver.replace('NativeTu15WrappedContactProcessStats stats;','NativeTu15LinearJacobianEmitterStats stats;');needle='native_tu15_process_wrapped_contact_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{{c.at("proxy")},c.at("geometry")},fp,services,observer,&stats);';assert driver.count(needle)==1;driver=driver.replace(needle,'native_tu15_emit_world_linear_jacobians(m,c.at("first"),c.at("second"),c.at("input"),c.at("packet"),fp,0,observer,&stats);');(project/'driver.cpp').write_text(driver)
  mutants={'wrong_second_cross':('angular_b=sub(positive_b,negative_b)','angular_b=sub(negative_b,positive_b)'),'wrong_reciprocal':('output-0x14u,true','output-0x10u,true'),'wrong_schema':('0x3040du','0x3040eu')}
  for n,(before,after)in mutants.items():assert source.count(before)==1;(project/(n+'.cpp')).write_text(source.replace(before,after))
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "differ"in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint(r.stderr.strip())\n');results=[]
  # One tiny standalone compiler target per variant; no game/World CMake.
  for cc in('g++','clang++'):
   for n in('native',*mutants):
    target=project/(cc.replace('+','x')+'-'+n);cmd=[cc,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(project/'include'),str(project/'driver.cpp'),str(project/(n+'.cpp')),'-o',str(target)];r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(cc+'-'+n+'-build.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-3000:];command=[str(target),str(fixture)]if n=='native'else[sys.executable,str(project/'negative.py'),str(target),str(fixture)];r=subprocess.run(command,capture_output=True,text=True);(a.output/(cc+'-'+n+'-run.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-4000:]
   results.append(dict(compiler=cc,passed=True))
  executed={pc for c in rows for pc in c['executed_sites']};assert len(executed)==155;report.update(passed=True,profiles=len(rows),full_return_profiles=len(rows)-len(prefixes),fault_observer_profiles=len(prefixes),body_sites=155,executed_sites=155,ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),compiler_results=results,negative_controls=list(mutants),source_hashes={n:sha(ROOT/n)for n in(CPP,HPP,'tests/native_tu15_wrapped_contact_process_tests.cpp','tools/validate_native_linear_jacobian_emitter_tu15.py')},listing_sha256=meta['listing_sha256'],scope=__doc__)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owned.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
