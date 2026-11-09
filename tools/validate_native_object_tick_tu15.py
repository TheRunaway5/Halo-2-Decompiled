#!/usr/bin/env python3
"""Physical B8F40 parent with real early-return child and genuine boundaries."""
from pathlib import Path
import argparse,copy,hashlib,inspect,json,os,random,struct,subprocess,sys
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-object-tick-stage-20261102/staged'
BACKEND='unchanged-tu15-native-object-tick-parent-v1'
BASE,SCENE,POOL,HEADERS,BITMAP,BUSY,BODIES=0x1000000,0x1001000,0x1003000,0x1004000,0x1005000,0x1006000,0x1007000
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,r):p.write_text(json.dumps(r,indent=2)+'\n')
def metadata(image):
 source=inspect.getsource(physical.metadata).replace('(0x321d20,0x321a70,0x321180,0x321220,0x320510)','(0xb8f40,0xbe4a0)').replace('assert counts==[97,205,51,55,63],counts','assert counts==[117,125],counts')
 source=source.replace("access=CS_AC_WRITE if ins.mnemonic in('fst','fstp')else op.access;", "access=CS_AC_WRITE if ins.mnemonic in('fst','fstp')else (CS_AC_READ if ins.mnemonic=='test'and not op.access else op.access);")
 env=dict(physical.__dict__,BOUNDARIES=(0x10e630,));exec(source,env);return env['metadata'](image)
def cases():
 rows=[]
 def make(name,count=0,maximum=0,mask=0,flags=0x23,first=(1,),skip=False,stop=None):
  raw=bytearray(random.Random(len(rows)).randbytes(0x10000))
  def put(p,v,n=4):raw[p-BASE:p-BASE+n]=(v&((1<<(8*n))-1)).to_bytes(n,'little')
  put(SCENE+0x1d80,count);put(POOL+0x24,12);put(POOL+0x38,maximum);put(POOL+0x44,HEADERS);put(POOL+0x48,BITMAP)
  put(BITMAP,mask&0xffffffff);put(BITMAP+4,mask>>32)
  for n in range(40):
   put(HEADERS+n*12,0x8000+n,2);put(HEADERS+n*12+2,flags,1);put(HEADERS+n*12+8,BODIES+n*0x100);put(BODIES+n*0x100+0xc0,8 if skip else 0,1)
  for n,index in enumerate(first):put(SCENE+0x1d00+n*4,((0x8000+index)<<16)|index)
  banks=[dict(address=BASE,before=raw.hex())]+[dict(address=p,before=struct.pack('<I',v).hex())for p,v in((0x51daf0,SCENE),(0x4ecbd0,POOL),(0x4eabc4,BUSY))]
  i=len(rows);rows.append(dict(name=name,entry=0xb8f40,receiver=0,arguments=[],banks=banks,before_fp=[0x27f|((i%4)<<10),0x4520,0x1f80|((i%4)<<13)],stop_pc=stop or physical.STOP,boundary=None))
 for maximum in(0,0xffffffff,0x80000000):make('empty_'+hex(maximum),maximum=maximum)
 for count in(0,0xffffffff,1,3):
  for flags in(0,1,2,8,0x23,0x2b):make(f'first_{count:x}_{flags:x}',count=count,flags=flags,first=(1,2,3))
 for maximum,mask in((1,0),(1,1),(4,0),(4,5),(33,1<<32),(40,(1<<33)|3)):
  for flags in(0,1,2,8,0x23):make(f'bitmap_{maximum}_{mask:x}_{flags:x}',maximum=maximum,mask=mask,flags=flags)
 make('second_body_skip',maximum=4,mask=15,skip=True)
 make('update_boundary',count=1,flags=3,stop=0xbe4a0)
 make('finalize_boundary',count=1,flags=0x27)
 make('fault_seed_empty',maximum=4,mask=0)
 make('fault_seed_positive',count=2,maximum=4,mask=5,first=(1,2))
 return rows
def boundary(case,actual):
 target=actual['actual_stop']
 if case.get('fault')or 'observer_fault'in case:return None
 if target==physical.STOP:return None
 assert target in(0xbe4a0,0x10e630)
 return dict(owner=target,datum=actual['boundary_stack'][1])
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False);owned=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(owned),retained_large_artifacts=[],supplied_original_outcomes=False,production_applied=False)
 original_observer=physical.observer_source;original_boundary=physical.boundary
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image)
  physical.observer_source=lambda:original_observer().replace('timestamps=J::array();','timestamps=J::array(),executed=J::array();').replace('const auto site=sites.find(pc);','executed.push_back(pc);const auto site=sites.find(pc);').replace('{"boundary_gp",gp(regs)}','{"executed_sites",executed},{"boundary_gp",gp(regs)}')
  physical.boundary=boundary
  rows=physical.physical(image,meta,cases(),owned)
  prefixes=[];code=_Code(image,'.text')
  for seed in(c for c in rows if c['name'].startswith('fault_seed')):
   for kind in('reads','writes'):
    seen=set();counts={}
    for event in seed[kind]:
     key=event['address'],event['bytes']if kind=='reads'else len(event['written'])//2;counts[key]=counts.get(key,0)+1
     if event['instruction']in seen:continue
     seen.add(event['instruction']);case=copy.deepcopy(seed);case.update(name=seed['name']+'_'+kind+'_'+hex(event['instruction']),stop_pc=event['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=counts[key],access='read'if kind=='reads'else'write'));prefixes.append(case)
   for n,event in enumerate(seed['writes']):
    case=copy.deepcopy(seed);case.update(name=seed['name']+'_observer_'+str(n),stop_pc=event['instruction']+code.at(event['instruction']).size,observer_fault=n+1);prefixes.append(case)
  rows+=physical.physical(image,meta,prefixes,owned)
  fixture=owned/'original.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=rows))
  driver=(ROOT/'tests/native_tu15_wrapped_contact_process_tests.cpp').read_text().replace('#include "halo2/native_tu15_wrapped_contact_process.hpp"','#include "halo2/native_tu15_object_tick.hpp"').replace('unchanged-tu15-native-wrapped-contact-process-v1',BACKEND).replace('J::array({46,40,40})','J::array({117,125})')
  begin=driver.index('NativeTu15CompoundProcessServices services;');end=driver.index('   const auto cw=',begin)
  services=r'''NativeTu15ObjectTickServices services;bool boundary_hit{};
   const auto word=[&](std::uint32_t p,unsigned n=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned k=0;k<n;++k)v|=std::uint32_t(b[k])<<(8*k);return v;};
   const auto stop=[&](std::uint32_t owner,std::uint32_t datum){const auto&b=c.at("boundary");check(!b.is_null()&&b.at("owner")==owner&&b.at("datum")==datum,name+" genuine child DATA differs");boundary_hit=true;throw std::runtime_error("actual-island-owner-boundary");};
   services.update_object=[&](std::uint32_t datum,auto&){
    if(!c.at("boundary").is_null()&&c.at("boundary").at("owner")==0xbe4a0u){stop(0xbe4a0u,datum);return;}
    // Exact Source BE4A0 early-return provider: only its active-bit2/flag20
    // domain is admitted. Original executes these instructions itself.
    const auto pool=word(0x4ecbd0u),headers=word(pool+0x44u);const auto flags=word(headers+(datum&0xffffu)*12u+2u,1);
    check(!(flags&2u)||(flags&0x20u),name+" unqualified BE4A0 provider path");
   };
   services.finalize_object=[&](std::uint32_t datum,auto&){stop(0x10e630u,datum);};
'''
  driver=driver[:begin]+services+driver[end:];driver=driver.replace('NativeTu15WrappedContactProcessStats stats;','NativeTu15ObjectTickStats stats;')
  old='native_tu15_process_wrapped_contact_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{{c.at("proxy")},c.at("geometry")},fp,services,observer,&stats);';assert driver.count(old)==1;driver=driver.replace(old,'native_tu15_tick_objects(m,fp,services,observer,&stats);')
  (owned/'driver.cpp').write_text(driver);source=(STAGE/'src/physics/native_tu15_object_tick.cpp').read_text()
  mutants={'wrong_busy_value':('busy_owner+3u,1u','busy_owner+3u,0u'),'wrong_bitmap_word':('(index>>5u)*4u','(index>>4u)*4u'),'missing_late_global_read':('const auto final_owner=m.word(0x4eabc4u);','const auto final_owner=busy_owner;')}
  for name,(old,new)in mutants.items():assert source.count(old)==1;(owned/(name+'.cpp')).write_text(source.replace(old,new))
  results=[]
  for cc in('g++','clang++'):
   for name in('native',*mutants):
    binary=owned/(cc+'-'+name);cpp=STAGE/'src/physics/native_tu15_object_tick.cpp'if name=='native'else owned/(name+'.cpp');command=[cc,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(STAGE/'include'),'-I'+str(ROOT/'include'),str(owned/'driver.cpp'),str(cpp),'-o',str(binary)]
    build=subprocess.run(command,capture_output=True,text=True);(a.output/(cc+'-'+name+'-build.log')).write_text(build.stdout+build.stderr);assert build.returncode==0,build.stderr[-5000:]
    run=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True);(a.output/(cc+'-'+name+'.log')).write_text(run.stdout+run.stderr)
    assert (run.returncode==0)==(name=='native'),(cc,name,run.stdout,run.stderr)
   results.append(dict(compiler=cc,passed=True,mutants_rejected=list(mutants)))
  executed=set(pc for c in rows for pc in c['executed_sites']);parent=set(complete_body(code,0xb8f40).code)
  report.update(passed=True,original_sha256=NEW_PIN,profiles=len(rows),fault_or_observer_profiles=len(prefixes),parent_sites=117,executed_parent_sites=len(parent&executed),unexecuted_parent_sites=sorted(parent-executed),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),results=results,source_sha256={str(p.relative_to(STAGE)):sha(p)for p in STAGE.rglob('*')if p.is_file()},generated_driver_sha256=sha(owned/'driver.cpp'),scope='Declared DATA full scheduler/real BE4A0 flag20 returns and genuine child boundaries; no supplied child outcomes. Wider BE4A0/10E630/native production/caller/live closure remains open.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:
  physical.observer_source=original_observer;physical.boundary=original_boundary;report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owned.rglob('*')if p.is_file());save(a.output/'report.json',report)
if __name__=='__main__':main()
