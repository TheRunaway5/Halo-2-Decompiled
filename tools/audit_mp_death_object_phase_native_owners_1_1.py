#!/usr/bin/env python3
"""Audit original five-phase native callees and executed instruction coverage.

Actual register method loads absent from the function index are checked against
original reads and execution. Complete bodies, not recorded results, are bound.
Native store and persistent-bank comparisons remain separate. This is not live
Match construction, rendered corpse/camera or whole-title scheduling proof.
"""
import argparse,hashlib,json,collections
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace,PIN,CACHE_PIN

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--report',type=Path,required=True)
 ap.add_argument('--bindings',type=Path,required=True)
 ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 j=json.loads(a.report.read_text());b=json.loads(a.bindings.read_text())
 assert j['source_sha256']==PIN and j['cache_sha256']==CACHE_PIN
 assert j['tool_sha256']==sha(ROOT/'tools/oracle_mp_death_object_phases_1_1.py')
 assert j['failure']is None and j['frozen_prior_byte_identical']
 assert not j['instruction_patches'] and not j['new_supplied_guest_fields_or_results']
 assert b['tool_sha256']==sha(ROOT/'tools/generate_death_world_runtime_bindings_1_1.py')
 assert b['generated_sha256']==sha(ROOT/'src/death_world_runtime_entries.hpp')
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
 # Original register targets which are not in the function index. The
 # original load and the actual entry instruction must both be observed.
 executed=set()
 for listing in j['instructions']:
  if listing.get('kind')=='complete_new_execution':
   executed.update(int(line.split()[0],16)for line in (a.report.parent/listing['file']).read_text().splitlines())
 register_calls={p for p in executed if code.at(p).mnemonic=='call'and code.at(p).operands[0].type==1}
 method_loads={0xf9b10:0x10ce33,0x1ddf30:0x20658}
 for target,site in method_loads.items():
  assert target in executed and target in native
  ins=code.at(site)
  assert ins.mnemonic=='mov'and ins.operands[0].type==1 and ins.operands[1].type==3
  assert any(int(q['instruction'],16)==site and q['size']==4 and
      int.from_bytes(bytes.fromhex(q['raw_hex']),'little')==target for q in j['rebuild_reads'])
  targets[target].add(site)
 covered=set()
 for entry in native:covered.update(_trace(code,entry,native).code)
 assert not executed-covered, [hex(p)for p in sorted(executed-covered)]
 assert not missing,missing
 a.output.write_text(json.dumps(dict(tool_sha256=sha(Path(__file__)),report_sha256=sha(a.report),bindings_sha256=sha(a.bindings),source_sha256=PIN,cache_sha256=CACHE_PIN,original_frames=len(j['frames']),object_phase_roots=len(j['rebuild_roots']),world_steps=len(j['frames'])-1,original_recorded_calls=len(j['rebuild_calls']),original_store_count=len(j['rebuild_stores']),complete_native_entries=len(native),actual_original_owners=len(targets),missing=missing,original_executed_instructions=len(executed),uncovered_executed_instructions=[],register_call_sites=[f'{q:08x}'for q in sorted(register_calls)],verified_unlisted_register_method_loads={f'{k:08x}':f'{v:08x}'for k,v in method_loads.items()},actual_owners={f'{p:08x}':[f'{q:08x}'for q in sorted(sites)]for p,sites in sorted(targets.items())},captured_gameplay_results_used=False,live_match_proof=False,boundaries=__doc__),indent=2)+'\n')
 print(json.dumps(dict(object_phase_roots=len(j['rebuild_roots']),world_steps=len(j['frames'])-1,actual_original_owners=len(targets),missing=missing)))
if __name__=='__main__':main()
