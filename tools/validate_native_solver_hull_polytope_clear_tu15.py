#!/usr/bin/env python3
"""Source314870 polytope flags, signed counts and current header rereads."""
import copy,inspect,struct
from pathlib import Path
import validate_native_solver_hull_facet_tu15 as facet
ROOT=Path(__file__).resolve().parents[1]
def cases(image):
 rows=[];base=0x1000000;p=base+0x100
 for first in (0xffffffff,0x80000000,0,1,2,5):
  for second in (0xffffffff,0x80000000,0,1,3,6):
   raw=bytearray((n*13+len(rows))&255 for n in range(0x10000))
   struct.pack_into('<2I',raw,0x2b0,base+0x3000,first)
   struct.pack_into('<2I',raw,0x2200,base+0x4000,second)
   n=len(rows);rows.append(dict(name=f'clear_{first:x}_{second:x}',entry=0x314870,receiver=p,arguments=[],relative=0,banks=[dict(address=base,before=raw.hex())],before_fp=[0x27f,((n%8)<<11)|0x453f,0x1f80],stop_pc=facet.base.physical.STOP,boundary=None))
 for name,field,value,countfield in (
  ('facet_count_alias',0x2b0,p+0x170,0x2b4),
  ('facet_pointer_alias',0x2b0,p+0x16c,0x2b4),
  ('edge_count_alias',0x2200,p+0x20f4,0x2204)):
  c=copy.deepcopy(rows[-1]);raw=bytearray.fromhex(c['banks'][0]['before']);struct.pack_into('<I',raw,field,value);struct.pack_into('<I',raw,countfield,3);c['name']=name;c['banks'][0]['before']=raw.hex();rows.append(c)
 for seed in (rows[0],rows[-1],rows[-2],rows[28]):
  c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def driver_source():
 text=facet.driver_source().replace('halo2/native_tu15_solver_hull_polytope.hpp','halo2/native_tu15_solver_hull_polytope_state.hpp').replace('fixture.at("body_sites")[0]==76','fixture.at("body_sites")[0]==32').replace('NativeTu15SolverHullPolytopeStats','NativeTu15SolverHullPolytopeStateStats')
 old='const auto result=native_tu15_minimum_solver_hull_facet(m,c.at("receiver"),fp,0,&stats);check(result==c.at("source_return"),name+" original facet DATA identity differs");'
 new='native_tu15_clear_solver_hull_polytope_flags(m,c.at("receiver"),observer,&stats);'
 assert text.count(old)==1;return text.replace(old,new)
def main():
 text=inspect.getsource(facet.main).replace('base.OWNERS=(0x3131b0,)','base.OWNERS=(0x314870,)').replace('counts[0]==76','counts[0]==32').replace('complete_body(code,0x3131b0)','complete_body(code,0x314870)').replace('native_tu15_solver_hull_polytope.cpp','native_tu15_solver_hull_polytope_state.cpp').replace('native_tu15_solver_hull_polytope.hpp','native_tu15_solver_hull_polytope_state.hpp')
 text=text.replace("('wrong_stride','cursor+=0x140u','cursor+=0x100u'),('wrong_comparison','&1u)!=0','&1u)==0'),('wrong_identity','minimum=record;','minimum=record+0x10u;')", "('wrong_stride','offset+=0x50u','offset+=0x40u'),('wrong_clear_width','array+offset+0x44u,1u','array+offset+0x44u,4u'),('cached_count','count=word(polytope+0x1b4u);++index','++index')")
 namespace=dict(facet.__dict__,cases=cases,driver_source=driver_source,__doc__=__doc__);exec(text,namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
