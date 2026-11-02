#!/usr/bin/env python3
"""Instanced ray ctor/cache/code refs and nativebase lifetime, returns/FP/ordered DATA and faults."""
import inspect,json,struct
from pathlib import Path
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x2e9790,0x30aa40,0x30aa50,0x2e96a0,0x2e96b0,0x1f3600,0x283450)
BACKEND='unchanged-tu15-native-instanced-ray-lifetime-v1'
ROOT=Path(__file__).resolve().parents[1]
def metadata(image):
 support.OWNERS=OWNERS;support.BOUNDARIES=(0x2e4d60,0x2e4d80,0x2e12a0)
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[27,2,8,3,21,33,37];')
 text=text.replace('assert not op.mem.segment;', 'if op.mem.segment:continue\n    ')
 environment=dict(support.__dict__);exec(text,environment);return environment['metadata'](image)
def cases(image):
 rows=[]
 def make(entry,refs=2,code_refs=2,flags=0,alias=0,protected=False):
  banks={0x1000000:bytearray((n*37+11)&255 for n in range(0x3000)),0x48c938:bytearray(4),0x556588:bytearray(1),0x5565ac:bytearray(4)};shape=0x1000100;collection=0x80061000 if protected else 0x1000300;code=0x1000500;cache=0x1000800;allocator=0x1000a00;table=0x1000b00
  if protected:banks[collection]=bytearray(64)
  if alias==1:shape=collection
  if alias==2:shape=code
  def put(at,value,width=4):
   lo=next(lo for lo,b in banks.items()if lo<=at and at+width<=lo+len(b));banks[lo][at-lo:at-lo+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
  put(shape+4,flags,2);put(shape+12,collection);put(shape+16,code);put(collection,table);put(collection+6,refs,2);put(code+0x24,code_refs);put(allocator,table);put(table,0x2e4d80);put(table+4,0x2e4d60);put(0x48c938,allocator);put(0x556588,1 if protected else 0,1);put(0x5565ac,0x1000)
  receiver=code if entry in(0x30aa40,0x30aa50)else cache if entry==0x2e96a0 else shape;arguments=[collection,code]if entry==0x2e9790 else[]
  n=len(rows);rows.append(dict(name=f'lifetime_{n}_{entry:x}_alias{alias}',entry=entry,receiver=receiver,arguments=arguments,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|0x453f,0x1f80|((n%4)<<13)],stop_pc=base.physical.STOP,boundary=None))
 for entry in(0x2e9790,0x30aa40,0x30aa50,0x2e96a0,0x2e96b0,0x1f3600):
  for refs in(0,1,2,0xffff):
   for code_refs in(0,1,2,0xffffffff):make(entry,refs,code_refs)
 for entry in(0x2e96b0,0x1f3600):
  for flags in(0,0x7fff,0x8000,0xffff):make(entry,flags=flags)
  make(entry,refs=1,protected=True)
 for alias in(1,2):make(0x2e9790,alias=alias)
 for seed in(rows[0],rows[32],rows[64],rows[80],rows[-1]):c=dict(seed);c['name']='fault_seed_'+seed['name'];rows.append(c)
 return rows
def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.physical;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   if actual['actual_stop']==physical.STOP:return None\n   return dict(owner=actual['actual_stop'],receiver=actual['boundary_gp'][1],arguments=actual['boundary_stack'][1:3])\n"+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_instanced_ray_lifetime.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_instanced_ray_lifetime.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({27,2,8,3,21,33,37})').replace('auto services=native_tu15_contact_pressure_native_services();','NativeTu15InstancedRayLifetimeServices services;').replace('NativeTu15ContactPressureStats stats;','std::uint32_t result{};').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS);begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+NATIVE_SERVICES+driver[end:];driver=driver.replace('   check(failed==fault_case',RETURN_CHECK+'   check(failed==fault_case');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('code_ref','m.word(code+0x24u)+1u','m.word(code+0x24u)+2u'),('shape_table','shape,0x461e24u','shape,0x461e20u'),('cache_offset','cache+0x44u','cache+0x40u')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_instanced_ray_lifetime.hpp'")
 calls='''switch(c.at("entry").get<unsigned>()){
     case 0x2e9790:result=native_tu15_initialize_instanced_ray_shape(m,c.at("receiver"),c.at("arguments")[0],c.at("arguments")[1],observer);break;
     case 0x30aa40:native_tu15_reference_instanced_mopp_code(m,c.at("receiver"),observer);break;
     case 0x30aa50:native_tu15_release_instanced_mopp_code(m,c.at("receiver"),fp,services,observer);break;
     case 0x2e96a0:result=native_tu15_initialize_instanced_ray_cache(m,c.at("receiver"),observer);break;
     case 0x2e96b0:native_tu15_destroy_instanced_ray_shape(m,c.at("receiver"),fp,services,observer);break;
     case 0x1f3600:native_tu15_destroy_instanced_collection_base(m,c.at("receiver"),fp,services,observer);break;
    }'''
 check='''if(!fault_case&&!observer_failed&&!boundary_hit&&(c.at("entry")==0x2e9790||c.at("entry")==0x2e96a0))check(result==c.at("boundary_gp")[0].get<std::uint32_t>(),name+" initializedobject return differs");'''
 native_services='''   services.release_code=[&](auto allocator,auto owner,auto code,auto&){check(c.at("boundary").at("receiver")==allocator&&c.at("boundary").at("arguments")[0]==code,name+" code release formals differ");boundary(owner);};
   services.destroy_collection=[&](auto object,auto owner,auto flag,auto&){check(c.at("boundary").at("receiver")==object&&c.at("boundary").at("arguments")[0]==flag,name+" collection destructor formals differ");boundary(owner);};
   services.allocation_mode=[&](auto object,auto mode,auto&){check(c.at("boundary").at("receiver")==object&&c.at("boundary").at("arguments")[0]==mode,name+" allocation mode formals differ");boundary(0x2e12a0u);};
'''
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","shutil.copyfile(ROOT/'src/physics/native_tu15_island_activation.cpp',project/'types.cpp')")
 filter_code="rmw={site['instruction']for site in meta['sites']if any(op['read']and op['write']for op in site['operands'])};excluded=[c['name']for c in prefix if c.get('fault',{}).get('access')=='write'and c['stop_pc']in rmw];prefix=[c for c in prefix if c['name']not in excluded];rows+=physical.physical(image,meta,prefix,scratch)"
 text=text.replace('rows+=physical.physical(image,meta,prefix,scratch)',filter_code).replace('report.update(passed=','report.update(excluded_preinstruction_RMW_write_fault_cases=excluded,passed=')
 environment=dict(base.__dict__,__file__=__file__,CALLS=calls,RETURN_CHECK=check,NATIVE_SERVICES=native_services);exec(text,environment);code=environment['main']();path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());codebook=_Code(base.Xbe(base.NEW_DEFAULT),'.text');sites=set().union(*(set(complete_body(codebook,owner).code)for owner in OWNERS));report.update(physically_executed_sites=len(sites&support.parent.EXECUTED),unexecuted_sites=sorted(sites-support.parent.EXECUTED),prefetch_policy='OriginalPREFETCHT0 unchanged/nonfaulting; no semantic memory read invented. Surrounding architectural pool stride/base reads remain strict.',return_contract='Sourcector/cache return objectidentity; all voidowner volatile outputs unused',scope=__doc__,remaining=['Actual16C620 query parent/BSP/object closure and real input/currentFP composition','Production native routing/typed service composition and visible camera/strictOriginal']);base.save(path,report);return code
if __name__=='__main__':raise SystemExit(main())
