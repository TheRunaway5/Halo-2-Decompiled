#!/usr/bin/env python3
"""Compile complete authenticated TU1.5 ordinary heap growth helpers."""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate
ENTRIES=(0x2ddeb9,0x2dd367,0x2ddd9b,0x2dd524,0x2dd7c9,0x2dd2b3,0x2dd2d4)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
class Code:
 def __init__(self,image):self.sections=(_Code(image,'.text'),)
 def at(self,p):
  for code in self.sections:
   if code.inside(p):return code.at(p)
  raise AssertionError(hex(p))
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--manifest',type=Path,required=True);a=ap.parse_args()
 assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=Code(image)
 entries={f'{p:08x}':p for p in ENTRIES};sites={};instructions={};addresses={};routines=[]
 for entry in ENTRIES:
  section=next(s for s in image.sections if s.va<=entry<s.va+s.vsize)
  body=_trace(_Code(image,section),entry,set(ENTRIES));assert body.code
  sites[f'{entry:08x}']=sorted(body.code);assert not body.tables
  routines.append(dict(entry=entry,tables=[],byte_tables=[],section=section.name))
  for p in sorted(body.code):
   ins=code.at(p);assert bytes(ins.bytes)==image.read(p,ins.size);instructions[f'{p:08x}']=p;addresses[p]=p;addresses[p+ins.size]=p+ins.size
   for o in ins.operands:
    if o.type==2:addresses[o.imm&0xffffffff]=o.imm&0xffffffff
    if o.type==3:addresses[o.mem.disp&0xffffffff]=o.mem.disp&0xffffffff
 paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries=entries,body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':v for p,v in addresses.items()},routines=routines)
 a.correspondence=a.manifest.with_name('heap-growth-bodies.json');a.correspondence.parent.mkdir(parents=True,exist_ok=True);a.correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n');a.additional_entries=Path(__file__)
 generate(a,paired,complete_listings=True,code_override=code)
 s=a.output.read_text().replace('halo2/physics_live_support_tu15.hpp','halo2/source_heap_growth_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_heap_growth_tu15').replace('source_physics_live_support_tu15_owns_entry','source_heap_growth_tu15_owns_entry');a.output.write_text(s)
 r=json.loads(a.manifest.read_text());r.update(heap_growth_generator_sha256=sha(__file__),generated_cpp_sha256=sha(a.output));a.manifest.write_text(json.dumps(r,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(bodies=len(ENTRIES),instructions=len(instructions))))
if __name__=='__main__':main()
