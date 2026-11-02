#!/usr/bin/env python3
"""Native1F7E90/1F7F60 region iterator with lazy mutable DATA and full source stores."""
import copy,inspect
from pathlib import Path
import validate_native_contact_pressure_tu15 as base
import validate_native_dirty_island_objects_composed_tu15 as support
ROOT=Path(__file__).resolve().parents[1];OWNERS=(0x1f7e90,0x1f7f60);BACKEND='unchanged-tu15-native-camera-collision-regions-v1'
B,C,I,S,M,MAP,R,PERM,BSPS=0x1000000,0x1000100,0x1000300,0x1000400,0x1000500,0x1000800,0x1001000,0x1002000,0x1003000

def metadata(image):
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[74,86];');env=dict(support.__dict__,OWNERS=OWNERS,BOUNDARIES=());exec(text,env);return env['metadata'](image)
def cases(image):
 rows=[]
 def make(entry,has_state=1,has_model=0,region_count=2,model_count=2,state_byte=0,map_byte=0,selected=0,encoded=0xffffffff,bsp_count=2,permutation_count=2,alias=0):
  data=bytearray((n*29+7)&255 for n in range(0x6000))
  def put(p,n,width=4):data[p-B:p-B+width]=(n&((1<<(8*width))-1)).to_bytes(width,'little')
  iterator=I if not alias else S-4
  put(C+28,region_count);put(C+32,R);put(M+0x70,model_count);put(M+0x74,MAP)
  for n in range(16):
   put(S+n,state_byte,1);put(MAP+n*16+4,n if map_byte==0 else map_byte,1);put(MAP+n*16+8,permutation_count);put(MAP+n*16+12,PERM+0x800+n*0x20)
   for p in range(3):put(PERM+0x800+n*0x20+p*8+5,selected,1)
  for region in range(3):
   put(R+region*12+4,permutation_count);put(R+region*12+8,PERM+region*0x100)
   for permutation in range(3):
    record=PERM+region*0x100+permutation*20;put(record+4,bsp_count);put(record+8,BSPS+region*0x800+permutation*0x200)
    for bsp in range(3):put(BSPS+region*0x800+permutation*0x200+bsp*0x44,0x10+region+permutation+bsp,1)
  put(iterator,C)
  for n in range(16):put(iterator+4+n,state_byte,1)
  put(iterator+20,encoded);put(iterator+24,0x12345678)
  c=dict(name=f'regions_{len(rows)}_{entry:x}_alias{alias}',entry=entry,receiver=0,entry_registers={},arguments=[],banks=[dict(address=B,before=data.hex())],before_fp=[0x27f|((len(rows)%4)<<10),((len(rows)%8)<<11)|0x453f,0x1f80|((len(rows)%4)<<13)],stop_pc=base.physical.STOP,boundary=None)
  if entry==0x1f7e90:c.update(entry_registers=dict(eax=C,ebx=S if has_state else 0),arguments=[iterator,M if has_model else 0])
  else:c.update(entry_registers=dict(esi=iterator))
  rows.append(c)
 for has_state,has_model in((0,0),(0,1),(1,0),(1,1)):
  for model_count in(0,1,2,16,17,0xffffffff,0x80000000):
   for state_byte in(0,1,2,0xff,0x80):make(0x1f7e90,has_state=has_state,has_model=has_model,model_count=model_count,state_byte=state_byte)
 for map_byte in(0xff,0xfe,1):
  for selected in(0,1,0xff):make(0x1f7e90,has_model=1,map_byte=map_byte,selected=selected)
 for state_byte in(0,1,0xff):make(0x1f7e90,has_model=0,state_byte=state_byte,alias=1)
 for region_count in(0,1,2,3,0xffffffff,0x80000000):
  for encoded in(0xffffffff,0x00000010,0x00000111,0x00010111,0x01000011,0x01010111):
   for state_byte in(0,1,0xff):make(0x1f7f60,region_count=region_count,encoded=encoded,state_byte=state_byte)
 for bsp_count in(0,1,2,0xffffffff,0x80000000):
  for permutation_count in(0,1,2,0xffffffff):make(0x1f7f60,bsp_count=bsp_count,permutation_count=permutation_count)
 for seed in(rows[0],rows[106],rows[140],rows[152],rows[-1]):c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows

def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.ORIGINAL_PHYSICAL;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_camera_collision_regions.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_camera_collision_regions.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({74,86})').replace('auto services=native_tu15_contact_pressure_native_services();','').replace('NativeTu15ContactPressureStats stats;','std::uint32_t result{};').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS);begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];driver=driver.replace('   check(failed==fault_case',RETURN_CHECK+'   check(failed==fault_case');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('state_default','iterator+4u+n*4u,0)','iterator+4u+n*4u,1)'),('bsp_stride','row=bsps+bsp*0x44u','row=bsps+bsp*0x40u'),('mapping_byte','if(selected!=0xffu)','if(selected==0xffu)')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","(project/'types.cpp').write_text('')")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_camera_collision_regions.hpp'")
 calls='''{auto g=c.at("entry_registers");if(c.at("entry")==0x1f7e90)native_tu15_begin_camera_collision_regions(m,g.at("eax"),g.at("ebx"),c.at("arguments")[0],c.at("arguments")[1],observer);else result=native_tu15_next_camera_collision_region(m,g.at("esi"),observer);}'''
 check='''if(!fault_case&&!observer_failed&&c.at("entry")==0x1f7f60)check((result&255u)==(c.at("boundary_gp")[0].get<std::uint32_t>()&255u),name+" collision region bool return differs");'''
 env=dict(base.__dict__,__file__=__file__,CALLS=calls,RETURN_CHECK=check);exec(text,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
