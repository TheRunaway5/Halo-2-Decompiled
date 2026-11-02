#!/usr/bin/env python3
"""Pin authentic TU1.5 part/contact/material/function source dependencies."""
import argparse,hashlib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 assert hashlib.sha256(Path(NEW_DEFAULT).read_bytes()).hexdigest()==NEW_PIN
 code=_Code(Xbe(NEW_DEFAULT),'.text');owners={}
 for owner in (0x1e0a60,0x184c50,0x355d0,0x13de70,0x17f8a0,0x17f790,0x13e9a0,0x37cde8):
  body=complete_body(code,owner);lines=[];calls=[]
  for pc in sorted(body.code):
   i=code.at(pc);lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
   if i.mnemonic=='call':calls.append(dict(site=pc,target=i.op_str))
  raw=''.join(lines);(a.output/f'{owner:08x}.txt').write_text(raw);owners[hex(owner)]=dict(sites=len(lines),listing_sha256=hashlib.sha256(raw.encode()).hexdigest(),calls=calls)
 (a.output/'source-pins.json').write_text(json.dumps(dict(original_xbe_sha256=NEW_PIN,source_authenticated=True,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],temporary_allocated_bytes_before_cleanup=0,owners=owners,retained_large_artifacts=[],scope='Read-only original source inventory, not native/runtime qualification.'),indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
