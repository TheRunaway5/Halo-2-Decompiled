#!/usr/bin/env python3
"""Export literal callback/binding evidence and unequal complete BSP owners."""
import argparse,csv,hashlib,json,sys,struct
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,_Code,_trace
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,raw):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==raw
 else:p.write_bytes(raw)
def encoded(v):return(json.dumps(v,sort_keys=True,indent=2)+'\n').encode()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--comparison',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 d=json.loads(a.comparison.read_text());capacity=next(q for q in d['owners']if q['old_entry']=='00124980')
 originals=dict(cache_pointer_slot=0x4eb124,physical_index=0x4ed9f0,physical_lower_table=0x4ed9fc,physical_upper_table=0x4eda10,requested_bytes_slot=0x4eb128,block_capacity_slot=0x4eb12c,backing_pointer_slot=0x4eb120)
 bindings={}
 for name,va in originals.items():
  matches=[q for q in capacity['image_memory_operands']if int(q['old_va'],16)==va];assert matches
  addresses={q['u5_va']for q in matches};assert len(addresses)==1
  bindings[name]=dict(old_va=f'{va:08x}',u5_va=next(iter(addresses)),read_or_store_sites=matches,scope='Runtime address tokens only. Image zeros are not initialization/readiness/residency defaults.')
 bodies=[];images=[]
 for label,path,inventory,entry in [('old',OLD,ROOT/'analysis/map-pack-update/functions.tsv',0x124290),('u5',NEW,ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv',0x1255c0)]:
  x=Xbe(path);images.append(x);code=_Code(x,'.text');starts={int(q['address'],16)for q in csv.DictReader(inventory.open(),delimiter='\t')};f=_trace(code,entry,starts);ins=[code.at(at)for at in sorted(f.code)]
  for i in ins:assert x.read(i.address,i.size)==i.bytes
  raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode();name=f'instructions/{entry:08x}-{label}.asm';save(a.output/name,raw)
  bodies.append(dict(image=label,entry=f'{entry:08x}',file=name,instructions=len(ins),sha256=sha(a.output/name),direct_calls=[dict(site=f'{s:08x}',target=f'{t:08x}')for s,t,k in f.sites if k=='call']))
 callback=images[0].read(0x45ad78,64);records=[]
 for i in range(8):
  release,create=struct.unpack_from('<2I',callback,i*8);records.append(dict(index=i,release=f'{release:08x}',create=f'{create:08x}'))
 assert records[6]['release']=='00124a30'and records[6]['create']=='00124980'
 v=dict(schema='animation-residency-boundary-v1',tool_sha256=sha(__file__),comparison_sha256=sha(a.comparison),source_pins={str(path):sha(path)for path in(OLD,NEW)},capacity_bindings=bindings,callback_table=dict(old_va='0045ad78',raw64_hex=callback.hex(),records=records),unequal_bsp_resource_bodies=bodies,scope='Raw callback6 and selected-image bindings.124290/1255C0 complete bodies are retained separately and are not claimed equivalent; additional u5 calls require distinct ownership. No u5 runtime or resource/IO success supplied.')
 save(a.output/'report.json',encoded(v));print(json.dumps(dict(report=str(a.output/'report.json'),sha256=sha(a.output/'report.json'),bodies=[q['instructions']for q in bodies])))
if __name__=='__main__':main()
