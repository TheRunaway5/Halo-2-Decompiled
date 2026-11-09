#!/usr/bin/env python3
"""Source-only inventory of still-unconverted positive body/bumper children."""
import argparse,json,hashlib
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x281890,0x186c30,0x184d30,0x184d80,0x31f460,0x282fd0,0x1d7fc0,0x1e10f0,0x316510)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--owners',nargs='+',type=lambda s:int(s,0));a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);H=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest();assert H(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);c=_Code(image,'.text');rows=[]
 for owner in a.owners or OWNERS:
  b=complete_body(c,owner);lines=[];calls=[]
  for pc in sorted(b.code):
   i=c.at(pc);lines.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
   if i.mnemonic=='call':calls.append({'instruction':pc,'operand':i.op_str})
  f=a.output/f'{owner:08x}.txt';f.write_text(''.join(lines));rows.append({'owner':owner,'sites':len(b.code),'calls':calls,'listing':str(f),'listing_sha256':H(f)})
 (a.output/'report.json').write_text(json.dumps({'passed':True,'source_only':True,'original_execution':False,'native_admission':False,'authority_sha256':NEW_PIN,'owners':rows,'tool_sha256':H(__file__)},indent=2)+'\n')
if __name__=='__main__':main()
