#!/usr/bin/env python3
"""Authenticate the remaining1496F0 frame service and its direct source owners."""
import argparse,hashlib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH');a.output.mkdir(parents=True,exist_ok=False);assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');rows=[]
 for owner in (0x1496f0,0x149ad0,0x149680,0x1498a0,0x1ccbe0,0x1cd130,0x1cb330,0x1cb220,0x2335b0):
  body=complete_body(code,owner);listing=[];calls=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:08X} {i.bytes.hex():24} {i.mnemonic} {i.op_str}\n')
   if i.mnemonic=='call':calls.append(dict(instruction=pc,operands=i.op_str))
  f=a.output/f'{owner:08x}-original-listing.txt';f.write_text(''.join(listing));rows.append(dict(owner=owner,sites=len(body.code),listing_sha256=sha(f),calls=calls))
 report=dict(passed=True,original_sha256=NEW_PIN,owners=rows,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[],scope='Read-only complete-body source inventory, no native implementation or live claim.',temporary_allocated_bytes_before_cleanup=0);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
