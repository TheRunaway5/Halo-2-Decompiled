#!/usr/bin/env python3
"""Authenticate the remaining model-constraint lifetime dependency frontier."""
import argparse,hashlib,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_contact_agent_dispatch_tu15 import sha,save
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
 p.add_argument('--owners',nargs='+',type=lambda s:int(s,0),default=(0x17a0f0,0x17a030,0x17a4a0,0x230a60,0x230ae0,0x22eec0,0x22e4f0,0x1d8c80));a=p.parse_args()
 a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);assert sha(NEW_DEFAULT)==NEW_PIN
 code=_Code(Xbe(NEW_DEFAULT),'.text');rows=[]
 for owner in a.owners:
  body=complete_body(code,owner);listing=[];calls=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
   if i.mnemonic=='call':calls.append(dict(instruction=hex(pc),target=i.op_str))
  text=''.join(listing);file=a.output/f'{owner:06x}.txt';file.write_text(text)
  rows.append(dict(owner=hex(owner),sites=len(body.code),listing=str(file),listing_sha256=hashlib.sha256(text.encode()).hexdigest(),calls=calls))
 save(a.output/'report.json',dict(original_sha256=NEW_PIN,owners=rows,
  owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[],
  source_backed_inventory_only=True,physics_parity_claimed=False));return 0
if __name__=='__main__':raise SystemExit(main())
