#!/usr/bin/env python3
"""Recover the reached TU1.5 vehicle contact-state owner from owned TU1.5 bytes."""
import argparse,collections,csv,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

PARENT=ROOT/'analysis/multiplayer/sandbox-source/vehicle-motion-math-20261011/correspondence-verified/report.json'
PARENT_PIN='2bde9d480d6b09c949b2cf09667b28828b6f211188d9523a36f692ff29a777b1'
ENTRY_ID=0x20b470  # Retained parent identifier; recovery and execution use TU1.5 only.
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert sha(PARENT)==PARENT_PIN and sha(NEW_DEFAULT)==NEW_PIN
 parent=json.loads(PARENT.read_text());image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
 starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
 a.output.mkdir(parents=True,exist_ok=False)
 owners=[];all_instructions=[];bindings={}
 for entry_id in (0x20b3b0,ENTRY_ID):
  calls=parent['outside_direct_calls'][f'{entry_id:08x}'];actual={c['later_target'] for c in calls};assert len(actual)==1
  entry=next(iter(actual));bindings[entry_id]=entry
  for call in calls:
   site=parent['instruction_map'][f"{call['site']:08x}"];i=code.at(site)
   assert i.mnemonic=='call' and i.operands[0].type==2 and i.operands[0].imm==entry
 third_calls=[]
 for entry_id,entry in list(bindings.items()):
  for site in _trace(code,entry,starts).code:
   i=code.at(site)
   if i.mnemonic=='call':
    assert entry==0x20fcb0 and i.operands[0].imm==0x22e1b0
    third_calls.append(dict(site=site,target=i.operands[0].imm))
 assert len(third_calls)==1;bindings[0x22e1b0]=third_calls[0]['target']
 external={}
 for entry_id,entry in sorted(bindings.items()):
  assert entry in starts
  trace=_trace(code,entry,starts);instructions=[code.at(p) for p in sorted(trace.code)]
  assert len(instructions)=={0x20fcb0:61,0x20fd70:65,0x22e1b0:34}[entry] and not trace.tables
  assert all(i.operands[0].imm in trace.code for i in instructions if i.mnemonic.startswith('j'))
  assert instructions[-1].mnemonic=='ret'
  listing=''.join(f'{i.address:08x}\t{i.bytes.hex()}\t{i.mnemonic}\t{i.op_str}\n' for i in instructions)
  listing_path=a.output/f'tu15-{entry:08x}.tsv';listing_path.write_text(listing)
  globals=[]
  for i in instructions:
   if i.mnemonic=='call':
    target=i.operands[0].imm
    if target not in bindings.values():external.setdefault(f'{target:08x}',[]).append(dict(site=i.address,later_target=target,owner=entry_id))
   for op in i.operands:
    if op.type==3 and not op.mem.base and not op.mem.index:
     raw=image.read(op.mem.disp,op.size)
     row=dict(site=i.address,address=op.mem.disp,width=op.size,original_hex=raw.hex())
     if i.mnemonic=='mov' and i.reg_name(i.operands[0].reg)=='edi' and op.size==4:
      pointer=int.from_bytes(raw,'little');row.update(pointer=pointer,pointee_hex=image.read(pointer,12).hex())
     globals.append(row)
  owners.append(dict(entry=entry,parent_entry_id=entry_id,instructions=len(instructions),
   listing_file=listing_path.name,listing_sha256=sha(listing_path),globals=globals))
  all_instructions.extend(instructions)
 result=dict(parent)
 result.update(schema=2,parity_target='English TU1.5 only',parent_sha256=PARENT_PIN,
  extension_tool_sha256=sha(__file__),tu15_owned_xbe_sha256=NEW_PIN,
  parent_native_entries=sorted(int(p,16) for p in parent['entries']),additional_native_entries=sorted(bindings),
  complete_bodies=99,tu15_new_owners=owners,new_call_bindings=third_calls,
  new_source_instruction_ids='TU1.5 virtual addresses',new_recovery_requires_older_image=False)
 result['entries']=dict(parent['entries']);result['entries'].update({f'{p:08x}':q for p,q in bindings.items()})
 result['instruction_map']=dict(parent['instruction_map'])
 assert not {i.address for i in all_instructions}&{int(p,16) for p in result['instruction_map']}
 result['instruction_map'].update({f'{i.address:08x}':i.address for i in all_instructions})
 result['outside_direct_calls']=dict(parent['outside_direct_calls'])
 for entry_id in (0x20b3b0,ENTRY_ID):del result['outside_direct_calls'][f'{entry_id:08x}']
 result['outside_direct_calls'].update(external)
 result['mnemonics']=dict(collections.Counter(parent['mnemonics'])+collections.Counter(i.mnemonic for i in all_instructions))
 assert len(result['instruction_map'])==11216
 (a.output/'report.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(tu15_entries=[hex(p) for p in bindings.values()],instructions=160,composed_bodies=99,report_sha256=sha(a.output/'report.json'))))
if __name__=='__main__':main()
