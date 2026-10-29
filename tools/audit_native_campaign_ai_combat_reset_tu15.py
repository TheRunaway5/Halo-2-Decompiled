#!/usr/bin/env python3
"""Original combat-reset dependency/coverage audit; fixture outputs are ignored."""
import argparse,hashlib,json,os,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ENTRIES={0x1ec7b0:'actor combat/perception mode transition',0x267220:'mark eligible actual queries',0x267290:'reset eligible perceptions',0x266490:'actual query retirement, first cached reference and policy',0x16e8a0:'shared actual data-array deletion'}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir();fixture=json.loads(a.fixture.read_text());image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text');bodies={e:_trace(code,e,set(ENTRIES)) for e in ENTRIES};visited=set()
 for spec in fixture['specs']:
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
  for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
  g.mem_map(0x2000000,0x40000);g.mem_map(0x3000000,4096)
  for p,n,v in spec['writes']:g.mem_write(p,v.to_bytes(n,'little'))
  g.mem_write(0x203f000,struct.pack('<I',0x3000000));g.reg_write(R.UC_X86_REG_ESP,0x203f000);g.reg_write(R.UC_X86_REG_EAX,spec['actor']);g.reg_write(R.UC_X86_REG_EDI,spec['mode']&0xffff);g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_MXCSR,spec['initial_mxcsr']);g.hook_add(UC_HOOK_CODE,lambda _g,pc,_n,_u:visited.add(pc));g.emu_start(0x1ec7b0,0x3000000,count=100000);assert g.reg_read(R.UC_X86_REG_EIP)==0x3000000
 rows=[]
 for e,b in bodies.items():
  missing=sorted(set(b.code)-visited);calls=[]
  for pc in sorted(b.code):
   i=code.at(pc)
   if i.mnemonic=='call':calls.append(dict(site=hex(pc),target=i.op_str))
  rows.append(dict(entry=hex(e),semantic_role=ENTRIES[e],sites=len(b.code),executed=len(set(b.code)&visited),uncovered=[dict(site=hex(pc),instruction=code.at(pc).mnemonic+' '+code.at(pc).op_str) for pc in missing],calls=calls))
 result=dict(original_xbe_sha256=NEW_PIN,fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),whole_original_inputs=len(fixture['specs']),owners=rows,unrecovered_direct_targets=[],policy_storage_owner='src/campaign/ai/native_tu15_ai_object_tracking_data.inc mounted by src/platform/native_tu15_final_startup_storage_data.cpp:116',policy_original_section='writable .data; native algorithm reads actual caller storage',policy_xref_receipt='analysis/campaign-native-combat-reset-policy-20261028/xrefs.json',full_ai_set_orders=False)
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(owners=len(rows),uncovered=[x for row in rows for x in row['uncovered']])))
if __name__=='__main__':main()
