#!/usr/bin/env python3
"""Native16C370 recursive object query with native-only ray/context/material children.

Root1082B DATA and every persistent bank are strict. Recursive frames and
child ray workspaces are independently owned native DATA; their complete
in-lifetime proofs are separate. No CPU-frame/postimage is serialized.
"""
import copy,inspect
from pathlib import Path
import validate_native_camera_collision_ray_tu15 as ray
import validate_native_contact_pressure_tu15 as base
import validate_native_dirty_island_objects_composed_tu15 as support
ROOT=Path(__file__).resolve().parents[1];OWNERS=(0x16c370,0x16b870,0x120fa0,0x1f7e00,0x1f8200,0xdb030,0x16b9a0,0x1f7e90,0x1f7f60,0x144090,0x1e6980,0x1e6cb0,0x2a0230,0x1e6e80)
COUNTS='124,60,76,42,177,29,107,74,86,82,262,125,30,97';BACKEND='unchanged-tu15-native-camera-dynamic-query-v1'

def metadata(image):
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==['+COUNTS+'];').replace('assert not op.mem.segment','assert not op.mem.segment or ins.reg_name(op.mem.segment)=='+repr('es'));env=dict(support.__dict__,OWNERS=OWNERS,BOUNDARIES=());exec(text,env);return env['metadata'](image)
def cases(image):
 rows=[];seeds=ray.cases(image)
 def make(hierarchy=0,recursive=1,mask=0x10,model_none=0,ignore=0,current_fraction=0x3f800000):
  c=copy.deepcopy(seeds[20]);raw=bytearray.fromhex(c['banks'][0]['before']);B=0x1000000;objects=B+0x9a00;headers=B+0xa000;tags=B+0x9000;mapping=B+0x9700;perms=B+0x9900;materials=B+0x9c00;result=B+0xd000
  def put(p,n,width=4):raw[p-B:p-B+width]=(n&((1<<(8*width))-1)).to_bytes(width,'little')
  put(objects+0x44,headers)
  for index,unit in enumerate((B+0x7000,B+0x8000,B+0xb000)):
   definition=(1,4,6)[index];model=(2,5,7)[index];tag=B+0x9200+index*0x40;model_tag=B+0x9400+index*0x80
   put(headers+index*12+8,unit);put(headers+index*12+2,0,1);put(headers+index*12+3,0,1);put(unit,definition);put(unit+4,0x80400000);put(unit+12,0xffffffff);put(unit+16,0xffffffff);put(unit+0x116,0x400,2);put(unit+0x11a,0x300,2)
   for off in(0x40,0x44,0x48):put(unit+off,0)
   put(unit+0x4c,0x42c80000)
   for n in range(16):put(unit+0x300+n,0,1)
   for node in range(3):
    for lane,value in enumerate((0x3f800000,0x3f800000,0,0,0,0x3f800000,0,0,0,0x3f800000,0,0,0)):put(unit+0x400+node*52+lane*4,value)
   put(tags+definition*16+8,tag);put(tag+0x38,0xffffffff if model_none==1 or(model_none==2 and index==0)else model);put(tags+model*16+8,model_tag);put(model_tag+12,3);put(model_tag+0x70,2);put(model_tag+0x74,mapping);put(model_tag+0x5c,materials)
  put(tags+3*16+8,ray.regions.C)
  for index in range(2):put(mapping+index*16+4,index,1);put(mapping+index*16+8,1);put(mapping+index*16+12,perms+index*8);put(perms+index*8+5,0,1)
  for index in range(-1,3):put(materials+index*20+16,7+index,2)
  for index in range(2):
   for off,value,width in((0,0,2),(4,0,1),(5,0,1),(6,0,2)):put(B+0x11800+index*8+off,value,width)
  if hierarchy in(1,3):put(B+0x7000+12,0xabcd0002)
  if hierarchy in(2,3,4):put(B+0x7000+16,0xabcd0001)
  if hierarchy==4:put(B+0x8000+16,0xabcd0002)
  put(result+4,current_fraction)
  before_fp=c['before_fp'];root_material=base.physical.SP-0x43a
  c.update(name=f'dynamic_parent_{len(rows)}_tree{hierarchy}_recursive{recursive}_mask{mask:x}_model{model_none}',entry=0x16c370,receiver=0,entry_registers={},arguments=[0xabcd0000,recursive,mask,1,B+0x5400,B+0x5500,0xabcd0000 if ignore==1 else 0xffffffff,0xabcd0001 if ignore==2 else 0xffffffff,result],root_material=root_material,root_context=base.physical.SP-0x438,root_ray=base.physical.SP-0x424,banks=[dict(address=B,before=raw.hex()),dict(address=root_material,before=bytes(1082).hex()),*[b for b in c['banks']if b['address']not in(B,)and not base.physical.STACK<=b['address']<base.physical.STACK+0x10000],dict(address=0x4ecbd0,before=objects.to_bytes(4,'little').hex()),dict(address=0x4f0414,before=tags.to_bytes(4,'little').hex()),dict(address=0x48a0b0,before='0700')])
  rows.append(c)
 for hierarchy in range(5):
  for recursive in(0,1,0x100):
   for model_none in(0,1,2):make(hierarchy=hierarchy,recursive=recursive,model_none=model_none)
 for mask in(0,0x20010,0x10000010,0x80010,0x40010,0x40000010):
  for hierarchy in(0,3,4):make(hierarchy=hierarchy,mask=mask)
 for ignore in(1,2):
  for recursive in(0,1):make(hierarchy=3,recursive=recursive,ignore=ignore)
 for fraction in(0,0x3e800000,0x7fc12345,0x7f812345):make(hierarchy=3,current_fraction=fraction)
 for n in(0,2,18,44,62):c=copy.deepcopy(rows[n]);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows

def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.ORIGINAL_PHYSICAL;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 text=text.replace("if e['instruction']in seen:continue","if e['instruction']in seen or(category=='writes'and e['instruction']==0x1e6c81):continue")
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_camera_dynamic_query.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_camera_dynamic_query.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({124,60,76,42,177,29,107,74,86,82,262,125,30,97})').replace('auto services=native_tu15_contact_pressure_native_services();','').replace('NativeTu15ContactPressureStats stats;','std::uint32_t result{};').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS);begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];driver=driver.replace('   check(failed==fault_case',RETURN_CHECK+'   check(failed==fault_case');driver=driver.replace('if(log_reads)reads.push_back','if(log_reads&&!(p>=0x6000000u&&std::uint64_t(p)+b.size()<=0x6010000u))reads.push_back').replace('if(log_writes)raw_stores.push_back','if(log_writes&&!(p>=0x6000000u&&std::uint64_t(p)+b.size()<=0x6010000u))raw_stores.push_back').replace('for(bool read_pass:{false,true}){','before.banks.emplace(0x6000000u,std::vector<std::byte>(0x10000));for(bool read_pass:{false,true}){').replace('stores.push_back(store);','if(store.address>=0x6000000u&&std::uint64_t(store.address)+store.written.size()<=0x6010000u)return;stores.push_back(store);');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('recursive_admission','if(!(q.recursive&255u))','if(q.recursive&255u)'),('child_flag','q.mask&0x20000u','q.mask&0x40000u'),('material_publication','0x16c467u,q.result+36u','0x16c467u,q.result+38u')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 units=('native_tu15_camera_dynamic_query_leaves.cpp','native_tu15_camera_query_object_context.cpp','native_tu15_camera_query_leaves.cpp','native_tu15_camera_collision_ray.cpp','native_tu15_camera_collision_regions.cpp','native_tu15_bsp_segment_query.cpp','native_tu15_bsp_surface_query.cpp')
 copy="(project/'types.cpp').write_text('');[shutil.copyfile(ROOT/'src/physics'/name,project/name)for name in"+repr((*units,'native_tu15_joint_constraint_numeric_internal.hpp'))+"];shutil.copyfile(ROOT/'src/rendering/world/native_tu15_camera_projection.cpp',project/'native_tu15_camera_projection.cpp')"
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')",copy)
 text=text.replace("+'.cpp types.cpp)","+'.cpp types.cpp "+' '.join(units)+" native_tu15_camera_projection.cpp)")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_camera_dynamic_query.hpp',ROOT/'src/physics/native_tu15_camera_collision_ray.cpp'")
 calls='''{auto a=c.at("arguments");NativeTu15CameraDynamicServices services;services.acquire_frame=[&](unsigned level){const auto private_data=0x6000000u+0x1000u+level*0x1000u;NativeTu15CameraDynamicFrame frame;frame.ray={private_data,private_data+64u,private_data+52u,private_data+76u,private_data+116u};if(!level){frame.material=c.at("root_material");frame.collision_context=c.at("root_context");frame.ray_result=c.at("root_ray");}else{frame.material=private_data+0x200u;frame.collision_context=private_data+0x204u;frame.ray_result=private_data+0x220u;}return frame;};result=native_tu15_query_dynamic_camera_object(m,{a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8]},fp,0,services,observer);}'''
 check='''if(!fault_case&&!observer_failed)check((result&255u)==(c.at("boundary_gp")[0].get<std::uint32_t>()&255u),name+" recursive dynamic bool return differs");'''
 env=dict(base.__dict__,__file__=__file__,CALLS=calls,RETURN_CHECK=check);exec(text,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
