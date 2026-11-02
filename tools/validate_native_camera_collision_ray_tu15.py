#!/usr/bin/env python3
"""Native1F8200 ray parent with sole region/inverse/BSP/surface kernels, no CPU children."""
import copy,inspect,struct
from pathlib import Path
import validate_native_camera_collision_regions_tu15 as regions
import validate_native_bsp_segment_query_tu15 as bsp
import validate_native_contact_pressure_tu15 as base
import validate_native_dirty_island_objects_composed_tu15 as support
ROOT=Path(__file__).resolve().parents[1];OWNERS=(0x1f8200,0x1f7e90,0x1f7f60,0x144090,0x1e6980,0x1e6cb0,0x2a0230,0x1e6e80);BACKEND='unchanged-tu15-native-camera-collision-ray-v1'
COUNTS='177,74,86,82,262,125,30,97'
def metadata(image):
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==['+COUNTS+'];');text=text.replace('assert not op.mem.segment','assert not op.mem.segment or ins.reg_name(op.mem.segment)=='+repr('es'));env=dict(support.__dict__,OWNERS=OWNERS,BOUNDARIES=());exec(text,env);return env['metadata'](image)
def cases(image):
 rows=[];region_seed=regions.cases(image)[240];bsp_seed=bsp.cases(image)[256]
 def make(count=2,scale=0x3f800000,origin=0xbf800000,delta=0x40000000,flags=1,rounding=0):
  c=copy.deepcopy(region_seed);raw=bytearray(0x20000);region_raw=bytes.fromhex(region_seed['banks'][0]['before']);raw[:len(region_raw)]=region_raw;shape=bytearray.fromhex(bsp_seed['banks'][0]['before'])
  for offset in range(0,len(shape)-3,4):
   value=int.from_bytes(shape[offset:offset+4],'little')
   if 0x1000000<=value<0x1005000:struct.pack_into('<I',shape,offset,value+0x10000)
  raw[0x10000:0x10000+len(shape)]=shape
  B=0x1000000;context=B+0x5800;matrices=B+0x5000;O=B+0x5400;V=B+0x5500;R=B+0x6000
  def put(p,n,width=4):raw[p-B:p-B+width]=(n&((1<<(8*width))-1)).to_bytes(width,'little')
  put(regions.C+28,count)
  for n in range(16):put(regions.S+n,0,1)
  descriptor=bytes(shape[0x100:0x140])
  for region in range(3):
   for permutation in range(3):
    for node in range(3):
     entry=regions.BSPS+region*0x800+permutation*0x200+node*0x44;put(entry,node,1);raw[entry+4-B:entry+68-B]=descriptor
  for n in range(3):
   frame=matrices+n*52
   for lane,value in enumerate((scale,0x3f800000,0,0,0,0x3f800000,0,0,0,0x3f800000,0,0,0)):put(frame+lane*4,value)
  for lane,value in enumerate((0xffffffff,0,regions.C,regions.S,matrices)):put(context+lane*4,value)
  for lane,value in enumerate((origin,0,0)):put(O+lane*4,value)
  for lane,value in enumerate((delta,0,0)):put(V+lane*4,value)
  data=base.physical.SP-0x90
  constants=[b for b in bsp_seed['banks']if b['address']not in(0x1000000,)]
  c.update(name=f'collision_ray_{len(rows)}_count{count}_scale{scale:x}_flags{flags}',entry=0x1f8200,receiver=0,entry_registers={},arguments=[context,flags,O,V,R],inverse=data,local_displacement=data+52,local_origin=data+64,bsp_context=data+76,iterator=data+116,before_fp=[0x27f|(rounding<<10),0x4520,0x1f80|(rounding<<13)],banks=[dict(address=B,before=raw.hex()),dict(address=data,before=bytes(144).hex()),*constants])
  # Any Source readonly constant used by the existing inverse is genuine XBE.
  mapped=[(b['address'],len(bytes.fromhex(b['before'])))for b in c['banks']]
  for p in(0x45695c,0x46a0d0):
   if not any(first<=p and p+4<=first+n for first,n in mapped):c['banks'].append(dict(address=p,before=image.read(p,4).hex()))
  rows.append(c)
 for count in(0,1,2,0xffffffff):
  for scale in(0,0x3f800000,0xbf800000,0x3f000000):
   for flags in(1,2,3,9):make(count=count,scale=scale,flags=flags)
 for origin,delta in((0x3f800000,0xc0000000),(0,0),(0x80000000,0x40000000),(1,0x80000001),(0x7fc12345,0x40000000),(0x7f812345,0x3f800000),(0x7f800000,0xff800000)):
  for rounding in range(4):make(origin=origin,delta=delta,rounding=rounding)
 for index in(0,20,36,52):c=copy.deepcopy(rows[index]);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows

def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.ORIGINAL_PHYSICAL;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 text=text.replace("if e['instruction']in seen:continue","if e['instruction']in seen or(category=='writes'and e['instruction']==0x1e6c81):continue")
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_camera_collision_ray.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_camera_collision_ray.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({177,74,86,82,262,125,30,97})').replace('auto services=native_tu15_contact_pressure_native_services();','').replace('NativeTu15ContactPressureStats stats;','std::uint32_t result{};').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS);begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];driver=driver.replace('   check(failed==fault_case',RETURN_CHECK+'   check(failed==fault_case');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('matrix_stride','frame=matrices+node*52u','frame=matrices+node*48u'),('origin_translation','inverse+40u','inverse+44u'),('encoded_publication','q.result,encoded','q.result,encoded+1u')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","(project/'types.cpp').write_text('');[shutil.copyfile(ROOT/'src/physics'/name,project/name)for name in('native_tu15_bsp_segment_query.cpp','native_tu15_bsp_surface_query.cpp','native_tu15_camera_collision_regions.cpp','native_tu15_joint_constraint_numeric_internal.hpp')];shutil.copyfile(ROOT/'src/rendering/world/native_tu15_camera_projection.cpp',project/'native_tu15_camera_projection.cpp')")
 text=text.replace("+'.cpp types.cpp)","+'.cpp types.cpp native_tu15_bsp_segment_query.cpp native_tu15_bsp_surface_query.cpp native_tu15_camera_collision_regions.cpp native_tu15_camera_projection.cpp)")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_camera_collision_ray.hpp',ROOT/'src/physics/native_tu15_bsp_segment_query.cpp'")
 calls='''{auto a=c.at("arguments");result=native_tu15_query_camera_collision_ray(m,{a[0],a[1],a[2],a[3],a[4]},{c.at("inverse"),c.at("local_origin"),c.at("local_displacement"),c.at("bsp_context"),c.at("iterator")},fp,0,observer);}'''
 check='''if(!fault_case&&!observer_failed)check((result&255u)==(c.at("boundary_gp")[0].get<std::uint32_t>()&255u),name+" collision ray bool return differs");'''
 env=dict(base.__dict__,__file__=__file__,CALLS=calls,RETURN_CHECK=check);exec(text,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
