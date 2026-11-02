#!/usr/bin/env python3
"""Actual18A2A0 native direction reorientation, whole and fault/alias phases."""
import inspect,json,struct
from pathlib import Path
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x18a2a0,)
BACKEND='unchanged-tu15-native-look-reorientation-v1'
ROOT=Path(__file__).resolve().parents[1]
def metadata(image):
 support.OWNERS=OWNERS;support.BOUNDARIES=()
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[30];')
 text=text.replace("read=bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)","read=False if ins.mnemonic in('fst','fstp')else bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)or ins.mnemonic in('fst','fstp')")
 environment=dict(support.__dict__);exec(text,environment);return environment['metadata'](image)
def cases(image):
 rows=[];vectors=[(0x3f800000,0,0),(0xbf800000,0,0),(0,0x3f800000,0),(0,0xbf800000,0),(0,0,0),(0x80000000,0x80000000,0x80000000),(0,0,0x3f800000),(0,0,0xbf800000),(0x3f000000,0x3e800000,0xbe800000),(1,2,3),(0x00800000,1,0x80800000),(0x7f7fffff,0x7f7fffff,0x7f7fffff),(0x7f800000,0x3f800000,0xff800000),(0x7fc12345,0x3f800000,0),(0x3f800000,0xffc54321,0),(0x7f812345,0x3f800000,0xff854321)]
 def make(vector,control,rounding,index=0,alias=0):
  controls=0x1001000;record=(controls+index*0x94+0x14)&0xffffffff;forward=0x1000200
  if alias:forward=record+(0xc,0x10,0x14)[alias-1]
  raw=bytearray((n*37+11)&255 for n in range(0x2000))
  for lane,value in enumerate(vector):struct.pack_into('<I',raw,forward-0x1000000+lane*4,value)
  n=len(rows);rows.append(dict(name=f'reorient_{n}_index{index:x}_alias{alias}',entry=0x18a2a0,receiver=index,forward=forward,entry_registers={'eax':forward},arguments=[],banks=[dict(address=0x1000000,before=raw.hex()),dict(address=0x4f9b54,before=struct.pack('<I',controls).hex()),dict(address=0x44923c,before=image.read(0x44923c,4).hex())],before_fp=[0x7f|(control<<8)|(rounding<<10),((n%8)<<11)|0x4500|(n%32),0x1f80|((n%4)<<13)|(0x40 if n&1 else 0)],stop_pc=base.physical.STOP,boundary=None))
 for vector in vectors:
  for precision in(0,2,3):
   for rounding in range(4):make(vector,precision,rounding)
 for index in(1,0xffffffff,0x80000000):
  for alias in range(4):make(vectors[8],2,0,index,alias)
 for alias in(1,2,3):make(vectors[3],2,0,0,alias)
 for seed in(rows[0],rows[36],rows[-1]):c=dict(seed);c['name']='fault_seed_'+seed['name'];rows.append(c)
 return rows
def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.physical;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_look_reorientation.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_look_reorientation.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({30})').replace('auto services=native_tu15_contact_pressure_native_services();','').replace('NativeTu15ContactPressureStats stats;','NativeTu15LookReorientationStats stats;').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);','native_tu15_reorient_look_controller(m,c.at("receiver"),c.at("forward"),fp,0,observer,&stats);');begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('yaw_wrap','if(negative){','if(!negative){'),('pitch_sign','m.pop_float(0x18a2ddu','asm volatile("fchs":::"memory","st");m.pop_float(0x18a2ddu'),('record_stride','index*0x94u','index*0x98u')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  shutil.copyfile(ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","(project/'types.cpp').write_text('')")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_look_reorientation.hpp',ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp'")
 environment=dict(base.__dict__,__file__=__file__);exec(text,environment);code=environment['main']();path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());sites=set(complete_body(_Code(base.Xbe(base.NEW_DEFAULT),'.text'),0x18a2a0).code);report.update(physically_executed_parent_sites=len(sites&support.parent.EXECUTED),unexecuted_parent_sites=sorted(sites-support.parent.EXECUTED),preserved_x87_depth_physically_tested=0,scope=__doc__,remaining=['SAME FP-field-only binding and actual reorientation input/caller contract','Host production callsite wiring+counter transport+visible multiplayer/strict original','Full/mutating observers refused by binding; unmasked FP/overfull stack domains not admitted']);base.save(path,report);return code
if __name__=='__main__':raise SystemExit(main())
