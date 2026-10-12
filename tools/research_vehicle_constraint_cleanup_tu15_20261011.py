#!/usr/bin/env python3
"""Authenticate TU1.5 constraint cleanup and reached direct dependencies."""
import argparse,collections,csv,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from research_vehicle_contact_state_tu15_20261011 import sha
PARENT=ROOT/'analysis/multiplayer/sandbox-source/vehicle-contact-state-tu15-20261011/research-verified/report.json'
PARENT_PIN='d7c597ab5482a91f50ed555ec43f9b5627db48a093338a46c127677bf6497596'
ROOTS=(0x22e390,0x22fe30,0x22fea0)
SECONDARY={0x22efd0,0x22e210,0x1d8b90,0x17a0f0,0x18cdd0,0x1903a0,0x16e6a0}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert sha(PARENT)==PARENT_PIN and sha(NEW_DEFAULT)==NEW_PIN
 parent=json.loads(PARENT.read_text());image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
 starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
 bindings={};admissions=[]
 for entry in ROOTS:
  calls=parent['outside_direct_calls'][f'{entry:08x}'];assert calls
  for call in calls:
   site=parent['instruction_map'][f"{call['site']:08x}"];i=code.at(site)
   assert i.mnemonic=='call' and i.operands[0].type==2 and i.operands[0].imm==entry==call['later_target']
   admissions.append(dict(caller=call['owner'],site=site,target=entry))
  bindings[entry]=entry
 for caller in ROOTS:
  for site in sorted(_trace(code,caller,starts).code):
   i=code.at(site)
   if i.mnemonic=='call':
    assert i.operands[0].type==2;target=i.operands[0].imm;bindings[target]=target
    admissions.append(dict(caller=caller,site=site,target=target))
 first=set(bindings);pending=set(SECONDARY)
 for caller in sorted(first):
  for site in sorted(_trace(code,caller,starts).code):
   i=code.at(site)
   if i.mnemonic=='call' and i.operands[0].type==2 and i.operands[0].imm in SECONDARY:
    target=i.operands[0].imm;bindings[target]=target;pending.discard(target)
    admissions.append(dict(caller=caller,site=site,target=target))
 assert not pending and len(bindings)==22
 parent_actual=set(parent['entries'].values());reused=sorted(parent_actual&set(bindings))
 assert reused==[0x1d8c80]
 bindings={p:q for p,q in bindings.items() if p not in parent_actual};assert len(bindings)==21
 a.output.mkdir(parents=True,exist_ok=False);owners=[];all_instructions=[];outside={};mnemonics=collections.Counter()
 for entry in sorted(bindings):
  assert entry in starts
  trace=_trace(code,entry,starts);instructions=[code.at(p) for p in sorted(trace.code)];assert not trace.tables
  listing=a.output/f'tu15-{entry:08x}.tsv'
  listing.write_text(''.join(f'{i.address:08x}\t{i.bytes.hex()}\t{i.mnemonic}\t{i.op_str}\n' for i in instructions))
  owners.append(dict(entry=entry,parent_entry_id=entry,instructions=len(instructions),listing_file=listing.name,listing_sha256=sha(listing)))
  all_instructions.extend(instructions);mnemonics.update(i.mnemonic for i in instructions)
  for i in instructions:
   if i.mnemonic in ('call','jmp') and i.operands[0].type==2:
    target=i.operands[0].imm
    if target not in trace.code and target not in bindings and target not in parent_actual:
     outside.setdefault(f'{target:08x}',[]).append(dict(site=i.address,later_target=target,owner=entry,tail=i.mnemonic=='jmp'))
 assert len(all_instructions)==2565 and len({i.address for i in all_instructions})==2565
 result=dict(parent);result.update(schema=3,parity_target='English TU1.5 only',parent_sha256=PARENT_PIN,
  extension_tool_sha256=sha(__file__),tu15_owned_xbe_sha256=NEW_PIN,
  parent_native_entries=sorted(int(p,16) for p in parent['entries']),additional_native_entries=sorted(bindings),
  complete_bodies=120,tu15_new_owners=owners,new_call_bindings=admissions,reused_parent_targets=reused,
  new_recovery_requires_older_image=False,new_source_instruction_ids='TU1.5 virtual addresses')
 result['entries']=dict(parent['entries']);result['entries'].update({f'{p:08x}':q for p,q in bindings.items()})
 result['instruction_map']=dict(parent['instruction_map']);assert not {i.address for i in all_instructions}&{int(p,16) for p in result['instruction_map']}
 result['instruction_map'].update({f'{i.address:08x}':i.address for i in all_instructions})
 result['outside_direct_calls']=dict(parent['outside_direct_calls'])
 for entry in ROOTS:del result['outside_direct_calls'][f'{entry:08x}']
 for key,rows in outside.items():result['outside_direct_calls'].setdefault(key,[]);result['outside_direct_calls'][key]=result['outside_direct_calls'][key]+rows
 result['mnemonics']=dict(collections.Counter(parent['mnemonics'])+mnemonics)
 assert len(result['instruction_map'])==13781
 (a.output/'report.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(new_bodies=21,reused_bodies=1,instructions=2565,report_sha256=sha(a.output/'report.json'))))
if __name__=='__main__':main()
