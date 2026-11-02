#!/usr/bin/env python3
"""Same native24 scene binding through real MOPP leaf/provider/instance returns.

Declared source-coherent BSP/instance inputs and authored geometry opcodes are
component controls, not current imported-world or live-hit claims. All public
banks/result/read/store/FP are strict; native private DATA is separately owned.
"""
import copy,inspect,json,struct
from pathlib import Path
import validate_native_camera_instanced_query_tu15 as instance
import validate_native_instanced_scene_ray_actual_tu15 as scene
import validate_native_instanced_ray_callbacks_tu15 as callbacks
import validate_native_mopp_ray_traversal_tu15 as core
import validate_native_contact_pressure_tu15 as base
import validate_native_dirty_island_objects_composed_tu15 as support
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
SETUP=ROOT/'analysis/physics/native-mopp-private-setup-stage-20261102/staged'
CALLBACKS=ROOT/'analysis/physics/native-instanced-ray-host-callback-stage-20261102/staged'
OWNERS=(*scene.OWNERS,*callbacks.OWNERS,*instance.OWNERS)
BACKEND='unchanged-tu15-native-instanced-scene-binding-v1'
EXECUTED=set()
def metadata(image):
 text=inspect.getsource(base.metadata).replace('assert not op.mem.segment;base,index=',"\n    if op.mem.segment:assert ins.reg_name(op.mem.segment)=='fs';continue\n    base,index=")
 # base metadata uses i rather than ins; use the family metadata with its
 # authenticated Source TEST/FST access policy and no hard-coded owner count.
 text=inspect.getsource(support.metadata);text=text.replace("    assert not op.mem.segment;base,index=","    if op.mem.segment:assert ins.reg_name(op.mem.segment)in('fs','es');continue\n    base,index=")
 text=text.replace('assert counts==[276,38,20];','')
 env=dict(support.__dict__,OWNERS=OWNERS,BOUNDARIES=());exec(text,env);meta=env['metadata'](image);mopp=core.metadata(image);meta['sites']+=mopp['sites'];meta['body_sites']+=[896];meta['listing_sha256'].update(mopp['listing_sha256']);return meta
def cases(image):
 all_seeds=[c for c in instance.cases(image)if c['entry']==0x16c000 and not c['name'].startswith('fault_seed')]
 seeds=[c for c in all_seeds if '_scale3f800000'in c['name']][:18]
 seeds+=all_seeds[-8:]
 rows=[]
 for seed in seeds:
  for cluster,fast,recursive in((0,False,False),(1,False,False),(0,True,False),(1,False,True)):
   c=copy.deepcopy(seed);g=c['entry_registers'];args=c['arguments'];key=g['eax'];B=0x1000000;scenario=B+0x6000;clusters=B+0xf000;code=B+0xf200;raw=bytearray.fromhex(c['banks'][0]['before'])
   def put(p,n,width=4):raw[p-B:p-B+width]=(n&((1<<(width*8))-1)).to_bytes(width,'little')
   put(scenario+0xa0,clusters);put(clusters+cluster*0xb0+0xa8,1);put(clusters+cluster*0xb0+0xac,code)
   for off,value in((0,0xc3000000 if recursive else 0),(4,0xc3000000 if recursive else 0),(8,0xc3000000 if recursive else 0),(12,0x3f800000),(0x24,1)):put(code+off,value)
   program=bytes((0x10,64,192,1,0x30+key,0x30+key))if recursive else bytes((0x30+key,))
   raw[code+0x30-B:code+0x30-B+len(program)]=program
   c['banks'][0]['before']=raw.hex();c['banks']=[b for b in c['banks']if not base.physical.STACK<=b['address']<base.physical.STACK+0x10000]
   stamps=bytearray(32);struct.pack_into('<I',stamps,0,0x12345678);c['banks'].append(dict(address=0x4f44ec,before=stamps.hex()))
   for p,n in((0x46625c,0x78),(0x46a520,4),(0x46a1bc,4),(0x41c500,16)):
    if not any(b['address']<=p and p+n<=b['address']+len(bytes.fromhex(b['before']))for b in c['banks']):c['banks'].append(dict(address=p,before=image.read(p,n).hex()))
   c.update(name='scene_binding_'+str(len(rows))+'_'+seed['name']+'_fast'+str(fast)+'_recursive'+str(recursive),entry=0x24dc90,receiver=0,entry_registers=dict(eax=cluster,esi=args[2],edi=args[3]),arguments=[args[0]|(0x10000000 if fast else 0),args[1],g['ebx']],cluster=cluster)
   rows.append(c)
 for seed in(rows[0],rows[12],rows[-2]):c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def tracked(*args,**kwargs):
 rows=support.parent.ORIGINAL_PHYSICAL(*args,**kwargs)
 for row in rows:EXECUTED.update(row['executed_sites'])
 return rows
def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=tracked;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_instanced_scene_ray.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_instanced_scene_ray_binding.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('check(fixture.at("body_sites")==J::array({220,2,2,2}),"Original contact-pressure closure changed");','').replace('auto services=native_tu15_contact_pressure_native_services();','').replace('NativeTu15ContactPressureStats stats;','std::uint32_t result{};NativeTu15BoundInstancedRayActivity activity;').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS);begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];driver=driver.replace('   check(failed==fault_case',RETURN_CHECK+'   check(failed==fault_case');driver=driver.replace('if(log_reads)reads.push_back','if(log_reads&&!(p>=0x6000000u&&std::uint64_t(p)+b.size()<=0x6010000u))reads.push_back').replace('if(log_writes)raw_stores.push_back','if(log_writes&&!(p>=0x6000000u&&std::uint64_t(p)+b.size()<=0x6010000u))raw_stores.push_back').replace('for(bool read_pass:{false,true}){','before.banks.emplace(0x6000000u,std::vector<std::byte>(0x10000,std::byte{0xa5}));for(bool read_pass:{false,true}){').replace('stores.push_back(store);','if(store.address>=0x6000000u&&std::uint64_t(store.address)+store.written.size()<=0x6010000u)return;stores.push_back(store);');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('hit_return','const auto hit=m.word(collector+24u,1u)!=0u','const auto hit=m.word(collector+24u,1u)==0u'),('bsp_filter','collector+12u,q.bsp_filter','collector+12u,0u'),('code_release','native_tu15_destroy_instanced_ray_shape(memory,shape','native_tu15_destroy_instanced_collection_base(memory,shape')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 units=('native_tu15_instanced_ray_lifetime.cpp','native_tu15_island_activation.cpp','native_tu15_mopp_ray_traversal.cpp','native_tu15_mopp_ray_setup.cpp','native_tu15_instanced_ray_callbacks.cpp','native_tu15_camera_instanced_query.cpp','native_tu15_camera_query_leaves.cpp','native_tu15_camera_dynamic_query_leaves.cpp','native_tu15_bsp_segment_query.cpp','native_tu15_bsp_surface_query.cpp')
 copy="(project/'types.cpp').write_text('');[shutil.copyfile((SETUP/'src/physics'/name)if name=='native_tu15_mopp_ray_setup.cpp'else(CALLBACKS/'src/physics'/name)if name=='native_tu15_instanced_ray_callbacks.cpp'else(ROOT/'src/physics'/name),project/name)for name in"+repr((*units,'native_tu15_joint_constraint_numeric_internal.hpp'))+"];shutil.copyfile(ROOT/'src/rendering/world/native_tu15_camera_projection.cpp',project/'native_tu15_camera_projection.cpp')"
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')",copy).replace("+'.cpp types.cpp)","+'.cpp types.cpp "+' '.join(units)+" native_tu15_camera_projection.cpp)")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_instanced_scene_ray_binding.hpp'")
 text=text.replace('rows+=physical.physical(image,meta,prefix,scratch)',"rmw={s['instruction']for s in meta['sites']if any(o['read']and o['write']for o in s['operands'])};excluded_rmw=[c['name']for c in prefix if c.get('fault',{}).get('access')=='write'and c['stop_pc']in rmw];prefix=[c for c in prefix if c['name']not in excluded_rmw];rows+=physical.physical(image,meta,prefix,scratch)")
 text=text.replace('report.update(passed=','report.update(excluded_rmw_writefaults=excluded_rmw,passed=')
 calls='''{auto a=c.at("arguments"),g=c.at("entry_registers");result=native_tu15_query_bound_instanced_scene_ray(m,{g.at("eax"),a[0],a[1],g.at("esi"),g.at("edi"),a[2]},{0x6000000u,0x800u},fp,0,observer,&activity);}'''
 check='''if(!fault_case&&!observer_failed){check(result==(c.at("boundary_gp")[0].get<std::uint32_t>()&255u),name+" scene binding return differs");check(activity.completed==1&&activity.refusals==0&&activity.unknown_owners==0&&activity.scene.setup.unprovided_reads==0&&activity.scene.setup.unprovided_writes==0,name+" scene binding completion/guard differs");}'''
 env=dict(base.__dict__,__file__=__file__,CALLS=calls,RETURN_CHECK=check,SETUP=SETUP,CALLBACKS=CALLBACKS);exec(text,env);rc=env['main']();path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());image=base.Xbe(base.NEW_DEFAULT);code=_Code(image,'.text');report.update(scope=__doc__,callback_coverage={hex(owner):dict(executed=len(set(complete_body(code,owner).code)&EXECUTED),total=len(complete_body(code,owner).code))for owner in callbacks.OWNERS},same_production_binding=True,captured_or_live_input_claimed=False);base.save(path,report);return rc
if __name__=='__main__':raise SystemExit(main())
