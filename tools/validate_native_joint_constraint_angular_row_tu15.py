#!/usr/bin/env python3
"""Whole3288B0 angular Jacobian row, source scalar/SIMD and alias/fault phases."""
import inspect,json,struct
from pathlib import Path
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x3288b0,)
BACKEND='unchanged-tu15-native-joint-angular-row-v1'
ROOT=Path(__file__).resolve().parents[1]
def metadata(image):
 support.OWNERS=OWNERS;support.BOUNDARIES=()
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[104];')
 text=text.replace("read=bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)","read=False if ins.mnemonic in('fst','fstp')else bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)or ins.mnemonic in('fst','fstp')")
 environment=dict(support.__dict__);exec(text,environment);return environment['metadata'](image)
def cases(image):
 rows=[]
 def make(fixed_a,fixed_b,pattern,precision,rounding,alias=0):
  data=bytearray((n*37+11)&255 for n in range(0x3000));row=0x1000100;input=0x1000200;builder=0x1000300;first=0x1000800;second=0x1000a00;output=0x1001000;schema=0x1002000
  if alias==1:output=row
  if alias==2:output=first+0x30
  if alias==3:schema=builder
  if alias==4:schema=row+0x20
  def put(at,value):struct.pack_into('<I',data,at-0x1000000,value&0xffffffff)
  vectors=[(0x3f800000,0,0,0),(0x3f000000,0xbe800000,0x3e000000,0x3f800000),(0x80000000,0,0x3f800000,0xbf800000),(1,0x80000002,0x00800000,0x80800000),(0x7f7fffff,0xff7fffff,0x3f800000,0),(0x7f800000,0xff800000,0,0x3f800000),(0x7fc12345,0xffc54321,0x3f800000,0),(0x7f812345,0xff854321,0,0x80000000)]
  for n in range(12):put(row+n*4,vectors[(pattern+n//4)%len(vectors)][n%4])
  for body,fixed in((first,fixed_a),(second,fixed_b)):
   data[body-0x1000000+12]=fixed
   for n,value in enumerate((0x3e800000,0x3f000000,0x3f800000,0)):put(body+0x30+n*4,value)
   for axis in range(3):
    for lane in range(4):put(body+0x50+axis*16+lane*4,0x3f800000 if lane==axis else 0x3e000000 if lane==(axis+1)%3 else 0)
  put(input,first);put(input+4,second);put(input+12,(0x3f000000,0xbf800000,0,0x7fc12345)[pattern%4]);put(builder,output);put(builder+4,schema)
  n=len(rows);rows.append(dict(name=f'angular_{n}_fixed{fixed_a}{fixed_b}_p{pattern}_alias{alias}',entry=0x3288b0,receiver=0,arguments=[row,input,builder],banks=[dict(address=0x1000000,before=data.hex()),*[dict(address=p,before=image.read(p,k).hex())for p,k in((0x461960,16),(0x41f644,4),(0x46a0d0,4))]],before_fp=[0x7f|(precision<<8)|(rounding<<10),((n%8)<<11)|0x4500|(n%32),0x1f80|((n%4)<<13)|(0x40 if n&1 else 0)],stop_pc=base.physical.STOP,boundary=None))
 for a in(0,1):
  for b in(0,1):
   for pattern in range(8):
    for rounding in range(4):make(a,b,pattern,2,rounding)
 for precision in(0,3):
  for rounding in range(4):make(0,0,1,precision,rounding)
 for alias in range(1,5):make(0,0,1,2,0,alias)
 for seed in(rows[0],rows[64],rows[96],rows[-4]):c=dict(seed);c['name']='fault_seed_'+seed['name'];rows.append(c)
 return rows
def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.physical;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_joint_constraint_angular_row.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_joint_constraint_angular_row.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({104})').replace('auto services=native_tu15_contact_pressure_native_services();','').replace('NativeTu15ContactPressureStats stats;','').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);','native_tu15_emit_joint_constraint_angular_row(m,c.at("arguments")[0],{c.at("arguments")[1]},{c.at("arguments")[2]},fp,0,observer);');begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('jacobian_sign','axis=xor_bits(axis,mask);','axis=axis;'),('schema_kind','schema,0x4040cu','schema,0x40408u'),('bias_sign','asm volatile("fchs"','asm volatile("fnop"')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  shutil.copyfile(ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","(project/'types.cpp').write_text('')")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_joint_constraint_angular_row.hpp',ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp'")
 environment=dict(base.__dict__,__file__=__file__);exec(text,environment);code=environment['main']();path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());sites=set(complete_body(_Code(base.Xbe(base.NEW_DEFAULT),'.text'),0x3288b0).code);report.update(physically_executed_parent_sites=len(sites&support.parent.EXECUTED),unexecuted_parent_sites=sorted(sites-support.parent.EXECUTED),preserved_x87_depth_physically_tested=0,scope=__doc__,remaining=['HostPOD active-child composition inside native31D930 parent','Actual31D930 input/production routing/live strict original','Nonempty x87/alias/error domains outside declaredprofiles remain open']);base.save(path,report);return code
if __name__=='__main__':raise SystemExit(main())
