#!/usr/bin/env python3
"""Authenticate the point-creation listener child graph; no game execution."""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x2e4b60,0x281710,0x316200,0x281530,0x281f10,0x283120,0x31dd70,0x1d7530)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 sha=lambda f:hashlib.sha256(Path(f).read_bytes()).hexdigest();assert sha(NEW_DEFAULT)==NEW_PIN
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');owners=[]
 for owner in OWNERS:
  body=complete_body(code,owner);lines=[];calls=[]
  for pc in sorted(body.code):
   ins=code.at(pc);lines.append(f'{pc:x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n')
   if ins.mnemonic=='call':calls.append(dict(instruction=pc,operand=ins.op_str))
  f=a.output/f'{owner:08x}.txt';f.write_text(''.join(lines));owners.append(dict(owner=owner,sites=len(body.code),calls=calls,listing=str(f),listing_sha256=sha(f)))
 report=dict(passed=True,original_text_sha256=NEW_PIN,source_only=True,original_execution=False,native_admission=False,owners=owners,tool_sha256=sha(__file__))
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
