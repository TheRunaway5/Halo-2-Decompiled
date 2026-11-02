#!/usr/bin/env python3
"""Authenticate live World residual activation parents; read-only source audit."""
import argparse,hashlib,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from build_native_tu15_constraint_before_reference import sha,save
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--owners',nargs='+',type=lambda x:int(x,0));a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');rows=[]
 owners=a.owners or[0x3187c0,0x3190d0,0x321700,0x321a70,0x321d20,0x315cb0,0x316d40,0x322660,0x2816a0,0x2815f0,0x1d77d0,0x1cce00,0x282c10,0x1d7260]
 for owner in owners:
  body=complete_body(code,owner);listing=[];calls=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
   if i.mnemonic=='call'or(i.mnemonic=='jmp'and i.operands[0].type==2 and i.operands[0].imm not in body.code):
    calls.append(dict(site=pc,target=i.operands[0].imm if i.operands[0].type==2 else i.op_str,kind=i.mnemonic))
  text=''.join(listing);(a.output/f'{owner:08x}.txt').write_text(text)
  rows.append(dict(owner=owner,sites=len(body.code),listing_sha256=hashlib.sha256(text.encode()).hexdigest(),calls=calls))
 save(a.output/'report.json',dict(read_only=True,original_sha256=NEW_PIN,owners=rows,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]))
 print([{hex(r['owner']):{'sites':r['sites'],'calls':r['calls']}}for r in rows]);return 0
if __name__=='__main__':raise SystemExit(main())
