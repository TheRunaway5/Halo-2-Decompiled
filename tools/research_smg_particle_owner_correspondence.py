#!/usr/bin/env python3
"""Read-only complete particle owner correspondence in three owned images."""
import argparse,csv,json,struct
from pathlib import Path
from research_mp_inventory_switch_u5 import prefix,raw_prefix_candidates,norm
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,_Code,_trace
from oracle_smg_effect_owner_1_1 import sha,save,encoded

BASE=ROOT/'assets/disc/default.xbe'
IMAGES=(BASE,OLD,NEW)
INVENTORIES=(ROOT/'analysis/retail/functions.tsv',ROOT/'analysis/map-pack-update/functions.tsv',ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv')
PINS={str(BASE):'03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d',str(OLD):'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',str(NEW):'21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'}
OWNERS=((0x247470,0x249540),(0x247950,0x249a20),(0x249170,0x24b240))

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 for path,pin in PINS.items():assert sha(path)==pin,path
 images=[Xbe(path)for path in IMAGES];codes=[_Code(x,'.text')for x in images]
 starts=[{int(q['address'],16)for q in csv.DictReader(path.open(),delimiter='\t')}for path in INVENTORIES]
 buckets={}
 for e in sorted(starts[2]):buckets.setdefault(tuple(prefix(codes[2],images[2],e)),[]).append(e)
 rows=[]
 for b,e in OWNERS:
  seq=[];routines=[]
  for k,entry in enumerate((b,e)):
   routine=_trace(codes[k],entry,starts[k]);ins=[codes[k].at(q)for q in sorted(routine.code)];seq.append(ins);routines.append(routine)
  forms=[]
  for k,ins in enumerate(seq):forms.append([norm(i,images[k],{q.address:n for n,q in enumerate(ins)})for i in ins])
  assert forms[0]==forms[1],(hex(b),hex(e),'base/1.1 body differs')
  candidates=sorted(set(buckets.get(tuple(prefix(codes[1],images[1],e)),[])+raw_prefix_candidates(codes[1],images[1],codes[2],e)));matches=[]
  for candidate in candidates:
   r=_trace(codes[2],candidate,starts[2]);ins=[codes[2].at(q)for q in sorted(r.code)]
   if [norm(i,images[2],{q.address:n for n,q in enumerate(ins)})for i in ins]==forms[1]:matches.append((candidate,r,ins))
  assert len(matches)==1,(hex(e),[hex(q[0])for q in matches])
  candidate,r,ins=matches[0];seq.append(ins);routines.append(r)
  addresses=[{i.address:n for n,i in enumerate(s)}for s in seq];tables=[]
  assert len({len(r.tables)for r in routines})==1
  for ts in zip(*(r.tables for r in routines)):
   values=[]
   for k,(pointer,width,count)in enumerate(ts):
    assert width==4;raw=images[k].read(pointer,width*count);targets=struct.unpack('<'+'I'*count,raw)
    values.append(dict(va=f'{pointer:08x}',raw_hex=raw.hex(),targets=[f'{q:08x}'for q in targets],target_instruction_indices=[addresses[k][q]for q in targets]))
   assert values[0]['target_instruction_indices']==values[1]['target_instruction_indices']==values[2]['target_instruction_indices'];tables.append(values)
  listings=[]
  for k,label in enumerate(('disc_base','map_pack_1_1','title_update_1_5')):
   for i in seq[k]:assert images[k].read(i.address,i.size)==i.bytes
   path=a.output/'instructions'/f'{e:08x}-{label}.asm';save(path,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in seq[k]).encode());listings.append(dict(file=str(path.relative_to(a.output)),rows=len(seq[k]),sha256=sha(path)))
  memory=[];calls=[]
  for triple in zip(*seq):
   if triple[0].mnemonic=='call'and all(i.operands[0].type==2 for i in triple):calls.append([dict(site=f'{i.address:08x}',target=f'{i.operands[0].imm:08x}')for i in triple])
   for operands in zip(*(i.operands for i in triple)):
    if all(o.type==3 and not o.mem.base and not o.mem.index and o.mem.disp for o in operands):
     memory.append([dict(site=f'{i.address:08x}',va=f'{o.mem.disp:08x}',size=o.size,raw_hex=images[k].read(o.mem.disp,o.size).hex())for k,(i,o)in enumerate(zip(triple,operands))])
  row=dict(disc_base=f'{b:08x}',map_pack_1_1=f'{e:08x}',title_update_1_5=f'{candidate:08x}',rows=len(seq[0]),complete_normalized_body_and_internal_branch_equal=True,unique_u5_full_body_match=True,jump_tables=tables,paired_memory_operands=memory,paired_direct_calls=calls,listings=listings)
  if e==0x249a20:
   at=addresses[1][0x249efa];assert seq[0][at].address==0x247e2a;row['successful_prefix_checkpoints']=[f'{s[at].address:08x}'for s in seq]
  rows.append(row);print(json.dumps({k:row[k]for k in('disc_base','map_pack_1_1','title_update_1_5','rows')}),flush=True)
 for path,pin in PINS.items():assert sha(path)==pin,path
 report=dict(schema='source-smg-particle-owner-correspondence-v1',tool_sha256=sha(__file__),source_pins=PINS,source_inventory_pins={str(path):sha(path)for path in INVENTORIES},owners=rows,scope='Complete original static instruction/internal branch/jump-table correspondence only. Actual execution in the separately frozen report is TU1.1; no actual TU1.5 particle call, graphics submission, Xbox transcendental/RSQRT hardware parity or external-helper equivalence inferred.',instruction_patches=[],production_edits=[])
 save(a.output/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(a.output/'report.json'))))
if __name__=='__main__':main()
