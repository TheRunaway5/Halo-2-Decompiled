#!/usr/bin/env python3
"""Authenticate both PUBLIC reversed process parents, including2F7480 prologue."""
import argparse,hashlib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);assert hashlib.file_digest(NEW_DEFAULT.open('rb'),'sha256').hexdigest()==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');rows=[]
 for entry in(0x2f7480,0x2f7fb0):
  body=complete_body(code,entry);listing=''.join(f'{pc:08X} {code.at(pc).bytes.hex():24s} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(body.code));name=f'{entry:08x}.txt';(a.output/name).write_text(listing);rows.append(dict(entry=entry,sites=len(body.code),listing=name,listing_sha256=hashlib.sha256(listing.encode()).hexdigest(),children=[dict(site=pc,target=code.at(pc).operands[0].imm)for pc in sorted(body.code)if code.at(pc).mnemonic=='call']))
 result=dict(passed=True,original_xbe_sha256=NEW_PIN,owners=rows,mask_address=0x461960,mask_hex=image.read(0x461960,16).hex(),source_hashes={str(Path(__file__).relative_to(ROOT)):hashlib.sha256(Path(__file__).read_bytes()).hexdigest()},owned_scratch=os.environ['HALO2_TEST_SCRATCH'],temporary_allocated_bytes_before_cleanup=0,retained_large_artifacts=[]);(a.output/'report.json').write_text(json.dumps(result,indent=2,sort_keys=True)+'\n');print(rows);return 0
if __name__=='__main__':raise SystemExit(main())
