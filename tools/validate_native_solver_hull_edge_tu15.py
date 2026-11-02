#!/usr/bin/env python3
"""Source313470 opposite-axis support selection with real native shape providers."""
import copy,inspect,struct
from pathlib import Path
import validate_native_solver_hull_line_tu15 as line
ROOT=Path(__file__).resolve().parents[1]
def cases(image):
 rows=[]
 for seed in line.cases(image):
  if seed['name'].startswith('fault_seed'):continue
  c=copy.deepcopy(seed);raw=bytearray.fromhex(c['banks'][0]['before']);direction=0x1001800;origin=0x1001900;output=0x1001a00
  struct.pack_into('<4I',raw,0x1900,0x3f800001,0xbe800001,0x40000001,0x7f812345);c['banks'][0]['before']=raw.hex();c['entry']=0x313470;c['arguments']=[direction,origin,output];c['name']='edge_'+str(len(rows));rows.append(c)
 for source in (rows[0],rows[160],rows[-1]):
  for alias in (0,1):
   c=copy.deepcopy(source);c['name']+='alias'+str(alias)
   if alias==0:c['arguments'][2]=c['arguments'][0]
   else:c['arguments'][1]=c['arguments'][2]
   rows.append(c)
 for source in (rows[0],rows[len(rows)//2],rows[-1]):c=copy.deepcopy(source);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def driver_source():
 text=line.driver_source().replace('halo2/native_tu15_solver_hull_line.hpp','halo2/native_tu15_solver_hull_edge.hpp').replace('fixture.at("body_sites")[0]==77','fixture.at("body_sites")[0]==80')
 old='native_tu15_extend_solver_hull_line(m,c.at("receiver"),{PRIVATE+0x200u,PRIVATE+0x240u,PRIVATE+0x300u,PRIVATE+0x310u,PRIVATE+0x320u,PRIVATE+0x324u},fp,0,supports,filtered,&stats);'
 new='native_tu15_select_solver_hull_edge(m,c.at("receiver"),c.at("arguments")[0],c.at("arguments")[1],c.at("arguments")[2],{PRIVATE+0x200u,PRIVATE+0x300u,PRIVATE+0x310u,PRIVATE+0x320u,PRIVATE+0x324u},fp,0,supports,filtered,&stats);'
 assert text.count(old)==1;return text.replace(old,new).replace('NativeTu15SolverHullLineStats stats;','NativeTu15SolverHullEdgeStats stats;')
def main():
 text=inspect.getsource(line.main)
 text=text.replace('base.OWNERS=(0x3136a0,0x313390,0x313290,0x2e8d20,0x2e7d90)','base.OWNERS=(0x313470,0x313390,0x2e8d20,0x2e7d90)')
 text=text.replace('counts[0]==77','counts[0]==80').replace('native_tu15_solver_hull_line','native_tu15_solver_hull_edge')
 text=text.replace('physical.SP+4','physical.SP+16').replace("0x3136a0<=e[\\'instruction\\']<0x313785","0x313470<=e[\\'instruction\\']<0x31355e")
 # line.main contains a transformation of the upstream single-vertex script;
 # replace the resulting range and body identity, preserving all source order.
 text=text.replace("'0x3136a0<=e[\\'instruction\\']<0x313785'","'0x313470<=e[\\'instruction\\']<0x31355e'")
 text=text.replace("complete_body(code,0x3136a0)","complete_body(code,0x313470)")
 text=text.replace("('wrong_distance','subtract(subtract(first,transformed),vertex)','subtract(subtract(transformed,first),vertex)'),('wrong_direction','xor_bits(current_direction,sign)','current_direction'),('wrong_choice','second=(relation&0x100u)!=0','second=(relation&0x100u)==0')","('wrong_direction','xor_bits(axis,sign)','axis'),('wrong_choice','if(!(relation&0x100u)){','if(relation&0x100u){'),('wrong_scalar_output','output+0x30u,value','output+0x34u,value')")
 text=text.replace("'3132E8 child RMW write-fault remains separately open'","'No child RMW occurs in this selected helper; private-address faults remain outside this public DATA domain'")
 namespace=dict(line.__dict__,cases=cases,driver_source=driver_source,__doc__=__doc__);exec(text,namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
