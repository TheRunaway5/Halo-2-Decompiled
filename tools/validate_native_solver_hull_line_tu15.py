#!/usr/bin/env python3
"""Source3136A0 line extension with native313390/313290 and real support shapes."""
import copy,inspect,struct
from pathlib import Path
import validate_native_solver_hull_single_vertex_tu15 as parent
base=parent.base;pair=parent.pair;ROOT=Path(__file__).resolve().parents[1]
def cases(image):
 rows=[]
 for index,seed in enumerate(parent.initialization.cases(image)):
  if seed['name'].startswith('fault_seed')or index%3:continue
  for value in (0,0x3f800001,0x7fc12345,0x7f812345):
   c=copy.deepcopy(seed);raw=bytearray.fromhex(c['banks'][0]['before']);struct.pack_into('<I',raw,0x1580,1);struct.pack_into('<4I',raw,0x1800,value,0x3e800001,0xbf800001,value);c['banks'][0]['before']=raw.hex();c['entry']=0x3136a0;c['arguments']=[];c['name']='line_'+str(len(rows));rows.append(c)
 for seed in(rows[0],rows[len(rows)//2],rows[-1]):c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def driver_source():
 text=pair.driver_source().replace('halo2/native_tu15_solver_hull_support.hpp','halo2/native_tu15_solver_hull_line.hpp').replace('fixture.at("body_sites")[0]==75','fixture.at("body_sites")[0]==77')
 old='native_tu15_solver_hull_support_pair(m,c.at("receiver"),c.at("arguments")[0],c.at("arguments")[1],PRIVATE,fp,0,supports,filtered,&stats);'
 new='native_tu15_extend_solver_hull_line(m,c.at("receiver"),{PRIVATE+0x200u,PRIVATE+0x240u,PRIVATE+0x300u,PRIVATE+0x310u,PRIVATE+0x320u,PRIVATE+0x324u},fp,0,supports,filtered,&stats);'
 assert text.count(old)==1;text=text.replace(old,new).replace('NativeTu15SolverHullSupportStats stats;','NativeTu15SolverHullLineStats stats;')
 text=text.replace('if(++support_calls==2)','if(++support_calls%2==0)').replace('c.at("support_directions").size()==1','c.at("support_directions").size()>=support_calls/2').replace('c.at("support_directions")[0].get<std::string>()','c.at("support_directions")[support_calls/2-1].get<std::string>()')
 return text
def main():
 text=inspect.getsource(parent.main)
 text=text.replace('base.OWNERS=(0x313bb0,0x32a660,0x313560,0x313390,0x313290,0x2e8d20,0x2e7d90)','base.OWNERS=(0x3136a0,0x313390,0x313290,0x2e8d20,0x2e7d90)')
 text=text.replace('base.BOUNDARIES=(0x3136a0,)','base.BOUNDARIES=()')
 text=text.replace('counts[0]==742','counts[0]==77').replace('native_tu15_solver_hull_single_vertex','native_tu15_solver_hull_line')
 text=text.replace("assert actual['boundary_gp'][4]==physical.SP+8;assert actual['boundary_gp'][0]in(0,2)","assert actual['boundary_gp'][4]==physical.SP+4")
 text=text.replace('0x313bb0<=e[\'instruction\']<0x3146f5','0x3136a0<=e[\'instruction\']<0x313785').replace('complete_body(code,0x313bb0)','complete_body(code,0x3136a0)')
 text=text.replace("('wrong_decrement','current_count-1u','current_count+1u'),('wrong_result','return 0u','return 1u'),('wrong_extension','if(extend){','if(!extend){')","('wrong_distance','subtract(subtract(first,transformed),vertex)','subtract(subtract(transformed,first),vertex)'),('wrong_direction','xor_bits(current_direction,sign)','current_direction'),('wrong_choice','second=(relation&0x100u)!=0','second=(relation&0x100u)==0')")
 text=text.replace("'313DAC atomicDEC write-fault prefix'","'3132E8 child RMW write-fault remains separately open'")
 text=text.replace("'Remaining initial counts/line/triangle/tetrahedral/polytope hull closure'","'Remaining triangle/tetrahedral/polytope hull parent closure'")
 namespace=dict(parent.__dict__,cases=cases,driver_source=driver_source,__doc__=__doc__);exec(text,namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
