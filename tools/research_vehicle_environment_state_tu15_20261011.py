#!/usr/bin/env python3
"""Authenticate TU1.5 environment queries, force retirement and pose composition."""
import argparse,collections,csv,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from research_vehicle_contact_state_tu15_20261011 import sha
PARENT=ROOT/'analysis/multiplayer/sandbox-source/vehicle-constraint-cleanup-tu15-20261011/research-verified/report.json'
PARENT_PIN='4fca8578ae89224c2b5fecb83ad9e7ee72b1d482d4468243086e6388ad84fb5f'
ROOTS={0x11e600,0x11e850,0x18b1d0,0x17a030,0x17a4a0,0x190420,0x228220,0x2e4650}
SECONDARY={0x16e8a0,0x177fa0,0x177020}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert sha(PARENT)==PARENT_PIN and sha(NEW_DEFAULT)==NEW_PIN
 parent=json.loads(PARENT.read_text());image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
 starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
 parent_actual=set(parent['entries'].values());bindings=set(ROOTS);admissions=[]
 for entry in sorted(ROOTS):
  calls=parent['outside_direct_calls'][f'{entry:08x}'];assert calls
  for call in calls:
   site=parent['instruction_map'][f"{call['site']:08x}"];i=code.at(site)
   assert i.mnemonic=='call' and i.operands[0].type==2 and i.operands[0].imm==entry==call['later_target']
   admissions.append(dict(caller=call['owner'],site=site,target=entry))
 for depth in range(2):
  reached=set()
  for caller in sorted(bindings):
   for site in sorted(_trace(code,caller,starts).code):
    i=code.at(site)
    if i.mnemonic=='call':
     assert i.operands[0].type==2;target=i.operands[0].imm;reached.add(target)
  bindings|=reached-parent_actual
 assert len(bindings)==23
 pending=set(SECONDARY)
 for caller in sorted(bindings):
  for site in sorted(_trace(code,caller,starts).code):
   i=code.at(site)
   if i.mnemonic=='call' and i.operands[0].type==2 and i.operands[0].imm in SECONDARY:pending.discard(i.operands[0].imm)
 assert not pending;bindings|=SECONDARY;assert len(bindings)==26 and not bindings&parent_actual
 a.output.mkdir(parents=True,exist_ok=False);owners=[];all_instructions=[];outside={};mnemonics=collections.Counter()
 for entry in sorted(bindings):
  assert entry in starts;trace=_trace(code,entry,starts);instructions=[code.at(p) for p in sorted(trace.code)]
  listing=a.output/f'tu15-{entry:08x}.tsv'
  listing.write_text(''.join(f'{i.address:08x}\t{i.bytes.hex()}\t{i.mnemonic}\t{i.op_str}\n' for i in instructions))
  tables=[dict(address=p,width=w,count=n,raw_hex=image.read(p,w*n).hex()) for p,w,n in trace.tables]
  owners.append(dict(entry=entry,parent_entry_id=entry,instructions=len(instructions),listing_file=listing.name,listing_sha256=sha(listing),tables=tables))
  all_instructions.extend(instructions);mnemonics.update(i.mnemonic for i in instructions)
  for i in instructions:
   if i.mnemonic in ('call','jmp') and i.operands[0].type==2:
    target=i.operands[0].imm
    if target in bindings and i.mnemonic=='call':admissions.append(dict(caller=entry,site=i.address,target=target))
    if target not in trace.code and target not in bindings and target not in parent_actual:
     outside.setdefault(f'{target:08x}',[]).append(dict(site=i.address,later_target=target,owner=entry,tail=i.mnemonic=='jmp'))
 assert len(all_instructions)==1307 and len({i.address for i in all_instructions})==1307
 result=dict(parent);result.update(schema=4,parity_target='English TU1.5 only',parent_sha256=PARENT_PIN,
  extension_tool_sha256=sha(__file__),tu15_owned_xbe_sha256=NEW_PIN,
  parent_native_entries=sorted(int(p,16) for p in parent['entries']),additional_native_entries=sorted(bindings),
  complete_bodies=146,tu15_new_owners=owners,new_call_bindings=admissions,reused_parent_targets=[],
  new_recovery_requires_older_image=False,new_source_instruction_ids='TU1.5 virtual addresses')
 result['entries']=dict(parent['entries']);result['entries'].update({f'{p:08x}':p for p in bindings})
 result['instruction_map']=dict(parent['instruction_map']);assert not {i.address for i in all_instructions}&{int(p,16) for p in result['instruction_map']}
 result['instruction_map'].update({f'{i.address:08x}':i.address for i in all_instructions})
 result['outside_direct_calls']={key:rows for key,rows in parent['outside_direct_calls'].items() if int(key,16) not in bindings}
 for key,rows in outside.items():result['outside_direct_calls'].setdefault(key,[]);result['outside_direct_calls'][key]=result['outside_direct_calls'][key]+rows
 result['mnemonics']=dict(collections.Counter(parent['mnemonics'])+mnemonics);assert len(result['instruction_map'])==15088
 (a.output/'report.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(new_bodies=26,instructions=1307,report_sha256=sha(a.output/'report.json'))))
if __name__=='__main__':main()
