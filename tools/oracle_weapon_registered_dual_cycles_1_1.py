#!/usr/bin/env python3
"""Original dual cycles with both exact weapon datums in local FPS view0.

Original167D32/10A140/EFCC0 registration, unit hand timers, D2940 error blend,
input/update and projectile postpasses execute unchanged. Raw unit/player/
object/time ABI storage and inherited unavailable animation/effect/physics
services remain explicit. The actual E7EF0 unit action dispatcher executes;
this does not execute complete unit animation/action arbitration or rendering.
"""
import argparse,hashlib,json,sys
from pathlib import Path
import oracle_weapon_dual_cycles_1_1 as parent
from oracle_smg_firing_1_1 import FPS,WI,UNIT,ROOT

Base=parent.DualTrace
class RegisteredDualTrace(Base):
 def __init__(self,*args,**kwargs):
  super().__init__(*args,**kwargs)
  self.w(FPS+0x10,WI);self.w(FPS+0x1020,parent.OI)
 def pair_state(self):
  states=super().pair_state()
  for state in states:state['unit_hand_timers']=[self.u(UNIT+0x1fa,1),self.u(UNIT+0x1fb,1)]
  return states

base_export=parent.export_rows
def export_rows(report,name,representation,pattern):
 result=[]
 for line,row in zip(base_export(report,name,representation,pattern),report['rows']):
  values=line.split();timers=row['after'][0]['unit_hand_timers']
  assert row['after'][1]['unit_hand_timers']==timers
  result.append(' '.join([*values[:48],*(f'{v:08x}' for v in timers),*values[48:]]))
 return result

def main():
 parent.DualTrace=RegisteredDualTrace;parent.export_rows=export_rows;parent.__doc__=__doc__
 parent.main()
 parser=argparse.ArgumentParser(add_help=False);parser.add_argument('--output',type=Path,required=True)
 args,_=parser.parse_known_args();path=args.output/'report.json';report=json.loads(path.read_text())
 report.update(parent_generator_tool_sha256=report['tool_sha256'],tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
   fps_registry={'view0_primary_offset':'00000010','view0_secondary_offset':'00001020'})
 path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
