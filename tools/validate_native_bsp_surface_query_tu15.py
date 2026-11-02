#!/usr/bin/env python3
"""Source-owned BSPsurface parent plus2Dtree/polygon, returns/FP/ordered DATA and faults."""
import inspect,json,struct
from pathlib import Path
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x1e6cb0,0x2a0230,0x1e6e80)
BACKEND='unchanged-tu15-native-bsp-surface-query-v1'
ROOT=Path(__file__).resolve().parents[1]
def metadata(image):
 support.OWNERS=OWNERS;support.BOUNDARIES=()
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[125,30,97];')
 text=text.replace("if op.type!=X86_OP_MEM or ins.mnemonic=='lea':continue","if op.type!=X86_OP_MEM or ins.mnemonic in('lea','prefetcht0'):continue")
 environment=dict(support.__dict__);exec(text,environment);return environment['metadata'](image)
def cases(image):
 rows=[]
 def make(entry,pattern=0,axis=2,winding=1,point=(0x3e800000,0x3e800000),node=0,force=1,bitmap_mode=0,normal=(0,0,0x3f800000),fraction=0x3f000000,plane=0,rounding=0):
  data=bytearray((n*37+11)&255 for n in range(0x5000));D=0x1000100;P=0x1000800;L=0x1000900;E=0x1000a00;T=0x1001000;S=0x1001800;ED=0x1002000;V=0x1002800;O=0x1003000;DELTA=0x1003100;POINT=0x1003200;B=0x1003300
  def put(at,value,width=4):data[at-0x1000000:at-0x1000000+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
  for at,value in((D+12,P),(D+0x14,L),(D+0x1c,E),(D+0x24,T),(D+0x2c,S),(D+0x34,ED),(D+0x3c,V)):put(at,value)
  for n in range(3):
   for lane,value in enumerate(normal):put(P+n*16+lane*4,value)
  put(L+1,(0,1,2)[pattern%3],1);put(L+2,0,2)
  for n in range(2):put(E+n*4,(0,0x8000,1)[pattern%3],2);put(E+n*4+2,0,2)
  for n in range(3):
   put(T+n*16,0x3f800000);put(T+n*16+4,0);put(T+n*16+8,0x3f000000)
   put(T+n*16+12,0x8000,2);put(T+n*16+14,0x8001 if pattern&1 else 0x8000,2)
  for surface in range(2):
   put(S+surface*8+2,surface*4,2);put(S+surface*8+4,8 if bitmap_mode else 0,1);put(S+surface*8+5,35 if bitmap_mode==3 else 0,1)
   for edge in range(4):
    row=ED+(surface*4+edge)*12;put(row,edge,2);put(row+2,(edge+1)%4,2);put(row+4,surface*4+(edge+1)%4,2);put(row+6,surface*4+(edge+1)%4,2);put(row+10,0xffff if pattern%4!=3 else surface,2)
  for n,coordinates in enumerate(((0,0,0),(0x3f800000,0,0),(0x3f800000,0x3f800000,0),(0,0x3f800000,0))):
   for lane,value in enumerate(coordinates):put(V+n*16+lane*4,value)
  for lane,value in enumerate((point[0],point[1],0)):put(O+lane*4,value)
  for lane,value in enumerate((0,0,0)):put(DELTA+lane*4,value)
  put(POINT,point[0]);put(POINT+4,point[1]);put(B,0 if bitmap_mode==1 else 0xffffffff);put(B+4,0xffffffff)
  c=dict(name=f'bsp_{len(rows)}_{entry:x}_p{pattern}_a{axis}_w{winding}',entry=entry,receiver=0,arguments=[],entry_registers={},banks=[dict(address=0x1000000,before=data.hex()),dict(address=0x44ac8c,before=image.read(0x44ac8c,36).hex()),dict(address=0x46a0e8,before=image.read(0x46a0e8,4).hex())],before_fp=[(0x7f,0x27f,0x37f)[len(rows)%3]|(rounding<<10),((len(rows)%8)<<11)|0x4500|(len(rows)%32),0x1f80|(rounding<<13)],stop_pc=base.physical.STOP,boundary=None)
  if entry==0x2a0230:c.update(receiver=D+0x20,entry_registers=dict(eax=node,edx=POINT))
  elif entry==0x1e6e80:c.update(entry_registers=dict(eax=B if bitmap_mode else 0,edx=D),arguments=[0xffff if bitmap_mode==4 else 64,0,axis,winding,POINT])
  else:c.update(receiver=0,entry_registers=dict(ebx=D,edi=O,esi=DELTA),arguments=[64,B if bitmap_mode else 0,plane,fraction,force])
  rows.append(c)
 points=((0x3e800000,0x3e800000),(0xbf800000,0x3e800000),(0x40000000,0x3e800000),(0x3f800000,0),(0,0),(0x7fc12345,0x3e800000),(0x7f812345,0x3f800000),(0xff800000,0x7f800000),(1,0x80000001))
 for node in(0,1,0x80000000,0xffff8000,0xffff8001,0xffffffff):
  for point in points:
   for rounding in range(4):make(0x2a0230,node=node,point=point,rounding=rounding)
 for axis in range(3):
  for winding in range(2):
   for pattern in(0,3):
    for point in points:make(0x1e6e80,axis=axis,winding=winding,pattern=pattern,point=point)
 for bitmap in range(1,5):make(0x1e6e80,bitmap_mode=bitmap)
 normals=((0,0,0x3f800000),(0x3f800000,0,0),(0,0x3f800000,0),(0x3f000000,0x3f000000,0x3f000000),(0,0,0xbf800000),(0x7fc12345,0x3f800000,0),(0x3f800000,0x7f812345,0),(1,0x80000001,0))
 for pattern in range(4):
  for force in(0,1):
   for normal in normals:
    for point in points[:3]:make(0x1e6cb0,pattern=pattern,normal=normal,point=point,force=force)
 for bitmap in range(1,5):make(0x1e6cb0,pattern=1,bitmap_mode=bitmap)
 for plane in(0xffffffff,1,0x80000000):make(0x1e6cb0,pattern=2,plane=plane)
 for seed in(rows[0],rows[216],rows[220],rows[333],rows[390]):c=dict(seed);c['name']='fault_seed_'+seed['name'];rows.append(c)
 return rows
def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.physical;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_bsp_surface_query.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_bsp_surface_query.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({125,30,97})').replace('auto services=native_tu15_contact_pressure_native_services();','').replace('NativeTu15ContactPressureStats stats;','std::uint32_t result{};').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS);begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];driver=driver.replace('   check(failed==fault_case',RETURN_CHECK+'   check(failed==fault_case');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('tree_side','compare(y,zero).below?0u:1u','compare(y,zero).below?1u:0u'),('polygon_sign','compare(cross,zero).above()','!compare(cross,zero).above()'),('plane_sign','positive!=bool((plane>>15u)&1u)','positive==bool((plane>>15u)&1u)')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","shutil.copyfile(ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp');(project/'types.cpp').write_text('')")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_bsp_surface_query.hpp',ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp'")
 calls='''switch(c.at("entry").get<unsigned>()){
     case 0x2a0230:result=native_tu15_find_bsp_surface_2d(m,c.at("receiver"),c.at("entry_registers").at("eax"),c.at("entry_registers").at("edx"),fp,0);break;
     case 0x1e6e80:result=native_tu15_point_in_bsp_surface(m,c.at("entry_registers").at("edx"),c.at("entry_registers").at("eax"),c.at("arguments")[0],c.at("arguments")[1],c.at("arguments")[2],c.at("arguments")[3].get<unsigned>()!=0,c.at("arguments")[4],fp,0);break;
     case 0x1e6cb0:result=native_tu15_query_bsp_leaf_surface(m,{c.at("entry_registers").at("ebx"),c.at("receiver"),c.at("entry_registers").at("edi"),c.at("entry_registers").at("esi"),c.at("arguments")[0],c.at("arguments")[1],c.at("arguments")[2],c.at("arguments")[3],c.at("arguments")[4].get<unsigned>()!=0},fp,0);break;
    }'''
 check='''if(!fault_case&&!observer_failed){const auto mask=c.at("entry")==0x1e6e80?0xffu:0xffffffffu;check((result&mask)==(c.at("boundary_gp")[0].get<std::uint32_t>()&mask),name+" surface/AL return differs");}'''
 environment=dict(base.__dict__,__file__=__file__,CALLS=calls,RETURN_CHECK=check);exec(text,environment);code=environment['main']();path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());codebook=_Code(base.Xbe(base.NEW_DEFAULT),'.text');sites=set().union(*(set(complete_body(codebook,owner).code)for owner in OWNERS));report.update(physically_executed_sites=len(sites&support.parent.EXECUTED),unexecuted_sites=sorted(sites-support.parent.EXECUTED),prefetch_policy='OriginalPREFETCHT0 unchanged/nonfaulting; no semantic memory read invented. Surrounding architectural pool stride/base reads remain strict.',polygon_return_contract='Sourceparent readsAL; 1E6CB0/2A0230 full32bit surfaceindex checked',scope=__doc__,remaining=['Actual16C620 query parent/BSP/object closure and real input/currentFP composition','Production native routing/typed service composition and visible camera/strictOriginal']);base.save(path,report);return code
if __name__=='__main__':raise SystemExit(main())
