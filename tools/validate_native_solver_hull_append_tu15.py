#!/usr/bin/env python3
"""Native313290 append, current aliases/ordered packed lanes/fault phases."""
import copy,inspect,struct
from pathlib import Path
import validate_native_moving_shape_process_tu15 as base
ROOT=Path(__file__).resolve().parents[1]
def cases(image):
 rows=[]
 def make(index,first,second,third,alias=0,precision=2,rounding=0):
  lo=0x1000000;data=bytearray((n*13+len(rows)*7)&255 for n in range(0x3000));context=lo+0x100;simplex=lo+0x400;differences=lo+0x1000;pair=lo+0x2000
  if alias==1:pair=differences+(index<<4)
  if alias==2:pair=simplex+(index<<4)
  if alias==3:differences=simplex
  if alias==4:differences=context+0x40
  def put(p,n):struct.pack_into('<I',data,p-lo,n)
  for vector,values in [(pair,first),(pair+16,second),(pair+32,third)]:
   for lane,value in enumerate(values):put(vector+lane*4,value)
  put(context+0x48,simplex);put(context+0x50,differences);put(simplex+0x80,index)
  rows.append(dict(name='append_'+str(len(rows))+'_alias'+str(alias),entry=0x313290,receiver=context,arguments=[pair],relative=0,banks=[dict(address=lo,before=data.hex())],before_fp=[0x7f|(precision<<8)|(rounding<<10),((len(rows)%8)<<11)|0x453f,0x1f80|(rounding<<13)],stop_pc=base.physical.STOP,boundary=None))
 patterns=[((0x3f800000,0x3f000001,0xbf000001,0x3f000000),(0xbe000001,0x3e800001,0x40000001,0xbf000000),(0x3e000001,0xbf000001,0x3f800001,0x3f800000))]
 for word in [0x80000000,1,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345]:patterns.append(((word,0x3e800001,word,word),(0,word,0x3f000000,word),(word,0x3e800001,0xbf800001,word)))
 for index in range(4):
  for values in patterns:
   for rounding in range(4):make(index,*values,rounding=rounding)
 for alias in range(1,5):make(1,*patterns[0],alias=alias)
 for seed in(rows[0],rows[64]):c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def driver_source():
 text=(ROOT/'tests/native_tu15_moving_shape_process_tests.cpp').read_text().replace('#include "halo2/native_tu15_moving_shape_process.hpp"','#include "halo2/native_tu15_solver_hull_support.hpp"');text=text.replace('fixture.at("body_sites")[0]==274','fixture.at("body_sites")[0]==32');begin=text.index('   services.classify_shape=');end=text.index('   const auto cw=',begin);text=text[:begin]+text[end:];old='native_tu15_process_moving_shape_contact(m,{c.at("receiver")},{c.at("arguments")[0]},{c.at("arguments")[1]},{c.at("arguments")[2]},c.at("arguments")[3],{c.at("relative")},fp,0,services,observer,&stats);';new='native_tu15_append_solver_hull_vertex(m,c.at("receiver"),c.at("arguments")[0],fp,0,observer,&stats);';assert old in text;text=text.replace(old,new).replace('NativeTu15MovingShapeProcessServices services;','').replace('NativeTu15MovingShapeProcessStats stats;','NativeTu15SolverHullSupportStats stats;');return text
def main():
 base.OWNERS=(0x313290,);base.BOUNDARIES=();base.cases=cases
 meta=inspect.getsource(base.metadata).replace('counts[0]==274','counts==[32]');env=dict(base.__dict__);exec(meta,env);base.metadata=env['metadata'];source=inspect.getsource(base.main);begin=source.index('  def boundary(c,actual):');end=source.index('  physical.boundary=boundary',begin);source=source[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+source[end:]
 source=source.replace("source=ROOT/'src/physics/native_tu15_moving_shape_process.cpp'","source=ROOT/'src/physics/native_tu15_solver_hull_support.cpp'").replace("shutil.copyfile(ROOT/'tests/native_tu15_moving_shape_process_tests.cpp',project/'driver.cpp')","(project/'driver.cpp').write_text(driver_source())")
 begin=source.index('  for name,needle,replacement in(');end=source.index('\n   assert needle in text;',begin);source=source[:begin]+'''  for name,needle,replacement in(('wrong_difference','Vector difference=first;','Vector difference=second;'),('wrong_second','point_pair+0x10u','point_pair+0x20u'),('wrong_count','count+1u','count+2u')):'''+source[end:]
 source=source.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp')","shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp');shutil.copyfile(ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')")
 source=source.replace("units=('native_tu15_contact_query.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_activation_services.cpp','native_tu15_inverse_geometry.cpp','native_tu15_geometry.cpp')","units=()")
 source=source.replace("source,ROOT/'include/halo2/native_tu15_moving_shape_process.hpp'","source,ROOT/'include/halo2/native_tu15_solver_hull_support.hpp'")
 source=source.replace("for e in seed[category]:","for e in seed[category]:\n      if category=='writes'and e['instruction']==0x3132e8:continue")
 source=source.replace('negative_controls=list(variants),','negative_controls=list(variants),explicit_oracle_gaps=[dict(instruction=0x3132e8,phase="RMW write-fault: physical preinstruction stop omits indivisible count read; no matching prefix claimed")],')
 source=source.replace('complete_body(code,0x2e5320)','complete_body(code,0x313290)').replace("remaining=['Returned cache and2F8B60 child closure, parent epilogue'","scope=__doc__,remaining=['313390/313560/313BB0 fullsemantic closure'")
 environment=dict(base.__dict__,driver_source=driver_source,__doc__=__doc__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
