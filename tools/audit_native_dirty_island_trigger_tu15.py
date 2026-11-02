#!/usr/bin/env python3
"""Authenticate every literal TU1.5 pending-byte reference and its owners."""
import argparse,hashlib,os
from pathlib import Path
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from build_native_tu15_constraint_before_reference import sha,save
OWNERS=(0x282b70,0x282b90,0x282bc0,0x282be0,0x282c10,0x1cb330,0x149b90)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');known={};rows=[]
 for owner in OWNERS:
  body=complete_body(code,owner);listing=[]
  for pc in sorted(body.code):
   ins=code.at(pc);known[pc]=owner;listing.append(f'{pc:08x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n')
  text=''.join(listing);(a.output/f'{owner:08x}.txt').write_text(text);rows.append(dict(owner=owner,sites=len(body.code),listing_sha256=hashlib.sha256(text.encode()).hexdigest()))
 literal=(0x56dfcc).to_bytes(4,'little');offsets=[];cursor=0
 while True:
  at=code.bytes.find(literal,cursor)
  if at<0:break
  offsets.append(at);cursor=at+1
 references=[];unclassified=[]
 for at in offsets:
  choices=[]
  for p in range(max(code.lo,code.lo+at-11),code.lo+at+1):
   ins=code.at(p)
   if not ins or not(p<=code.lo+at and code.lo+at+4<=p+ins.size):continue
   if any(op.type==X86_OP_MEM and not op.mem.base and not op.mem.index and op.mem.disp&0xffffffff==0x56dfcc for op in ins.operands):choices.append((p,ins))
  matched=[(p,i)for p,i in choices if p in known];assert len(matched)==1,(hex(code.lo+at),[(hex(p),i.mnemonic,i.op_str)for p,i in choices])
  pc,ins=matched[0];references.append(dict(instruction=pc,owner=known[pc],hex=ins.bytes.hex(),assembly=ins.mnemonic+' '+ins.op_str))
 assert {r['instruction']for r in references}=={0x282bb3,0x282c03,0x282c10,0x282fbb,0x1cb401}
 save(a.output/'report.json',dict(verified=True,original_sha256=NEW_PIN,absolute_literal_references=references,owners=rows,
  positive_writers=[dict(instruction=0x282bb3,owner=0x282b90,source_condition='current secondary/default allocation total differs from stored island-collision begin total',island_store=0x282bad),dict(instruction=0x282c03,owner=0x282be0,source_condition='current secondary/default allocation total differs from stored merge begin total',island_store=0x282bfd)],
  clearing_writers=[dict(instruction=0x282fbb,owner=0x282c10),dict(instruction=0x1cb401,owner=0x1cb330)],
  allocation_total='Actual149B90/native_tu15_island_allocated_bytes reads globals486070/486074; nonnull owners with signed-positive+20 count sum allocation ledger words from owner+18/+1C backward8B; arithmetic wraps32bits',
  inference='A genuine enabled dirty-island input requires a real monitored secondary/default allocation delta during collision or deferred merge. Ordinary driving/grenades are not evidence for such a delta; Source controls/flags must not be injected.',
  runtime_input_claimed=False,native_source_files_mutated=False,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],allocated_bytes_before_cleanup=0,retained_large_artifacts=[]));print(dict(verified=True,references=len(references)));return 0
if __name__=='__main__':raise SystemExit(main())
