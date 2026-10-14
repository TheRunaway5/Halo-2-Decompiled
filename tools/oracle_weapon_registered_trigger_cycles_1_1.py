#!/usr/bin/env python3
"""Full original registered dual cycles, now retaining both12-byte triggers.

Original constructors/input/update/action dispatcher and both projectile
postpasses execute. Raw actual definitions, unit/inventory/local view storage
and inherited unavailable effect/world/animation services remain explicit.
This extends the prior registered dual fixture without claiming full-world or
rendered parity. TU1.5 trigger/update correspondence is recorded separately.
"""
import argparse,hashlib,json,struct
from pathlib import Path
import oracle_weapon_dual_cycles_1_1 as parent
import oracle_weapon_registered_dual_cycles_1_1 as registered
from oracle_smg_firing_1_1 import WEAPON
class RegisteredTriggerTrace(registered.RegisteredDualTrace):
 def __init__(self,*args,**kwargs):
  super().__init__(*args,**kwargs)
  self.services[0xd2310]=('unit camouflage disruption: inherited unavailable owner',0,8)
 def pair_state(self):
  states=super().pair_state()
  for state,address in zip(states,(WEAPON,parent.OTHER)):
   state['trigger_hex']=bytes(self.g.mem_read(address+0x20c,12)).hex()
  return states

def export_rows(report,name,representation,pattern):
 result=[]
 for line,row in zip(registered.export_rows(report,name,representation,pattern),report['rows']):
  values=line.split();triggers=[]
  for state in row['after']:triggers.extend(struct.unpack('<3I',bytes.fromhex(state['trigger_hex'])))
  result.append(' '.join([*values[:50],*(f'{v:08x}' for v in triggers),*values[50:]]))
 return result

def main():
 parent.DualTrace=RegisteredTriggerTrace;parent.export_rows=export_rows;parent.__doc__=__doc__;parent.main()
 parser=argparse.ArgumentParser(add_help=False);parser.add_argument('--output',type=Path,required=True);args,_=parser.parse_known_args()
 path=args.output/'report.json';report=json.loads(path.read_text());report.update(parent_generator_tool_sha256=report['tool_sha256'],registered_generator_tool_sha256=hashlib.sha256(Path(registered.__file__).read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),trigger_words_per_hand=3)
 path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
