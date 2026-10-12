#!/usr/bin/env python3
"""Freeze unchanged original callback/contact outputs; never infer MOPP order.

Run the read-only census and original oracles documented in
SOURCE_INSTANCE_CONTACT_TIES.md first. These reports retain the pinned XBE,
explicit cold breakable state, and controlled placement boundaries.
"""
from pathlib import Path
import hashlib,json,struct,sys
sys.path.insert(0,str(Path(__file__).parent));from inspect_multiplayer import Cache
ROOT=Path(__file__).resolve().parents[1]
report=ROOT/'analysis/multiplayer-shading/instance-tie-original.json'
control=ROOT/'analysis/multiplayer-shading/instance-tie-controls-original.json'
a=json.loads(report.read_text());b=json.loads(control.read_text());assert a['xbe_sha256']==b['xbe_sha256']=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
f=lambda v:struct.unpack('<I',struct.pack('<f',v))[0]
NONE=0xffffffff
caches={name:Cache(ROOT/('assets/disc/maps' if name=='headlong' else 'assets/map-pack/maps')/(name+'.map')) for name in ('headlong','triplicate')}
def definition(name,index):
 c=caches[name];e=c.block(c.scenario+0x210,0x44)[0];start,memory=c.u(e),c.u(e+8);magic=memory-start;ptr=c.u(start+16+0x144)-magic;return struct.unpack_from('<H',c.data,ptr+index*88+52)[0]
rows=[];seen=set()
for r in a['rows']:
 name=r['map'];key=(name,*map(f,r['origin']),*map(f,r['ray']))
 if key in seen:continue
 seen.add(key);owner=r['orders'][0][-1];first=r['single'][str(owner['owner_instance'])]['full']
 words=[1,int(name=='triplicate'),NONE,NONE,NONE,*map(f,r['origin']),*map(f,r['ray']),1,owner['owner_instance'],r['expected_fraction_bits'],first['leaf'],owner['surface'],1,len(r['owners'])]
 for i in r['owners']:
  q=r['single'][str(i)];x=q['full'];assert x['hit']==1 and x['fraction_bits']==r['expected_fraction_bits'];words += [i,definition(name,i),x['fraction_bits'],x['leaf'],x['surface'],q['exact']['hit'],q['next']['hit']]
 rows.append((f"actual_{name}_{len(rows)}",words))
for r in b['cases']:
 owner=r['order'][-1];static=r.get('static_contact');besttype=int(owner[1]==3);bestid=owner[3] if besttype else NONE;best=next((q for q in r['queries'] if q[4]==bestid),None)
 words=[1,0,r['main_definition'] if r['main_definition'] is not None else NONE,r['new_copy'] if r['new_copy'] is not None else NONE,r['override_instance'],*r['override_words'],*map(f,r['origin']),*map(f,r['ray']),besttype,bestid,owner[2],best[2] if best else static['leaf'],best[3] if best else static['surface'],int(r['label']=='tie_then_farther'),len(r['queries'])]
 for q in r['queries']:
  i=q[4];df=definition('headlong',r['new_copy'] if r['new_copy'] is not None and i==r['override_instance'] else i)
  # The control verifies bounded first-owner retention independently through
  # the original callback trace. Per-instance +1ULP controls are actual-only.
  exact=r['equal_bound'][0] if 'equal_bound' in r else NONE
  words += [i,df,q[1],q[2],q[3],exact,NONE]
 rows.append((r['label'],words))
fixture=ROOT/'tests/fixtures/retail_instance_contact_ties.tsv';fixture.write_text('# unchanged original1691A0/183FC0/1DE630/244C10; explicit ordered callback controls, no accelerator order claim\n'+'\n'.join(label+'\t'+' '.join(f'{w:08x}' for w in words) for label,words in rows)+'\n')
provenance={'version':1,'source_xbe_sha256':a['xbe_sha256'],'actual_unique_rays':len(seen),'controlled_rays':len(b['cases']),'fixtures':len(rows),'original_authored_tie_observations':a['cases'],'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'reports':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [report,control]},'maps':{name:hashlib.sha256(c.path.read_bytes()).hexdigest() for name,c in caches.items()},'scope':'Actual authored contacts and unchanged original per-instance/callback arithmetic. Explicit callback order is supplied; source3009E0 accelerator order remains unresolved. Three controlled placement/static-bank cases are not claimed actual map state. Explicit cold all-one breakable bank. No guest IO/GPU.'}
fixture.with_suffix('.provenance.json').write_text(json.dumps(provenance,indent=2)+'\n');print(json.dumps(provenance,indent=2))
