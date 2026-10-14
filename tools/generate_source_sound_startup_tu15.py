#!/usr/bin/env python3
"""Compile complete authenticated TU1.5 sound-startup CPU owners.

The native device, section, DSP and SDK services remain real ABI boundaries;
no availability flag, voice, initialization result or callee return is emitted.
"""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

ENTRIES=(0x224ec0,0x2254f0,0x12eb50,0x1972c0,0x1973e0,0x2275a0,0x227ea0)
CALLS=((0x128050,0x224ec0),(0x224f26,0x387877),(0x225020,0x1972c0),
       (0x225072,0x2271a0),(0x22507d,0x12eb50),(0x22518c,0x1973e0),
       (0x225465,0x227ea0),(0x2254dc,0x2254f0),(0x197495,0x2275a0))

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--output',type=Path,default=Path('src/source_sound_startup_tu15_generated.cpp'))
 p.add_argument('--directory',type=Path,required=True);a=p.parse_args()
 assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
 for pc,target in CALLS:
  i=code.at(pc);assert i.mnemonic=='call' and i.operands[0].type==2 and i.operands[0].imm==target,(hex(pc),i.op_str)
 sites,instructions,addresses,routines={},{},{},[];a.directory.mkdir(parents=True,exist_ok=True)
 for entry in ENTRIES:
  body=_trace(code,entry,set(ENTRIES));assert body.code and all(width==4 for _,width,_ in body.tables)
  sites[f'{entry:08x}']=sorted(body.code);routines.append(dict(entry=entry,tables=[dict(disc=q,count=n)for q,_,n in body.tables],byte_tables=[]))
  listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes);listing.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
   instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
   for operand in i.operands:
    if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
    elif operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
  (a.directory/f'{entry:08x}.asm').write_text(''.join(listing))
 paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in ENTRIES},body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':v for p,v in addresses.items()},routines=routines)
 correspondence=a.directory/'bodies.json';correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
 generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=correspondence,additional_entries=Path(__file__)),paired)
 s=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_source_sound_startup_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_sound_startup_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_sound_startup_tu15').replace('source_physics_live_support_tu15_owns_entry','source_sound_startup_tu15_owns_entry')
 a.output.write_text(s);report=json.loads((a.directory/'generation.json').read_text());report['generated_cpp_sha256']=sha(a.output);report['startup_generator_sha256']=sha(Path(__file__));(a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 print(json.dumps(dict(bodies=len(ENTRIES),instructions=len(instructions),output=str(a.output))))

if __name__=='__main__':main()
