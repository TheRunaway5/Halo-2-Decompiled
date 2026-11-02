#!/usr/bin/env python3
"""Authenticate actual-selected TU1.5 solver hull DATA family; inventory only."""
import argparse,hashlib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);assert hashlib.file_digest(NEW_DEFAULT.open('rb'),'sha256').hexdigest()==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');rows=[]
 for owner in [0x313bb0,0x32a660,0x313560,0x313390,0x313290,0x314710]:
  body=complete_body(code,owner);listing=''.join(f'{pc:x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(body.code));path=a.output/f'{owner:08x}.txt';path.write_text(listing);calls=[{'instruction':pc,'operand':code.at(pc).op_str}for pc in sorted(body.code)if code.at(pc).mnemonic=='call'];rows.append({'owner':owner,'sites':len(body.code),'listing_sha256':hashlib.sha256(listing.encode()).hexdigest(),'calls':calls})
 j={'qualified':False,'inventory_only':True,'reference':'English TU1.5','original_sha256':NEW_PIN,'owners':rows,'owned_scratch':str(scratch),'temporary_allocated_bytes_before_cleanup':0,'retained_large_artifacts':[]};(a.output/'source-pins.json').write_text(json.dumps(j,indent=2)+'\n');print(json.dumps(rows,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
