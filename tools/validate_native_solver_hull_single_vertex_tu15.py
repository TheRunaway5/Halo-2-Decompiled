#!/usr/bin/env python3
"""Source313BB0 one-vertex full returns and genuine native3136A0 frontiers.

Compiler alloca/SEH state stays only in the unchanged physical original. The
native algorithm owns ordinary arithmetic/Boolean DATA, with typed refusals.
"""
import copy,inspect,struct
from pathlib import Path
import validate_native_solver_hull_initialize_tu15 as initialization
pair=initialization.pair;base=pair.base;ROOT=Path(__file__).resolve().parents[1]
def cases(image):
 rows=[]
 for seed in initialization.cases(image):
  if seed['name'].startswith('fault_seed'):continue
  if int(seed['name'].rsplit('_',1)[1])%3!=0:continue
  for vertex in [0,0x3f800000,0x7f800000,0x7fc12345]:
   c=copy.deepcopy(seed);raw=bytearray.fromhex(c['banks'][0]['before']);struct.pack_into('<I',raw,0x1580,1);struct.pack_into('<4I',raw,0x1800,vertex,0,0,0);c['banks'][0]['before']=raw.hex();c['entry']=0x313bb0;c['arguments']=[0x1000b00];c['name']='single_vertex_'+str(len(rows));rows.append(c)
 for seed in [rows[0],rows[40],rows[-1]]:c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def driver_source():
 text=pair.driver_source().replace('#include "halo2/native_tu15_solver_hull_support.hpp"','#include "halo2/native_tu15_solver_hull_single_vertex.hpp"');text=text.replace('fixture.at("body_sites")[0]==75','fixture.at("body_sites")[0]==742')
 old='native_tu15_solver_hull_support_pair(m,c.at("receiver"),c.at("arguments")[0],c.at("arguments")[1],PRIVATE,fp,0,supports,filtered,&stats);'
 new='const auto result=native_tu15_complete_single_vertex_solver_hull(m,c.at("receiver"),c.at("arguments")[0],{PRIVATE+0x200u,PRIVATE+0x204u,{PRIVATE,PRIVATE+0x10u,PRIVATE+0x30u,PRIVATE+0x60u,PRIVATE+0x20u}},fp,0,supports,filtered,&stats);check(c.at("source_return")==result,name+" source313BB0 integer result differs");'
 assert old in text;text=text.replace(old,new).replace('NativeTu15SolverHullSupportStats stats;','NativeTu15SolverHullSingleVertexStats stats;')
 text=text.replace('if(std::string(e.what())!="actual-island-owner-boundary")throw;','if(std::string(e.what())=="Native hull requires remaining line-extension owner3136A0")boundary_hit=true;else if(std::string(e.what())!="actual-island-owner-boundary")throw;')
 return text
def main():
 base.OWNERS=(0x313bb0,0x32a660,0x313560,0x313390,0x313290,0x2e8d20,0x2e7d90);base.BOUNDARIES=(0x3136a0,);base.cases=cases
 base.observer_source=pair.pair_observer_source
 meta=inspect.getsource(base.metadata).replace('counts[0]==274','counts[0]==742')
 meta=meta.replace('    assert not op.mem.segment;base,index=',"    if op.mem.segment:\n     assert ins.reg_name(op.mem.segment)=='fs';continue\n    base,index=")
 env=dict(base.__dict__);exec(meta,env);base.metadata=env['metadata']
 source=inspect.getsource(base.main);begin=source.index('  def boundary(c,actual):');end=source.index('  physical.boundary=boundary',begin)
 boundary="""  def boundary(c,actual):
   if actual['actual_stop']==physical.STOP:
    assert actual['boundary_gp'][4]==physical.SP+8;assert actual['boundary_gp'][0]in(0,2);c['source_return']=actual['boundary_gp'][0];return None
   assert actual['actual_stop']==0x3136a0 and actual['boundary_gp'][1]==c['receiver'];c['source_return']=None;return dict(owner=0x3136a0)
"""
 source=source[:begin]+boundary+source[end:]
 source=source.replace("source=ROOT/'src/physics/native_tu15_moving_shape_process.cpp'","source=ROOT/'src/physics/native_tu15_solver_hull_single_vertex.cpp'").replace("shutil.copyfile(ROOT/'tests/native_tu15_moving_shape_process_tests.cpp',project/'driver.cpp')","(project/'driver.cpp').write_text(driver_source())")
 begin=source.index('  for name,needle,replacement in(');end=source.index('\n   assert needle in text;',begin)
 source=source[:begin]+"  for name,needle,replacement in(('wrong_decrement','current_count-1u','current_count+1u'),('wrong_result','return 0u','return 1u'),('wrong_extension','if(extend){','if(!extend){')):"+source[end:]
 source=source.replace("units=('native_tu15_contact_query.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_activation_services.cpp','native_tu15_inverse_geometry.cpp','native_tu15_geometry.cpp')","units=('native_tu15_contact_shape.cpp','native_tu15_solver_hull_support.cpp')")
 source=source.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp')","shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp');shutil.copyfile(ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')")
 source=source.replace("source,ROOT/'include/halo2/native_tu15_moving_shape_process.hpp'","source,ROOT/'include/halo2/native_tu15_solver_hull_single_vertex.hpp'")
 source=source.replace("      if e['instruction']in seen:continue","      if not(0x313bb0<=e['instruction']<0x3146f5)or(category=='writes'and e['instruction']==0x313dac):continue\n      if e['instruction']in seen:continue")
 source=source.replace("for n,e in enumerate(seed['writes']):","for n,e in enumerate(seed['writes']):\n     if not(0x313bb0<=e['instruction']<0x3146f5):continue")
 source=source.replace('complete_body(code,0x2e5320)','complete_body(code,0x313bb0)')
 source=source.replace("genuine_cache_entries=sum(c['boundary']is not None and c['boundary']['owner']in(0x309060,0x309150)for c in rows)","genuine_cache_entries=0")
 source=source.replace("remaining=['Returned cache and2F8B60 child closure, parent epilogue'","scope=__doc__,full_returns=sum(c['actual_stop']==physical.STOP and not c.get('fault')and not c.get('observer_fault')for c in rows),explicit_oracle_gaps=['313DAC atomicDEC write-fault prefix'],remaining=['Remaining initial counts/line/triangle/tetrahedral/polytope hull closure'")
 environment=dict(base.__dict__,driver_source=driver_source,__doc__=__doc__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
