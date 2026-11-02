#!/usr/bin/env python3
"""Native BSP recursive segment/wrapper composed with genuine native surfaces."""
import copy,inspect,json,struct
from pathlib import Path
import validate_native_contact_pressure_tu15 as base
import validate_native_bsp_surface_query_tu15 as surface
import validate_native_dirty_island_objects_composed_tu15 as support
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
OWNERS=(0x1e68e0,0x1e6980,0x1e6cb0,0x2a0230,0x1e6e80)
BACKEND='unchanged-tu15-native-bsp-segment-query-v1'
C,D,N,P,L,O,V,OUT=0x1004000,0x1000100,0x1000600,0x1000800,0x1000900,0x1003000,0x1003100,0x1003800

def metadata(image):
 source=inspect.getsource(surface.metadata).replace('assert counts==[125,30,97];','assert counts==[42,262,125,30,97];');env=dict(surface.__dict__,OWNERS=OWNERS);exec(source,env);return env['metadata'](image)
def cases(image):
 rows=[];seed=surface.cases(image)[326]
 def make(flags=1,previous=1,kind=0,node=0,minimum=0,maximum=0x3f800000,origin=0xbf800000,delta=0x40000000,count=0,wrapper=False,epsilon=0x3f800000,normal=0x3f800000):
  c=copy.deepcopy(seed);data=bytearray.fromhex(c['banks'][0]['before'])
  def put(p,n,width=4):data[p-0x1000000:p-0x1000000+width]=(n&((1<<(8*width))-1)).to_bytes(width,'little')
  for p,n in((D+4,N),(D+12,P),(D+20,L),(C,flags),(C+4,D),(C+8,64),(C+12,0),(C+16,O),(C+20,V),(C+24,OUT),(C+28,0),(C+32,previous),(C+36,0),(OUT,0x3f800000),(OUT+24,count),(P,normal),(P+4,0),(P+8,0),(P+12,0),(O,origin),(O+4,0),(O+8,0),(V,delta),(V+4,0),(V+8,0)):put(p,n)
  put(L,kind,1);put(L+1,1,1);put(L+2,0,2)
  # One plane with two genuine packed signed24-bit leaves. Each source read
  # starts one byte before the packed child and consumes an ordinary dword.
  put(N,0,2);put(N+2,0x800000,3);put(N+5,0xffffff,3)
  context=base.physical.SP-40 if wrapper else C
  c.update(name=f'bspseg_{len(rows)}_flags{flags}_prev{previous}_kind{kind}',entry=0x1e68e0 if wrapper else 0x1e6980,receiver=flags if wrapper else 0,entry_registers={'eax':OUT,'edx':D}if wrapper else {},arguments=[64,0,O,V]if wrapper else[C,node,minimum,maximum],before_fp=[0x27f|((len(rows)%4)<<10),0x4500|(len(rows)%32),0x1f80|((len(rows)%4)<<13)],context=context,query=[flags,D,OUT,epsilon,64,0,O,V],xmm0_bits=epsilon)
  c['banks'][0]['before']=data.hex();c['banks'].append(dict(address=0x46a0d0,before=image.read(0x46a0d0,4).hex()))
  if wrapper:c['banks'].append(dict(address=context,before=bytes([0xa5]*40).hex()))
  rows.append(c)
 for flags in range(32):
  for previous in(0,1,2,3):
   for kind in(0,1):make(flags=flags,previous=previous,kind=kind,node=0x800000)
 for origin,delta in((0xbf800000,0x40000000),(0x3f800000,0xc0000000),(0x3f800000,0),(0xbf800000,0),(0,0),(0x80000000,0),(1,0x3f800000),(0x7fc12345,0x3f800000),(0x7f812345,0x3f800000)):
  for flags in(1,2,3,4,9):make(flags=flags,origin=origin,delta=delta)
 for count in(255,256,0xffffffff,0x80000000):make(node=0x800000,count=count)
 for epsilon in(0,0x80000000,0xbf800000,0x3f000000,0x3f800000,0x40000000,0x7fc12345):make(wrapper=True,epsilon=epsilon)
 for seed in(rows[0],rows[22],rows[256],rows[-4]):c=copy.deepcopy(seed);c['name']='fault_seed_'+seed['name'];rows.append(c)
 return rows

def main():
 # Reference inputs include actual XMM0 scalar epsilon, marshaled before
 # entry; no GP or output postimage is consumed by native code.
 original=support.parent.ORIGINAL_PHYSICAL
 source=inspect.getsource(original).replace("entry_setup='\\n'.join", "entry_setup='\\n'.join")
 needle="  assembly.append(f'''.code32";source=source.replace(needle,"  if 'xmm0_bits'in c:entry_setup=' mov $'+str(c['xmm0_bits'])+',%eax\\n movd %eax,%xmm0\\n'+entry_setup\n"+needle)
 env=dict(base.physical.__dict__);assert source.count("movd %eax,%xmm0")==1;exec(source,env);base.physical.physical=env['physical'];base.physical.observer_source=support.parent.observer_source
 base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 # Atomic INC has a separately declared read/modify/write write-fault gap:
 # the before-instruction oracle cannot establish its after-read failure.
 text=text.replace("if e['instruction']in seen:continue","if e['instruction']in seen or(category=='writes'and e['instruction']==0x1e6c81):continue")
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_bsp_segment_query.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_bsp_segment_query.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({42,262,125,30,97})').replace('auto services=native_tu15_contact_pressure_native_services();','').replace('NativeTu15ContactPressureStats stats;','std::uint32_t result{};').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS);begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];driver=driver.replace('   check(failed==fault_case',RETURN_CHECK+'   check(failed==fault_case');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('child_side','unsigned(min_side)*3u','unsigned(!min_side)*3u'),('count_increment','current_count+1u','current_count+2u'),('query_flag','const auto first=flags&1u','const auto first=flags&2u')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","shutil.copyfile(ROOT/'src/physics/native_tu15_bsp_surface_query.cpp',project/'types.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_bsp_segment_query.hpp',ROOT/'src/physics/native_tu15_bsp_surface_query.cpp'")
 calls='''if(c.at("entry")==0x1e68e0){auto q=c.at("query");result=native_tu15_query_bsp_segment(m,{q[0],q[1],q[2],q[3],q[4],q[5],q[6],q[7]},c.at("context"),fp,0,observer);}else{auto a=c.at("arguments");result=native_tu15_traverse_bsp_segment(m,a[0],a[1],a[2],a[3],fp,0,observer);}'''
 check='''if(!fault_case&&!observer_failed)check((result&255u)==(c.at("boundary_gp")[0].get<std::uint32_t>()&255u),name+" BSP bool return differs");'''
 namespace=dict(base.__dict__,__file__=__file__,CALLS=calls,RETURN_CHECK=check);exec(text,namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
