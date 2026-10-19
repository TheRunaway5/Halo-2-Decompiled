#!/usr/bin/env python3
"""Lower complete TU1.5 geometry-cache frame, pressure, prediction and age-reset owners."""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
ENTRIES=(0x130890,0x130720,0x130f90,0x13fd10,0x1306b0,0x130bd0,0x13b2e0,0x13b320,0x140430,0x130d20,0x130da0,0x22a790,0x22a810)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();assert not a.output.exists()and not a.directory.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 x=Xbe(NEW_DEFAULT);code=_Code(x,'.text');sites={};instructions={};addresses={};routines=[];calls={}
 for entry in ENTRIES:
  b=_trace(code,entry,set(ENTRIES));assert b.code and not b.tables
  sites[f'{entry:08x}']=sorted(b.code);routines.append(dict(entry=entry,tables=[],byte_tables=[]))
  for pc in sorted(b.code):
   i=code.at(pc);assert x.read(pc,i.size)==bytes(i.bytes);instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
   if i.mnemonic=='call':calls[f'{pc:08x}']=i.op_str
   for op in i.operands:
    if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
    elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
 paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in ENTRIES},body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
 a.directory.mkdir(parents=True);f=a.directory/'bodies.json';f.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n');manifest=a.directory/'generation.json';generate(SimpleNamespace(output=a.output,manifest=manifest,correspondence=f,additional_entries=Path(__file__)),paired,code_override=code)
 s=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_geometry_cache_frame_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_geometry_cache_frame_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_geometry_cache_frame_tu15').replace('source_physics_live_support_tu15_owns_entry','source_geometry_cache_frame_tu15_owns_entry');a.output.write_text(s);d=json.loads(manifest.read_text());d.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),scope=__doc__,calls=calls);manifest.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n');print('Complete frame owner:',len(instructions),'instructions; dependencies',calls)
if __name__=='__main__':main()
