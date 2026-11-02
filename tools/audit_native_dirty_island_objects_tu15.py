#!/usr/bin/env python3
"""Authenticate282C10 whole semantic source and exact child-entry readers."""
import argparse,hashlib,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from build_native_tu15_constraint_before_reference import sha,save
OWNERS=(0x282c10,0x1cc310,0x1496f0,0x1d7ca0,0x16e8a0,0xba570,0x1d9510,0xba8c0)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--owners',nargs='+',type=lambda s:int(s,0),default=OWNERS);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');rows=[]
 for owner in a.owners:
  body=complete_body(code,owner);lines=[];calls=[]
  for pc in sorted(body.code):
   i=code.at(pc);lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
   if i.mnemonic=='call':calls.append(dict(site=pc,target=i.operands[0].imm if i.operands[0].type==2 else i.op_str))
  listing=''.join(lines);(a.output/f'{owner:08x}.txt').write_text(listing);rows.append(dict(owner=owner,sites=len(body.code),calls=calls,listing_sha256=hashlib.sha256(listing.encode()).hexdigest()))
 save(a.output/'report.json',dict(read_only=True,original_sha256=NEW_PIN,owners=rows,source_qualification_claimed=False,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],allocated_bytes_before_cleanup=0,retained_large_artifacts=[]));print(dict(read_only=True,owners=[(hex(r['owner']),r['sites'])for r in rows]));return 0
if __name__=='__main__':raise SystemExit(main())
