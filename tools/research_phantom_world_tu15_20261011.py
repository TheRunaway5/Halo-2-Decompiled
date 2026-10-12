#!/usr/bin/env python3
"""Bind TU1.5 phantom forces to the existing complete world activation owners."""
import argparse,collections,csv,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
V=ROOT/'analysis/multiplayer/sandbox-source/phantom-force-tu15-20261011/research-verified/report.json'
W=ROOT/'analysis/multiplayer/sandbox-source/physics-world-game-callback-runtime-20261011/correspondence-verified/complete-report.json'
VPIN='273fa35926b21c3ce92b7fb06ce4be716227483c8fb3ce262d36efd78a15c4f5'
WPIN='e4b0a21a7859c727ba3374222465e9f425c63ba70639b13dff13eca441e2ad43'
ENTRIES={0x282870,0x2816a0,0x22d460,0x1d9ec0,0x1f3460}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert sha(V)==VPIN and sha(W)==WPIN and sha(NEW_DEFAULT)==NEW_PIN
 vehicle=json.loads(V.read_text());world=json.loads(W.read_text());image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
 entries=set(vehicle['entries'].values())|set(world['entries'].values())
 sites=set(vehicle['instruction_map'].values())|set(world['instruction_map'].values())
 starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
 # Authenticate the whole TU body union independently of legacy identifiers.
 assert set().union(*(_trace(code,e,starts).code for e in entries))==sites
 assert len(entries)==604 and len(sites)==50530 and not entries&ENTRIES
 bindings=[]
 for site,target,canonical in ((0x315cbb,0x3165d0,0x312a90),(0x315cd4,0x316d40,0x3131c0)):
  i=code.at(site);assert i.mnemonic=='call' and i.operands[0].type==2 and i.operands[0].imm==target
  assert world['entries'][f'{canonical:08x}']==target
  bindings.append(dict(site=site,target=target,world_entry_id=canonical,raw_hex=i.bytes.hex()))
 # Bind the callbacks through complete TU constructors and untouched table
 # words. Runtime observations alone are not ownership evidence.
 virtuals=[]
 for constructor,store,table,offset,target in ((0x1cb8e0,0x1cbe4b,0x466cbc,4,0x282870),
   (0x281550,0x28155a,0x466c70,8,0x2816a0),(0x1d9ad0,0x1d9ae2,0x461970,0x1c,0x31d520)):
  trace=_trace(code,constructor,starts);i=code.at(store)
  assert constructor in entries and store in trace.code and set(trace.code)<=sites
  assert i.mnemonic=='mov' and i.operands[0].type==3 and i.operands[1].type==2 and i.operands[1].imm==table
  raw=image.read(table+offset,4);assert struct.unpack('<I',raw)[0]==target
  virtuals.append(dict(constructor=constructor,store=store,table=table,offset=offset,target=target,
   store_raw_hex=i.bytes.hex(),slot_raw_hex=raw.hex()))
 owners=[];outside={};new_sites=set();mnemonics=collections.Counter()
 for entry in sorted(ENTRIES):
  trace=_trace(code,entry,starts);body=[code.at(p) for p in sorted(trace.code)]
  tables=[dict(address=p,width=w,count=n,raw_hex=image.read(p,w*n).hex()) for p,w,n in trace.tables]
  owners.append(dict(entry=entry,parent_entry_id=entry,instructions=len(body),tables=tables))
  for i in body:
   if i.mnemonic in ('call','jmp') and i.operands[0].type==2:
    target=i.operands[0].imm
    if target not in entries|ENTRIES|set(trace.code):
     outside.setdefault(f'{target:08x}',[]).append(dict(site=i.address,target=target,owner=entry,tail=i.mnemonic=='jmp'))
  new_sites|=set(trace.code);mnemonics.update(i.mnemonic for i in body)
 assert len(new_sites)==477 and not new_sites&sites
 entries|=ENTRIES;sites|=new_sites
 a.output.mkdir(parents=True,exist_ok=False)
 listing=a.output/'tu15-instructions.tsv';listing.write_text(''.join(f'{p:08x}\t{code.at(p).bytes.hex()}\t{code.at(p).mnemonic}\t{code.at(p).op_str}\n' for p in sorted(sites)))
 report=dict(schema=2,parity_target='English TU1.5 only',extension_tool_sha256=sha(__file__),original_sha256=NEW_PIN,
  parent_vehicle_sha256=VPIN,parent_world_sha256=WPIN,complete_bodies=len(entries),distinct_sites=len(sites),
  entries={f'{p:08x}':p for p in sorted(entries)},instruction_map={f'{p:08x}':p for p in sorted(sites)},address_map={},
  vehicle_entry_ids=vehicle['entries'],world_entry_ids=world['entries'],
  vehicle_instruction_ids=vehicle['instruction_map'],world_instruction_ids=world['instruction_map'],
  original_bindings=bindings,virtual_bindings=virtuals,tu15_new_owners=owners,new_sites=sorted(new_sites),
  outside_direct_calls=outside,mnemonics=dict(mnemonics),listing_file=listing.name,listing_sha256=sha(listing),
  source_identifiers='TU1.5 addresses only, normalized per compiled owner',new_native_bodies=len(ENTRIES),
  older_image_executed=False,old_checkpoint_used=False,live_physics_proof=False)
 (a.output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(bodies=len(entries),sites=len(sites),new_bodies=len(ENTRIES),new_sites=len(new_sites),report_sha256=sha(a.output/'report.json'))))
if __name__=='__main__':main()
