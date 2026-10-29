#!/usr/bin/env python3
"""Retain the authenticated orders/reset dependency frontier; static evidence only."""
import argparse,hashlib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
NATIVE={0x26c600,0x140d50,0x140db0,0x140e00,0x267a30,0x267a40}
ENTRIES=[0x2af970,0x27f9c0,0x20b150,0x20b3e0,0x20b0f0,0x1ec820,0x1ec7b0,0x1eb590,
 0x26c600,0x140d50,0x140db0,0x140e00,0x267a30,0x267a40,0x267220,0x267290,0x266490,
 0x277c00,0x277930,0x1ea580,0xe8e10,0xe8e50,0xd3340,0xb7e50,0x29b450,0x211690,
 0x211a80,0x211cb0,0x211b10,0x211360,0x211550,0x20c710,0xbbe00,0x218d90,0x274fa0,0x16e8a0]
ROLES={0x274fa0:'clump combat-status selection from member distributions, four policy rows at47D6D0; candidate predicate+0/current selector+4',
       0x1ea580:'actor behavior-reset fanout through47B478 descriptor+34; separate from clump combat-status selection',
       0x1eb590:'actor active-camouflage control with real unit and dirty-entity publication'}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text');rows=[];unknown=set()
 for e in ENTRIES:
  b=_trace(code,e,set(ENTRIES));calls=[]
  for pc in sorted(b.code):
   i=code.at(pc)
   if i.mnemonic!='call':continue
   target=int(i.op_str,16) if i.op_str.startswith('0x') else None
   calls.append(dict(site=hex(pc),target=hex(target) if target else 'indirect',operand=i.op_str,native_dependency=target in NATIVE))
   if target and target not in ENTRIES:unknown.add(target)
  rows.append(dict(entry=hex(e),semantic_role=ROLES.get(e),original_instruction_sites=len(b.code),listing_sha256=hashlib.sha256(''.join(f'{pc:x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n' for pc in sorted(b.code)).encode()).hexdigest(),native_scope='actor/script-key association, original4-byte payload and installed hash/equality' if e in NATIVE else None,calls=calls))
 result=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,owners=rows,unexpanded_direct_targets=sorted(hex(x) for x in unknown),full_orders_native=False,remaining_required_families=['actor combat/perception reset','unit protection/action requests and attachment/network publication','squad transition/spatial producers','HS expression/thread activation and order scripts','clump combat-status selection and actual policy callbacks','actor behavior reset and indirect callbacks'],native_component_proof='analysis/campaign-native-kernels-20261028/ai-orders/script-assignment/component-proof.json')
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(owners=len(rows),unexpanded_direct_targets=len(unknown),native_component_entries=len(NATIVE),full_orders_native=False)))
if __name__=='__main__':main()
