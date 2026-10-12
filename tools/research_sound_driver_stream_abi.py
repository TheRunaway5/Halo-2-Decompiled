#!/usr/bin/env python3
"""Authenticate stream-format CRT and queue methods through their real callers/vtables."""
import argparse,csv,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,PINS,Xbe,_Code,_trace,sha,save,encoded
from research_mp_inventory_switch_u5 import norm
PAIR=ROOT/'analysis/multiplayer/integration/sound-driver-stream-native/paired'
PAIR_PIN='781933decd99123c63996cdea1900a527247db159750f9dd74711dbe8d254646'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert sha(PAIR/'report.json')==PAIR_PIN
 packet=json.loads((PAIR/'report.json').read_text())
 format_owner=next(q for q in packet['owners']if q['old']=='00220520')
 ftol=next(q for q in format_owner['calls']if q['old_target']=='0032909c')
 images=[Xbe(q)for q in(OLD,NEW)];codes=[_Code(q,'.text')for q in images]
 inventories=[ROOT/'analysis/map-pack-update/functions.tsv',ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv']
 starts=[{int(q['address'],16)for q in csv.DictReader(path.open(),delimiter='\t')}for path in inventories]
 queue_addresses=[(0x4865d8,0x4865f8),(0x48b8e8,0x48b908)]
 records=[]
 for index,(path,image,code)in enumerate(zip((OLD,NEW),images,codes,strict=True)):
  assert sha(path)==PINS[str(path)]
  rows=[]
  for category,queue in zip(('pcm','adpcm'),queue_addresses[index],strict=True):
   vtable=int.from_bytes(image.read(queue,4),'little');entry=int.from_bytes(image.read(vtable,4),'little')
   rows.append(dict(owner=category,queue=f'{queue:08x}',vtable=f'{vtable:08x}',entry=f'{entry:08x}',
                    queue_pointer_hex=image.read(queue,4).hex(),method_pointer_hex=image.read(vtable,4).hex()))
  rows.append(dict(owner='ftol',entry=ftol['old_target'if index==0 else'new_target']))
  for row in rows:
   entry=int(row['entry'],16);body=_trace(code,entry,starts[index]);ins=[code.at(at)for at in sorted(body.code)]
   assert ins and all(q.bytes==image.read(q.address,len(q.bytes))for q in ins)
   listing=a.output/'instructions'/f'{row["owner"]}-{index}.asm'
   save(listing,''.join(f'{q.address:08x} {q.bytes.hex()} {q.mnemonic} {q.op_str}\n'for q in ins).encode())
   row.update(rows=len(ins),listing=str(listing.relative_to(a.output)),listing_sha256=sha(listing))
  records.append(dict(path=str(path),sha256=sha(path),owners=rows))
 for left,right in zip(records[0]['owners'],records[1]['owners'],strict=True):
  sequences=[]
  for index,row in enumerate((left,right)):
   entry=int(row['entry'],16);body=_trace(codes[index],entry,starts[index]);ins=[codes[index].at(at)for at in sorted(body.code)];positions={q.address:k for k,q in enumerate(ins)}
   sequences.append([norm(q,images[index],positions)for q in ins])
  assert sequences[0]==sequences[1],left['owner']
 report=dict(schema='source-driver-stream-method-abi-v1',tool_sha256=sha(__file__),paired_report_sha256=PAIR_PIN,
             images=records,complete_method_bodies_equal=True,platform_returns=[],
             limits=['Queue addresses are original literal mutable instances, not runtime-created owners.',
                     'Static method/CRT authentication only; no source stream creation or device result.'])
 save(a.output/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),owners=3)))
if __name__=='__main__':main()
