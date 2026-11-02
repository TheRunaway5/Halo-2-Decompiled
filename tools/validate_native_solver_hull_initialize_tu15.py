#!/usr/bin/env python3
"""Native313560→313390→313290 with actual sphere/capsule, no support outcomes."""
import copy,inspect,struct
from pathlib import Path
import validate_native_solver_hull_support_pair_tu15 as pair
ROOT=Path(__file__).resolve().parents[1]
def cases(image):
 rows=[]
 for seed in pair.cases(image):
  if seed['name'].startswith('fault_seed'):continue
  for position in [0,1,2]:
   c=copy.deepcopy(seed);raw=bytearray.fromhex(c['banks'][0]['before']);context=c['receiver'];lo=0x1000000;simplex=lo+0x1500;differences=lo+0x1800
   def put(p,n):struct.pack_into('<I',raw,p-lo,n)
   put(context+0x48,simplex);put(context+0x50,differences);put(simplex+0x80,0)
   if position==0:
    for n in range(4):put(context+0x30+n*4,0)
   if position==2:put(context+0x30,1)
   c['banks'][0]['before']=raw.hex();c['banks']+=[dict(address=p,before=image.read(p,4).hex())for p in [0x46a180,0x46a454]];c['arguments']=[lo+0xa00,lo+0xb00];c['entry']=0x313560;c['name']='initialize_'+str(len(rows));rows.append(c)
 for seed in [rows[0],rows[100],rows[-1]]:c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def driver_source():
 text=pair.driver_source();text=text.replace('fixture.at("body_sites")[0]==75','fixture.at("body_sites")[0]==89');old='native_tu15_solver_hull_support_pair(m,c.at("receiver"),c.at("arguments")[0],c.at("arguments")[1],PRIVATE,fp,0,supports,filtered,&stats);';new='check(native_tu15_initialize_solver_hull(m,c.at("receiver"),c.at("arguments")[0],c.at("arguments")[1],{PRIVATE,PRIVATE+0x10,PRIVATE+0x30,PRIVATE+0x60,PRIVATE+0x20},fp,0,supports,filtered,&stats)==c.at("arguments")[0],name+" numeric output flag identity differs");';assert old in text;return text.replace(old,new)
def main():
 source=inspect.getsource(pair.main).replace('base.observer_source=pair_observer_source','base.observer_source=pair_observer_source')
 source=source.replace("base.OWNERS=(0x313390,0x2e8d20,0x2e7d90)","base.OWNERS=(0x313560,0x313390,0x313290,0x2e8d20,0x2e7d90)")
 source=source.replace("'counts[0]==75'","'counts[0]==89'").replace("'complete_body(code,0x313390)'","'complete_body(code,0x313560)'")
 source=source.replace('0x313390<=e[\'instruction\']<0x313470','0x313560<=e[\'instruction\']<0x31369b')
 source=source.replace("('wrong_transpose_order','add(add(xterm,yterm),zterm)','add(add(zterm,yterm),xterm)'),('wrong_final_translation','add(zterm,translation)','add(zterm,zterm)'),('wrong_second_output','point_pair+0x10u);','point_pair+0x20u);')","('wrong_direction','xor_bits(translation,sign)','translation'),('wrong_flag','output_flag,1,observe','output_flag,0,observe'),('wrong_plane_coefficient','output_plane+0x24u,0x3f000000u','output_plane+0x24u,0x3f800000u')")
 environment=dict(pair.__dict__,cases=cases,driver_source=driver_source,__doc__=__doc__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
