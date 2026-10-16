#!/usr/bin/env python3
"""Lower whole unchanged TU1.5 timed notification and virtual callback bodies.

Actual clock, timer, deferred queue and exclusion owners are mandatory. Playing
voice/PIO branches retain explicit required native resource boundaries.
"""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate

ENTRIES=[0x38901d,0x388c49,0x3891fa,0x388be7,0x38d1dd,0x38ceb2,
         0x38cbb9,0x38cbe8,0x389148,0x389325,0x383d40,0x383d70,0x383d92]

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--manifest',type=Path,required=True);a=p.parse_args()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'DSOUND')
 bodies={e:_trace(code,e,set(ENTRIES)) for e in ENTRIES};assert not any(b.tables for b in bodies.values())
 instructions={};addresses={}
 for pc in sorted(set().union(*(b.code for b in bodies.values()))):
  op=code.at(pc);assert bytes(op.bytes)==image.read(pc,op.size);instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+op.size]=pc+op.size
  for operand in op.operands:
   if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
   if operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
 paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{e:08x}':e for e in ENTRIES},body_sites={f'{e:08x}':sorted(b.code) for e,b in bodies.items()},instruction_map=instructions,address_map={f'{pc:08x}':v for pc,v in addresses.items()},routines=[dict(entry=e,tables=[],byte_tables=[]) for e in ENTRIES])
 a.correspondence=a.manifest.with_name('sound-timed-notifications-bodies.json');a.correspondence.parent.mkdir(parents=True,exist_ok=True);a.correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n');a.additional_entries=Path(__file__)
 generate(a,paired,complete_listings=True,code_override=code)
 text=a.output.read_text().replace('halo2/physics_live_support_tu15.hpp','halo2/source_sound_timed_notifications_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_sound_timed_notifications_tu15').replace('source_physics_live_support_tu15_owns_entry','source_sound_timed_notifications_tu15_owns_entry');a.output.write_text(text)
 report=json.loads(a.manifest.read_text());report.update(sound_timed_notifications_generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),generated_cpp_sha256=hashlib.sha256(a.output.read_bytes()).hexdigest());a.manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(bodies=len(ENTRIES),instructions=len(instructions))))
if __name__=='__main__':main()
