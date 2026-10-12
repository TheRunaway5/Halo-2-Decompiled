#!/usr/bin/env python3
"""Full instruction correspondence for native footstep authored consumers."""
import sys,csv,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/reference/halo2-decompiled/tools')]
from xbe import Xbe
from functions import _Code,_trace
from research_mp_original_data_u5 import norm
from research_mp_live_locomotion_owners_u5 import raw_prefix_candidates
images=[Xbe(ROOT/'assets/disc/default.xbe'),Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),Xbe(Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe'))]
codes=[_Code(x,'.text')for x in images]
starts=[{int(r['address'],16)for r in csv.DictReader((ROOT/p).open(),delimiter='\t')}for p in('analysis/retail/functions.tsv','analysis/map-pack-update/functions.tsv','analysis/multiplayer/title-update-1.5/decompiled/functions.tsv')]
output=ROOT/'analysis/multiplayer/integration/live-authored-audio-20261010/footstep-correspondence';output.mkdir(parents=True,exist_ok=True)
rows=[]
for entry in(0xe5040,0xe4d90,0x187fe0,0x188090,0x188130,0x1882d0,0x1883e0):
 f=_trace(codes[0],entry,starts[0]);old=[codes[0].at(a)for a in sorted(f.code)];pos={i.address:n for n,i in enumerate(old)};tokens=[norm(i,images[0],pos)for i in old];r=dict(base=f'{entry:08x}',rows=len(old),images=[])
 for n in(1,2):
  candidates=raw_prefix_candidates(codes[0],images[0],codes[n],entry,12);matches=[]
  for p in candidates:
   try:
    q=_trace(codes[n],p,starts[n]);ins=[codes[n].at(a)for a in sorted(q.code)];np={i.address:k for k,i in enumerate(ins)}
    if [norm(i,images[n],np)for i in ins]==tokens:matches.append((p,ins))
   except Exception:pass
  assert len(matches)==1,(hex(entry),n,[hex(a)for a in candidates],[hex(a)for a,_ in matches])
  p,ins=matches[0];operands=[]
  for a,b in zip(old,ins):
   if a.mnemonic=='call'and a.op_str.startswith('0x'):operands.append(dict(kind='call',base_site=f'{a.address:08x}',site=f'{b.address:08x}',base=a.op_str,target=b.op_str))
   for x,y in zip(a.operands,b.operands):
    if x.type==y.type==3 and not x.mem.base and not y.mem.base:
     try:oldbytes=images[0].read(x.mem.disp,x.size);newbytes=images[n].read(y.mem.disp,y.size)
     except ValueError:continue
     operands.append(dict(kind='memory',base_site=f'{a.address:08x}',site=f'{b.address:08x}',base=f'{x.mem.disp:08x}',target=f'{y.mem.disp:08x}',size=x.size,base_hex=oldbytes.hex(),hex=newbytes.hex()))
  name=f'{entry:08x}-{n}.asm';(output/name).write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));r['images'].append(dict(image=n,entry=f'{p:08x}',complete_normalized_body_equal=True,operands=operands,file=name))
 rows.append(r);print(hex(entry),[v['entry']for v in r['images']],flush=True)
(output/'report.json').write_text(json.dumps(dict(images=[dict(path=str(x.path),sha256=hashlib.sha256(x.data).hexdigest())for x in images],owners=rows),indent=2,sort_keys=True)+'\n')
