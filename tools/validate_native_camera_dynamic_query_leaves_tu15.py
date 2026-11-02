#!/usr/bin/env python3
"""Actual16B870 admission,16B9A0 transform/publication,DB030 material DATA kernels."""
import copy,inspect,struct
from pathlib import Path
import validate_native_contact_pressure_tu15 as base
import validate_native_dirty_island_objects_composed_tu15 as support
OWNERS=(0x16b870,0x16b9a0,0xdb030)
BACKEND='unchanged-tu15-native-camera-dynamic-query-leaves-v1'
ROOT=Path(__file__).resolve().parents[1]
B,H,O,T,C,P,R=0x1000000,0x1000100,0x1001000,0x1002000,0x1003000,0x1003100,0x1004000

def metadata(image):
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[60,107,29];');env=dict(support.__dict__,OWNERS=OWNERS,BOUNDARIES=());exec(text,env);return env['metadata'](image)
def cases(image):
 rows=[]
 def make(entry,kind=0,mask=0xffffffff,header_flags=0,object_flags=0,special=0,index=0,transform=True,reverse=False,value=0x3f800000,alias=0,object_none=False,model_none=False,match=0):
  data=bytearray((n*37+11)&255 for n in range(0x6000))
  def put(p,n,width=4):data[p-B:p-B+width]=(n&((1<<(8*width))-1)).to_bytes(width,'little')
  put(H+2,header_flags,1);put(H+3,kind,1);put(O+4,object_flags);put(O+0x10a,special,1);put(O+0x348,special,1);put(O+0x12c,special)
  for lane,n in enumerate((value,0x40000000,0x40400000,0xc0800000)):put(P+lane*4,n)
  for lane,n in enumerate((0x3f000000,0x3f800000,0,0,0,0x3f800000,0,0,0,0x3f800000,0x40a00000,0x40c00000,0x40e00000)):put(T+lane*4,n)
  for lane,n in enumerate((0x3f000000,P,1,2,0x8000 if reverse else 0)):put(C+lane*4,n)
  put(C+20,3,1);put(C+21,4,1);put(C+22,5,2)
  objects=B+0x500;headers=B+0x800;tags=B+0x600;tag=B+0x1600;model=B+0x1800;materials=B+0x1900
  put(objects+0x44,headers);put(headers+8,O);put(O,1);put(tags+24,tag);put(tag+0x38,0xffffffff if model_none else 2);put(tags+40,model);put(model+0x5c,materials)
  for n in range(-1,3):put(materials+n*20+16,0x1234+n,2)
  output=R if alias==0 else(P-40 if alias==1 else T-40 if alias==2 else C-4)
  banks=[dict(address=B,before=data.hex()),dict(address=0x4ecbd0,before=objects.to_bytes(4,'little').hex()),dict(address=0x4f0414,before=tags.to_bytes(4,'little').hex()),dict(address=0x48a0b0,before='2143')]
  c=dict(name=f'dynamic_leaf_{len(rows)}_{entry:x}_k{kind}_alias{alias}',entry=entry,receiver=0,entry_registers={},arguments=[],banks=banks,before_fp=[0x27f|((len(rows)%4)<<10),0x4500|(len(rows)%32),0x1f80|((len(rows)%4)<<13)],stop_pc=base.physical.STOP,boundary=None)
  if entry==0x16b870:c.update(receiver=0xabcd0000,entry_registers=dict(edx=H,esi=O,edi=mask),arguments=[0xabcd0000 if match==1 else 0xffffffff,0xabcd0000 if match==2 else 0xffffffff])
  elif entry==0x16b9a0:c.update(receiver=T if transform else 0,entry_registers=dict(eax=output,edx=C))
  else:c.update(receiver=0xffffffff if object_none else 0xabcd0000,entry_registers=dict(eax=output,esi=index))
  rows.append(c)
 for kind in(0,1,2,10,11,12,27,28,255):
  for mask in(0,1<<((kind+4)&31),0xffffffff):
   for header_flags in(0,0x10):
    for flags in(0,1,0x100000,0x400000,0x80000000):make(0x16b870,kind=kind,mask=mask,header_flags=header_flags,object_flags=flags)
 for kind in(0,11):
  for special in(0,2,4,8,255):make(0x16b870,kind=kind,object_flags=0xc00000,special=special)
 for kind in(0,11):
  for special in(0,2,4,8,255):make(0x16b870,kind=kind,object_flags=0x80400000,special=special)
 for match in(1,2):make(0x16b870,match=match)
 for transform in(False,True):
  for reverse in(False,True):
   for value in(0x3f800000,0,0x80000000,1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
    for alias in range(4):make(0x16b9a0,transform=transform,reverse=reverse,value=value,alias=alias)
 for object_none,model_none in((True,False),(False,True),(False,False)):
  for index in(0,1,0xffffffff):
   for alias in(0,3):make(0xdb030,index=index,object_none=object_none,model_none=model_none,alias=alias)
 for n in(1,80,145,280,292,328,365,len(rows)-1):c=copy.deepcopy(rows[n]);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows

def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.ORIGINAL_PHYSICAL;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_camera_dynamic_query_leaves.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_camera_dynamic_query_leaves.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({60,107,29})').replace('auto services=native_tu15_contact_pressure_native_services();','').replace('NativeTu15ContactPressureStats stats;','std::uint32_t result{};').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS);begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];driver=driver.replace('   check(failed==fault_case',RETURN_CHECK+'   check(failed==fault_case');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('type_shift','((kind+4u)&31u)','((kind+3u)&31u)'),('plane_scale','transform+48u','transform+44u'),('material_stride','materials+index*20u','materials+index*16u')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","(project/'types.cpp').write_text('')")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_camera_dynamic_query_leaves.hpp'")
 calls='''{auto g=c.at("entry_registers");switch(c.at("entry").get<unsigned>()){case 0x16b870:result=native_tu15_admit_camera_query_object(m,c.at("receiver"),g.at("edx"),g.at("esi"),g.at("edi"),c.at("arguments")[0],c.at("arguments")[1]);break;case 0x16b9a0:native_tu15_publish_camera_collision(m,g.at("eax"),c.at("receiver"),g.at("edx"),fp,observer);break;case 0xdb030:result=native_tu15_camera_collision_material_to(m,c.at("receiver"),g.at("esi"),g.at("eax"),observer);break;}}'''
 check='''if(!fault_case&&!observer_failed&&c.at("entry")!=0x16b9a0){const auto mask=c.at("entry")==0x16b870?255u:0xffffffffu;check((result&mask)==(c.at("boundary_gp")[0].get<std::uint32_t>()&mask),name+" dynamic leaf scalar return differs");}'''
 env=dict(base.__dict__,__file__=__file__,CALLS=calls,RETURN_CHECK=check);exec(text,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
