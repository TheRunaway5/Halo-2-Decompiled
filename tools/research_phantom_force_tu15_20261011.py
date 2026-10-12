#!/usr/bin/env python3
"""Authenticate the TU1.5 phantom force updater and physical delivery owners."""
import argparse,collections,csv,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from research_vehicle_contact_state_tu15_20261011 import sha
PARENT=ROOT/'analysis/multiplayer/sandbox-source/vehicle-environment-state-tu15-20261011/research-verified/report.json'
PARENT_PIN='91ff94bf741614d14d7c13ca55292f2bf7a6874e253b21484d3e4fcf98c6c238'
ENTRIES={0x1da750,0x24570,0xe1480,0x11b930,0x1d9100,0x1d92c0,0x1da530,0x315c90,0x1d9080,0x315cb0,0x3239f0,0x323a00}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert sha(PARENT)==PARENT_PIN and sha(NEW_DEFAULT)==NEW_PIN
 parent=json.loads(PARENT.read_text());image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
 starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
 known=set(parent['entries'].values());assert not known&ENTRIES
 admissions=[];reached=set();virtuals=[]
 for table in sorted({r['later_table'] for r in parent['virtual_bindings']}):
  for offset,target in ((0x50,0x3239f0),(0x54,0x323a00)):
   assert struct.unpack('<I',image.read(table+offset,4))[0]==target
   virtuals.append(dict(table=table,offset=offset,target=target,raw_hex=image.read(table+offset,4).hex()));reached.add(target)
 for owner in parent['outside_direct_calls']['001d5e90']:
  site=parent['instruction_map'][f"{owner['site']:08x}"];i=code.at(site)
  assert i.mnemonic=='call' and i.operands[0].imm==0x1da750==owner['later_target']
  admissions.append(dict(caller=owner['owner'],site=site,target=0x1da750))
 a.output.mkdir(parents=True,exist_ok=False);rows=[];outside={};mnemonics=collections.Counter();instructions=[]
 for entry in sorted(ENTRIES):
  trace=_trace(code,entry,starts);assert not trace.tables
  body=[code.at(p) for p in sorted(trace.code)];listing=a.output/f'tu15-{entry:08x}.tsv'
  listing.write_text(''.join(f'{i.address:08x}\t{i.bytes.hex()}\t{i.mnemonic}\t{i.op_str}\n' for i in body))
  rows.append(dict(entry=entry,parent_entry_id=entry,instructions=len(body),tables=[],listing_file=listing.name,listing_sha256=sha(listing)))
  for i in body:
   if i.mnemonic in ('call','jmp') and i.operands[0].type==2:
    target=i.operands[0].imm
    if target in ENTRIES:
     admissions.append(dict(caller=entry,site=i.address,target=target));reached.add(target)
    elif target not in known and target not in trace.code:
     outside.setdefault(f'{target:08x}',[]).append(dict(site=i.address,later_target=target,owner=entry,tail=i.mnemonic=='jmp'))
  instructions.extend(body);mnemonics.update(i.mnemonic for i in body)
 assert reached==ENTRIES-{0x1da750} and len(instructions)==1314
 result=dict(parent);result.update(schema=5,parity_target='English TU1.5 only',parent_sha256=PARENT_PIN,
  extension_tool_sha256=sha(__file__),tu15_owned_xbe_sha256=NEW_PIN,complete_bodies=158,
  parent_native_entries=sorted(int(p,16) for p in parent['entries']),additional_native_entries=sorted(ENTRIES),
  tu15_new_owners=rows,new_call_bindings=admissions,phantom_virtual_bindings=virtuals,new_recovery_requires_older_image=False,new_source_instruction_ids='TU1.5 virtual addresses')
 result['entries']=dict(parent['entries']);result['entries'].update({f'{e:08x}':e for e in ENTRIES})
 result['instruction_map']=dict(parent['instruction_map']);assert not {i.address for i in instructions}&{int(p,16) for p in result['instruction_map']}
 result['instruction_map'].update({f'{i.address:08x}':i.address for i in instructions})
 result['outside_direct_calls']={key:rows for key,rows in parent['outside_direct_calls'].items() if int(key,16) not in ENTRIES and key!='001d5e90'}
 for key,rows in outside.items():result['outside_direct_calls'].setdefault(key,[]);result['outside_direct_calls'][key]+=rows
 result['mnemonics']=dict(collections.Counter(parent['mnemonics'])+mnemonics);assert len(result['instruction_map'])==16402
 (a.output/'report.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(new_bodies=12,new_sites=1314,report_sha256=sha(a.output/'report.json'))))
if __name__=='__main__':main()
