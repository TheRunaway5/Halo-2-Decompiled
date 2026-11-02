#!/usr/bin/env python3
"""Authenticate public31F840 and selected native-info dependencies."""
import argparse,hashlib,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from build_native_tu15_constraint_before_reference import save,sha
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--entry',action='append',type=lambda s:int(s,0),default=[]);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');rows=[]
 for owner in a.entry or[0x31f840]:
  body=complete_body(code,owner);lines=[];calls=[]
  for pc in sorted(body.code):
   i=code.at(pc);lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
   if i.mnemonic in('call','jmp')and i.operands[0].type==2 and i.operands[0].imm not in body.code:calls.append(dict(site=pc,target=i.operands[0].imm))
  text=''.join(lines);name=f'{owner:08x}.txt';(a.output/name).write_text(text);rows.append(dict(entry=owner,sites=len(body.code),listing=name,listing_sha256=hashlib.sha256(text.encode()).hexdigest(),external_calls=calls))
 result=dict(passed=True,original_sha256=NEW_PIN,owners=rows,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(Path(__file__))},owned_scratch=os.environ['HALO2_TEST_SCRATCH'],temporary_allocated_bytes_before_cleanup=0,retained_large_artifacts=[]);save(a.output/'report.json',result);print(rows);return 0
if __name__=='__main__':raise SystemExit(main())
