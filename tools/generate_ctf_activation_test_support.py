#!/usr/bin/env python3
"""Compile existing whole original native callees for focused CTF ABI tests.

No object-factory return is provided: a reached B9B70 remains an explicit stop.
Production uses its existing same-bank native factory and shared owners instead.
"""
import argparse,csv,hashlib,json,os
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate

ENTRIES=(0x1a8850,0xbcdf0,0xb9960,0xb1a70,0xb7e50,0xb7d60,0x31d400,
    0xa6780,0x2e4d60,0xad740,0x83050,0xbccf0,0x1a8e50,0x16ea60,
    0x3825d0,0x160b70,0x160c90,0x160ea0,0x164890)

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--directory',type=Path,required=True);a=p.parse_args()
 assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir();assert sha(NEW_DEFAULT)==NEW_PIN
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');a.directory.mkdir(parents=True,exist_ok=True)
 starts={int(r['address'],16)for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}|set(ENTRIES)
 sites={};instructions={};addresses={};routines=[];listings=[]
 for e in ENTRIES:
  body=_trace(code,e,starts);assert body.code and all(w==4 for _,w,_ in body.tables)
  sites[f'{e:08x}']=sorted(body.code);routines.append(dict(entry=e,tables=[dict(disc=q,count=n)for q,_,n in body.tables],byte_tables=[]))
  rows=[]
  for pc in sorted(body.code):
   i=code.at(pc);assert bytes(i.bytes)==image.read(pc,i.size)
   rows.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
   for o in i.operands:
    if o.type==2:addresses[o.imm&0xffffffff]=o.imm&0xffffffff
    elif o.type==3:addresses[o.mem.disp&0xffffffff]=o.mem.disp&0xffffffff
  path=a.directory/f'{e:08x}.asm';path.write_text(''.join(rows));listings.append(dict(entry=f'{e:08x}',file=path.name,instructions=len(rows),sha256=sha(path)))
 paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{e:08x}':e for e in ENTRIES},body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':v for p,v in addresses.items()},routines=routines)
 path=a.directory/'bodies.json';path.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n');manifest=a.directory/'generation.json'
 generate(SimpleNamespace(output=a.output,manifest=manifest,correspondence=path,additional_entries=Path(__file__)),paired,complete_listings=True,code_override=code)
 a.output.write_text(a.output.read_text().replace('halo2/physics_live_support_tu15.hpp','ctf_activation_test_support.hpp').replace('source_run_physics_live_support_tu15','source_run_ctf_activation_test_support').replace('source_physics_live_support_tu15_owns_entry','source_ctf_activation_test_support_owns_entry'))
 report=json.loads(manifest.read_text());report.update(test_only=True,generator_source_sha256=sha(__file__),generated_cpp_sha256=sha(a.output),instruction_listings=listings,supplied_function_results=[])
 manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(dict(bodies=len(ENTRIES),instructions=len(instructions),generated_cpp_sha256=sha(a.output))),flush=True)
if __name__=='__main__':main()
