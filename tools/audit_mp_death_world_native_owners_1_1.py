#!/usr/bin/env python3
"""Audit native coverage of real original death-world calls and virtual tails.

Includes method targets absent from the original function-index listing.
Only function ownership is checked here; native store comparisons remain
separate, and this is not proof of a live Match or a continuous native world.
"""
import argparse,hashlib,json,collections
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,PIN,CACHE_PIN

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--report',type=Path,required=True)
 ap.add_argument('--bindings',type=Path,required=True)
 ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 j=json.loads(a.report.read_text());b=json.loads(a.bindings.read_text())
 assert j['source_sha256']==PIN and j['cache_sha256']==CACHE_PIN
 assert j['tool_sha256']==sha(ROOT/'tools/oracle_mp_death_world_scheduler_1_1.py')
 assert j['failure']is None and j['frozen_prior_byte_identical']
 assert not j['instruction_patches'] and not j['new_supplied_guest_fields_or_results']
 assert b['tool_sha256']==sha(ROOT/'tools/generate_death_world_runtime_bindings_1_1.py')
 assert b['generated_sha256']==sha(ROOT/'src/gameplay/units/death_world_runtime_entries.hpp')
 native=set()
 for v in b['owners'].values():
  assert v['sha256']==sha(ROOT/v['source'])
  native.update(int(q,16)for q in v['complete_entries'])
 image_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
 assert sha(image_path)==PIN
 image=Xbe(image_path);code=_Code(image,'.text')
 for listing in j['instructions']:
  p=a.report.parent/listing['file'];assert sha(p)==listing['sha256']
  for line in p.read_text().splitlines():
   address,raw,*_=line.split();address=int(address,16);raw=bytes.fromhex(raw)
   assert image.read(address,len(raw))==raw
 targets=collections.defaultdict(set)
 for q in j['rebuild_calls']:targets[int(q['entry'],16)]
 register_calls=set()
 for q in j['rebuild_reads']:
  pc=int(q['instruction'],16);ins=code.at(pc)
  if ins.mnemonic not in {'call','jmp'}:continue
  op=ins.operands[0]
  if ins.mnemonic=='call' and op.type==2:targets[op.imm].add(pc)
  elif op.type==3 and q['size']==4 and (ins.mnemonic=='call' or not op.mem.index):
   target=int.from_bytes(bytes.fromhex(q['raw_hex']),'little')
   if code.lo<=target<code.hi:targets[target].add(pc)
  elif ins.mnemonic=='call' and op.type==1:register_calls.add(pc)
 missing={f'{p:08x}':[f'{q:08x}'for q in sorted(sites)]for p,sites in targets.items()if p not in native}
 assert not register_calls, 'An actual register call needs its original register capture'
 assert not missing,missing
 a.output.write_text(json.dumps(dict(tool_sha256=sha(Path(__file__)),report_sha256=sha(a.report),bindings_sha256=sha(a.bindings),source_sha256=PIN,cache_sha256=CACHE_PIN,original_frames=len(j['frames']),world_steps=len(j['rebuild_roots']),original_recorded_calls=len(j['rebuild_calls']),original_store_count=len(j['rebuild_stores']),complete_native_entries=len(native),actual_original_owners=len(targets),missing=missing,actual_owners={f'{p:08x}':[f'{q:08x}'for q in sorted(sites)]for p,sites in sorted(targets.items())},captured_gameplay_results_used=False,live_match_proof=False,boundaries=__doc__),indent=2)+'\n')
 print(json.dumps(dict(world_steps=len(j['rebuild_roots']),actual_original_owners=len(targets),missing=missing)))
if __name__=='__main__':main()
