#!/usr/bin/env python3
"""Source3131B0 current facet minimum; exact native hardware x87 and DATA identity."""
import copy,inspect,struct
from pathlib import Path
import validate_native_moving_shape_process_tu15 as base
ROOT=Path(__file__).resolve().parents[1]
def cases(image):
 rows=[]
 for count in (0,1,2,3,4,5,6,7,8,9,10,17):
  for pattern in ('descending','ascending','ties','nan','snan','infinity'):
   for rounding in range(4):
    for precision in (0,1,2,3):
     raw=bytearray((n*17+len(rows))&255 for n in range(0x3000));struct.pack_into('<2I',raw,0x2b0,0x1001000,count)
     for i in range(max(count,1)):
      value=struct.unpack('<I',struct.pack('<f',float(count-i if pattern=='descending'else i)))[0]
      if pattern=='ties':value=0x80000000 if i%2 else 0
      if pattern=='nan'and i%3==1:value=0x7fc12345
      if pattern=='snan'and i%3==0:value=0x7f812345
      if pattern=='infinity':value=0x7f800000 if i%2 else 0xff800000
      struct.pack_into('<I',raw,0x1010+i*80,value)
     n=len(rows);rows.append(dict(name=f'facet{count}_{pattern}_{rounding}_{precision}',entry=0x3131b0,receiver=0x1000100,arguments=[],relative=0,banks=[dict(address=0x1000000,before=raw.hex())],before_fp=[0x7f|(precision<<8)|(rounding<<10),((n%8)<<11)|0x453f,0x1f80|(rounding<<13)],stop_pc=base.physical.STOP,boundary=None))
 for seed in (rows[0],rows[650],rows[-1]):c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def driver_source():
 text=(ROOT/'tests/native_tu15_moving_shape_process_tests.cpp').read_text().replace('halo2/native_tu15_moving_shape_process.hpp','halo2/native_tu15_solver_hull_polytope.hpp').replace('fixture.at("body_sites")[0]==274','fixture.at("body_sites")[0]==76')
 begin=text.index('NativeTu15MovingShapeProcessServices services;');end=text.index('   const auto cw=',begin);text=text[:begin]+'bool boundary_hit{};\n'+text[end:]
 text=text.replace('NativeTu15MovingShapeProcessStats stats;','NativeTu15SolverHullPolytopeStats stats;')
 old='native_tu15_process_moving_shape_contact(m,{c.at("receiver")},{c.at("arguments")[0]},{c.at("arguments")[1]},{c.at("arguments")[2]},c.at("arguments")[3],{c.at("relative")},fp,0,services,observer,&stats);'
 new='const auto result=native_tu15_minimum_solver_hull_facet(m,c.at("receiver"),fp,0,&stats);check(result==c.at("source_return"),name+" original facet DATA identity differs");'
 assert text.count(old)==1;return text.replace(old,new)
def main():
 base.OWNERS=(0x3131b0,);base.BOUNDARIES=();base.cases=cases
 meta=inspect.getsource(base.metadata).replace('counts[0]==274','counts[0]==76');namespace=dict(base.__dict__);exec(meta,namespace);base.metadata=namespace['metadata']
 source=inspect.getsource(base.main);begin=source.index('  def boundary(c,actual):');end=source.index('  physical.boundary=boundary',begin)
 source=source[:begin]+'''  def boundary(c,actual):
   assert actual['actual_stop']==physical.STOP and actual['boundary_gp'][4]==physical.SP+4;c['source_return']=actual['boundary_gp'][0];return None
'''+source[end:]
 source=source.replace("source=ROOT/'src/physics/native_tu15_moving_shape_process.cpp'","source=ROOT/'src/physics/native_tu15_solver_hull_polytope.cpp'").replace("shutil.copyfile(ROOT/'tests/native_tu15_moving_shape_process_tests.cpp',project/'driver.cpp')","(project/'driver.cpp').write_text(driver_source())")
 begin=source.index('  for name,needle,replacement in(');end=source.index('\n   assert needle in text;',begin)
 source=source[:begin]+"  for name,needle,replacement in(('wrong_stride','cursor+=0x140u','cursor+=0x100u'),('wrong_comparison','&1u)!=0','&1u)==0'),('wrong_identity','minimum=record;','minimum=record+0x10u;')):"+source[end:]
 source=source.replace("units=('native_tu15_contact_query.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_activation_services.cpp','native_tu15_inverse_geometry.cpp','native_tu15_geometry.cpp')","units=()")
 source=source.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp')","shutil.copyfile(ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')")
 source=source.replace("source,ROOT/'include/halo2/native_tu15_moving_shape_process.hpp'","source,ROOT/'include/halo2/native_tu15_solver_hull_polytope.hpp'").replace('complete_body(code,0x2e5320)','complete_body(code,0x3131b0)')
 source=source.replace('remaining=[\'Returned cache and2F8B60 child closure, parent epilogue\'','scope=__doc__,full_returns=sum(not c.get("fault")and not c.get("observer_fault")for c in rows),remaining=[\'Fullpolytope/hullparentcomposition\'')
 namespace=dict(base.__dict__,cases=cases,driver_source=driver_source,__doc__=__doc__);exec(source,namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
