#!/usr/bin/env python3
"""Source3147D0 packed projection, current bound reads and x87 clipping."""
import copy,inspect,struct
from pathlib import Path
import validate_native_solver_hull_polytope_clear_tu15 as clear
import validate_native_solver_hull_facet_tu15 as facet
ROOT=Path(__file__).resolve().parents[1]
def cases(image):
 rows=[];base=0x1000000
 bounds=[(0xbf800000,0x3f800000),(0,0x40000000),(0x40000000,0x3f800000),(0,0),(0x7fc12345,0x3f800000),(0x7f812345,0x3f800000),(0x3f800000,0x7f812345)]
 vectors=[(0,0,0,0),(0x3f800000,0x3e000001,0xbe800001,0x3f000000),(0x40000001,0xc0000001,0x3f800001,0x7f812345),(1,0x80000000,0x7f800000,0)]
 for lower,upper in bounds:
  for point in vectors:
   for rounding in range(4):
    for precision in (0,1,2,3):
     n=len(rows);raw=bytearray((i*19+n)&255 for i in range(0x1000))
     struct.pack_into('<4I',raw,0x100,0x3f800001,0xbf000001,0x3e800001,0x3f800000);struct.pack_into('<I',raw,0x114,base+0x200);struct.pack_into('<4I',raw,0x200,0x3e000001,0xbe800001,0x3f000001,0)
     struct.pack_into('<4I',raw,0x300,*point);struct.pack_into('<2I',raw,0x400,lower,upper)
     rows.append(dict(name=f'clip{n}',entry=0x3147d0,receiver=base+0x100,arguments=[base+0x300,base+0x400,base+0x404],relative=0,banks=[dict(address=base,before=raw.hex()),dict(address=0x41d7a0,before=image.read(0x41d7a0,4).hex())],before_fp=[0x7f|(precision<<8)|(rounding<<10),((n%8)<<11)|0x453f,0x1f80|(rounding<<13)],stop_pc=facet.base.physical.STOP,boundary=None))
 updates=[]
 for rounding in range(4):
  for precision in (0,1,2,3):
   c=copy.deepcopy(rows[0]);c['name']=f'clip_lower_update_{rounding}_{precision}';raw=bytearray.fromhex(c['banks'][0]['before']);raw[0x300:0x310]=raw[0x200:0x210];struct.pack_into('<2I',raw,0x400,0,0x3f800000);c['banks'][0]['before']=raw.hex();c['before_fp']=[0x7f|(precision<<8)|(rounding<<10),0x453f,0x1f80|(rounding<<13)];updates.append(c)
 rows+=updates
 for seed in (rows[0],rows[127],rows[256],rows[-1]):
  for position in (0x100,0x300,0x400,0x41d7a0-base):
   c=copy.deepcopy(seed);c['name']+=f'_lower_alias_{position:x}';c['arguments'][1]=base+position;rows.append(c)
  c=copy.deepcopy(seed);c['name']+='_bound_alias';c['arguments'][2]=c['arguments'][1];rows.append(c)
 for seed in (rows[0],rows[144],rows[256],rows[-1],updates[2]):c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def driver_source():
 text=facet.driver_source().replace('halo2/native_tu15_solver_hull_polytope.hpp','halo2/native_tu15_solver_hull_polytope_state.hpp').replace('fixture.at("body_sites")[0]==76','fixture.at("body_sites")[0]==56').replace('NativeTu15SolverHullPolytopeStats','NativeTu15SolverHullPolytopeStateStats')
 old='const auto result=native_tu15_minimum_solver_hull_facet(m,c.at("receiver"),fp,0,&stats);'
 new='const auto&a=c.at("arguments");const auto result=native_tu15_clip_solver_hull_facet(m,c.at("receiver"),a[0],a[1],a[2],fp,0,observer,&stats);'
 assert text.count(old)==1;return text.replace(old,new)
def main():
 text=inspect.getsource(facet.main).replace('base.OWNERS=(0x3131b0,)','base.OWNERS=(0x3147d0,)').replace('counts[0]==76','counts[0]==56').replace('complete_body(code,0x3131b0)','complete_body(code,0x3147d0)').replace('native_tu15_solver_hull_polytope.cpp','native_tu15_solver_hull_polytope_state.cpp').replace('native_tu15_solver_hull_polytope.hpp','native_tu15_solver_hull_polytope_state.hpp').replace('physical.SP+4','physical.SP+16')
 text=text.replace("('wrong_stride','cursor+=0x140u','cursor+=0x100u'),('wrong_comparison','&1u)!=0','&1u)==0'),('wrong_identity','minimum=record;','minimum=record+0x10u;')", "('wrong_dot','sum_three(product)','scalar_bits(product)'),('wrong_upper_test','&1u)!=0','&1u)==0'),('wrong_result','result=2u','result=1u')")
 namespace=dict(facet.__dict__,cases=cases,driver_source=driver_source,__doc__=__doc__);exec(text,namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
