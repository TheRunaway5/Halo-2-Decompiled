#!/usr/bin/env python3
"""Lower the complete authenticated TU1.5 renderer initialization prefix through its pending device request."""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--manifest',type=Path,required=True);a=ap.parse_args()
 assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');entries=[0x12560,0x169d0,0x16a10,0x128a0];bodies={entry:_trace(code,entry,set(entries))for entry in entries};instructions={};addresses={};assert not any(body.tables for body in bodies.values())
 for p in sorted(set().union(*(body.code for body in bodies.values()))):
  ins=code.at(p);assert bytes(ins.bytes)==image.read(p,ins.size);instructions[f'{p:08x}']=p;addresses[p]=p;addresses[p+ins.size]=p+ins.size
  for o in ins.operands:
   if o.type==2:addresses[o.imm&0xffffffff]=o.imm&0xffffffff
   if o.type==3:addresses[o.mem.disp&0xffffffff]=o.mem.disp&0xffffffff
 paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{entry:08x}':entry for entry in entries},body_sites={f'{entry:08x}':sorted(body.code)for entry,body in bodies.items()},instruction_map=instructions,address_map={f'{p:08x}':v for p,v in addresses.items()},routines=[dict(entry=entry,tables=[],byte_tables=[])for entry in entries])
 a.correspondence=a.manifest.with_name('renderer-request-bodies.json');a.correspondence.parent.mkdir(parents=True,exist_ok=True);a.correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n');a.additional_entries=Path(__file__)
 generate(a,paired,complete_listings=True)
 s=a.output.read_text().replace('halo2/physics_live_support_tu15.hpp','halo2/source_renderer_request_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_renderer_request_tu15').replace('source_physics_live_support_tu15_owns_entry','source_renderer_request_tu15_owns_entry');a.output.write_text(s)
 r=json.loads(a.manifest.read_text());r.update(renderer_request_generator_sha256=sha(__file__),generated_cpp_sha256=sha(a.output));a.manifest.write_text(json.dumps(r,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(bodies=len(entries),instructions=len(instructions))))
if __name__=='__main__':main()
