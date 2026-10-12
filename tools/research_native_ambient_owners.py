#!/usr/bin/env python3
"""Pair complete ambient caller/phase-selection bodies and literal phase tables."""
import sys,csv,json,hashlib,argparse
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/reference/halo2-decompiled/tools')]
from xbe import Xbe
from functions import _Code,_trace
from research_mp_original_data_u5 import norm
from research_mp_live_locomotion_owners_u5 import raw_prefix_candidates
xs=[Xbe(ROOT/'assets/disc/default.xbe'),Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),Xbe(Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe'))];codes=[_Code(x,'.text')for x in xs]
ss=[{int(r['address'],16)for r in csv.DictReader((ROOT/p).open(),delimiter='\t')}for p in('analysis/retail/functions.tsv','analysis/map-pack-update/functions.tsv','analysis/multiplayer/title-update-1.5/decompiled/functions.tsv')]
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);out=parser.parse_args().output;out.mkdir(parents=True,exist_ok=True);rows=[]
for e in (0x21a250,0x18a7b0):
 f=_trace(codes[0],e,ss[0]);old=[codes[0].at(a)for a in sorted(f.code)];pos={i.address:n for n,i in enumerate(old)};t=[norm(i,xs[0],pos)for i in old];row=dict(base=e,instructions=len(old),images=[])
 (out/f'{e:08x}-base.asm').write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in old))
 for n in(1,2):
  candidates=raw_prefix_candidates(codes[0],xs[0],codes[n],e,12);matches=[]
  for a in candidates:
   try:
    q=_trace(codes[n],a,ss[n]);ins=[codes[n].at(z)for z in sorted(q.code)];p={i.address:k for k,i in enumerate(ins)}
    if [norm(i,xs[n],p)for i in ins]==t:matches.append((a,ins))
   except Exception:pass
  assert len(matches)==1,(hex(e),n,[hex(a)for a in candidates],len(matches))
  a,ins=matches[0];(out/f'{e:08x}-{n}.asm').write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));row['images'].append(dict(entry=a,normalized_complete_equal=True,pair=[dict(old=x.address,new=y.address)for x,y in zip(old,ins)]))
 rows.append(row);print(hex(e),[hex(i['entry'])for i in row['images']],flush=True)
tables=[]
for site in(0x21a79f,0x21a7a8,0x21a76b):
 row=dict(site=site,images=[])
 for n in range(3):
  a=site if n==0 else next(v['new']for v in rows[0]['images'][n-1]['pair']if v['old']==site)
  instruction=codes[n].at(a);address=instruction.operands[1].mem.disp;raw=xs[n].read(address,28)
  row['images'].append(dict(site=a,address=address,bytes_hex=raw.hex()))
 assert len({i['bytes_hex']for i in row['images']})==1
 tables.append(row)
(out/'report.json').write_text(json.dumps(dict(tables=tables,images=[dict(path=str(x.path),sha256=hashlib.sha256(x.data).hexdigest())for x in xs],owners=rows),indent=2)+'\n')
