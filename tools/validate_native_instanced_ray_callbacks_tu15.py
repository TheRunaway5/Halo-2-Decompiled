#!/usr/bin/env python3
"""Native instanced provider/forwarder/collector, genuine16C000 prefix.

No child hit or arithmetic outcome is supplied. Positive instance queries stop
at the unchanged original16C000 entry; its native composition is a later proof.
"""
import inspect,json,struct
from pathlib import Path
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
OWNERS=(0x24dec0,0x24def0,0x24df20)
BACKEND='unchanged-tu15-native-instanced-ray-callbacks-v1'
def metadata(image):
 support.OWNERS=OWNERS;support.BOUNDARIES=(0x16c000,)
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[13,16,46];')
 env=dict(support.__dict__);exec(text,env);return env['metadata'](image)
def cases(image):
 rows=[]
 def make(entry,key=1,duplicate=False,alias=0):
  data=bytearray((n*37+11)&255 for n in range(0x3000));collection=0x1000100;child=0x1000200;collector=0x1000300;body=0x1000400;table=0x1000500;result=0x1000600;packet=base.physical.SP-0x30
  if alias==1:child=collection
  if alias==2:child=collection+12
  if alias==3:child=0
  def put(p,n,width=4):data[p-0x1000000:p-0x1000000+width]=(n&((1<<(8*width))-1)).to_bytes(width,'little')
  put(collection+12,0x1002000);put(collector,table);put(table,0x24df20);put(collector+28,result);put(body+4,key)
  for offset,value in((4,0x3f800000),(8,0x4808c2d),(12,9),(16,0x1000800),(20,0x1000900)):put(collector+offset,value)
  stamps=bytearray(0x40);stamp=0x12345678;struct.pack_into('<I',stamps,12,stamp);index=(key&65535);index=index-65536 if index&32768 else index;struct.pack_into('<I',stamps,16+index*4,stamp if duplicate else 0x87654321)
  c=dict(name=f'callback_{len(rows)}_{entry:x}_{key:x}_{duplicate}_{alias}',entry=entry,receiver=collection if entry==0x24dec0 else child if entry==0x24def0 else collector,arguments=[key,child]if entry==0x24dec0 else[0x1000700,body,collector]if entry==0x24def0 else[body,0x1000700],child=child,collector=collector,body=body,packet=packet,banks=[dict(address=0x1000000,before=data.hex()),dict(address=0x4f44e0,before=stamps.hex()),dict(address=0x46a0d0,before=image.read(0x46a0d0,4).hex())],before_fp=[0x27f|((len(rows)%4)<<10),((len(rows)%8)<<11)|0x453f,0x1f80|((len(rows)%4)<<13)],stop_pc=base.physical.STOP,boundary=None)
  if entry==0x24def0:c['banks'].append(dict(address=packet+20,before='a5a5a5a5'))
  rows.append(c)
 for key in(0,1,2,0x7fffffff,0x80000000,0xffffffff):
  for alias in range(4):make(0x24dec0,key=key,alias=alias)
 for entry in(0x24def0,0x24df20):
  for key in(0,1,2,0x33440002,0xffff,0xfffe):
   for duplicate in(False,True):make(entry,key=key,duplicate=duplicate)
 for seed in(rows[0],rows[24],rows[36]):c=dict(seed);c['name']='fault_seed_'+seed['name'];rows.append(c)
 return rows
def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.physical;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin)
 text=text[:begin]+'''  def boundary(c,actual):
   if actual['actual_stop']==physical.STOP:return None
   assert actual['actual_stop']==0x16c000
   gp,args=actual['boundary_gp'],actual['boundary_stack'];return dict(owner=0x16c000,instance=gp[0],result=gp[3],mask=args[1],filter=args[2],origin=args[3],displacement=args[4])
'''+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_instanced_ray_callbacks.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_instanced_ray_callbacks.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({13,16,46})').replace('auto services=native_tu15_contact_pressure_native_services();','NativeTu15InstancedRayCallbackServices services;').replace('NativeTu15ContactPressureStats stats;','std::uint32_t result{};').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS);begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+SERVICES+driver[end:];driver=driver.replace('   check(failed==fault_case',RETURN_CHECK+'   check(failed==fault_case');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('provider_key','child+16u,key','child+16u,0u'),('stamp_equality','m.word(stamp_address)==stamp','m.word(stamp_address)!=stamp'),('signed_index','std::int16_t(instance&0xffffu)','std::uint16_t(instance&0xffffu)')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","(project/'types.cpp').write_text('');shutil.copyfile(ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_instanced_ray_callbacks.hpp'")
 calls='''switch(c.at("entry").get<unsigned>()){
     case 0x24dec0:result=native_tu15_resolve_instanced_ray_child(m,c.at("receiver"),c.at("arguments")[0],c.at("arguments")[1],observer);break;
     case 0x24def0:native_tu15_forward_instanced_child_ray(m,c.at("body"),c.at("collector"),c.at("packet"),fp,services,observer);break;
     case 0x24df20:native_tu15_collect_instanced_scene_ray(m,c.at("collector"),c.at("body"),fp,0,services,observer);break;
    }'''
 services='''   services.query_instance=[&](auto instance,auto mask,auto filter,auto origin,auto delta,auto result,auto&)->bool{const auto&b=c.at("boundary");check(b.at("instance")==instance&&b.at("mask")==mask&&b.at("filter")==filter&&b.at("origin")==origin&&b.at("displacement")==delta&&b.at("result")==result,name+" actual instance query DATA differs");boundary(0x16c000u);return false;};
   services.collect=[&](auto owner,auto collector,auto body,auto packet,auto&current){check(owner==0x24df20u&&packet==c.at("packet"),name+" actual collector/packet differs");native_tu15_collect_instanced_scene_ray(m,collector,body,current,0,services,observer);};
'''
 check='''if(!fault_case&&!observer_failed&&c.at("entry")==0x24dec0)check(result==c.at("boundary_gp")[0],name+" native provider identity differs");'''
 env=dict(base.__dict__,__file__=__file__,CALLS=calls,RETURN_CHECK=check,SERVICES=services);exec(text,env);rc=env['main']();path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());code=_Code(base.Xbe(base.NEW_DEFAULT),'.text');sites=set().union(*(set(complete_body(code,owner).code)for owner in OWNERS));report.update(scope=__doc__,physically_executed_sites=len(sites&support.parent.EXECUTED),unexecuted_sites=sorted(sites-support.parent.EXECUTED),remaining=['16C000 full native callback composition and accepted-hit SSE suffix','Full16C620 caller/production/livecamera acceptance']);base.save(path,report);return rc
if __name__=='__main__':raise SystemExit(main())
