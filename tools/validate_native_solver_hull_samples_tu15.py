#!/usr/bin/env python3
"""Source313300 deterministic three-coordinate samples, source recurrence and x87."""
import copy,inspect,struct
from pathlib import Path
import validate_native_solver_hull_facet_tu15 as facet
ROOT=Path(__file__).resolve().parents[1]
def cases(image):
 rows=[]
 bounds=[(0,0x3f800000),(0xbf800000,0x3f800000),(0x3f800000,0x40000000),(0x40000000,0x3f800000),(0,0x80000000),(0x7f800000,0x7f800000),(0x7fc12345,0x3f800000),(0x7f812345,0x7f800000),(1,0x7f7fffff),(0xff800000,0x7f800000)]
 for seed in (0,1,0xffffffff,0x80000000,0x7f123456):
  for low,high in bounds:
   for rounding in range(4):
    for precision in (0,1,2,3):
     n=len(rows);raw=bytes((k*17+n)&255 for k in range(0x1000));rows.append(dict(name=f'samples{n}',entry=0x313300,receiver=0,arguments=[seed,low,high,0x1000800],relative=0,banks=[dict(address=0x1000000,before=raw.hex()),dict(address=0x41d780,before=image.read(0x41d780,12).hex())],before_fp=[0x7f|(precision<<8)|(rounding<<10),((n%8)<<11)|0x453f,0x1f80|(rounding<<13)],stop_pc=facet.base.physical.STOP,boundary=None))
 for seed in (rows[0],rows[24],rows[-1]):
  c=copy.deepcopy(seed);c['arguments'][3]=0x41d780;c['name']+='_scale_alias';rows.append(c)
 for seed in (rows[0],rows[50],rows[-1]):c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def driver_source():
 text=facet.driver_source().replace('fixture.at("body_sites")[0]==76','fixture.at("body_sites")[0]==36')
 old='const auto result=native_tu15_minimum_solver_hull_facet(m,c.at("receiver"),fp,0,&stats);'
 new='const auto&a=c.at("arguments");const auto result=native_tu15_sample_solver_hull_direction(m,a[0],a[1],a[2],a[3],fp,0,observer,&stats);'
 assert text.count(old)==1;return text.replace(old,new)
def main():
 text=inspect.getsource(facet.main).replace('base.OWNERS=(0x3131b0,)','base.OWNERS=(0x313300,)').replace('counts[0]==76','counts[0]==36').replace('physical.SP+4','physical.SP+20').replace('complete_body(code,0x3131b0)','complete_body(code,0x313300)')
 text=text.replace("('wrong_stride','cursor+=0x140u','cursor+=0x100u'),('wrong_comparison','&1u)!=0','&1u)==0'),('wrong_identity','minimum=record;','minimum=record+0x10u;')","('wrong_seed','0x3039u-seed*0x3e39b193u','0x3039u+seed*0x3e39b193u'),('wrong_formula','fmuls %0;fadds %1;fmul %%st(1),%%st(0)','fmuls %0;fmul %%st(1),%%st(0);fadds %1'),('wrong_stride','output+axis*4u','output+axis*8u')")
 namespace=dict(facet.__dict__,cases=cases,driver_source=driver_source,__doc__=__doc__);exec(text,namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
