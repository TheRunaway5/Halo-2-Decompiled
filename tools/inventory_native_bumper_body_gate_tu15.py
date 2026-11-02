#!/usr/bin/env python3
"""Authenticate current TU1.5 body-creation entry/calls; no legacy kernel reuse."""
import argparse,hashlib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--owners',nargs='+',type=lambda v:int(v,16),default=[0x1d7530]);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);assert os.environ.get('HALO2_TEST_SCRATCH');assert hashlib.file_digest(NEW_DEFAULT.open('rb'),'sha256').hexdigest()==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');rows=[]
 for owner in a.owners:
  body=complete_body(code,owner);listing=''.join(f'{pc:x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(body.code));(a.output/f'{owner:08x}.txt').write_text(listing);rows.append({'owner':owner,'sites':len(body.code),'listing_sha256':hashlib.sha256(listing.encode()).hexdigest(),'calls':[{'pc':pc,'operand':code.at(pc).op_str}for pc in sorted(body.code)if code.at(pc).mnemonic=='call']})
 j={'inventory_only':True,'qualified':False,'original_sha256':NEW_PIN,'owners':rows,'legacy_version_behavior_used':False,'tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'retained_large_artifacts':[]};(a.output/'report.json').write_text(json.dumps(j,indent=2)+'\n');print(json.dumps(rows,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
