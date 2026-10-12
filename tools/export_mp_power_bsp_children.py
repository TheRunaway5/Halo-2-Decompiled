#!/usr/bin/env python3
"""Original-only retained BSP child/triangle producer fixtures."""
import argparse,json
from pathlib import Path
from oracle_mp_power_bsp_children_1_1 import ROOT,verify,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from export_mp_physics_contact_lifecycle import add,segments
from export_mp_power_body_slots import full_snapshot
from oracle_smg_firing_1_1 import Xbe
SELECT={'001f2420','002e5bc0','003066e0','00305e40','001f26b0','001f2db0','001f3320','001f16f0','002e0a60'}
# Original owning packet/temp stores, excluding ABI pushes/saved registers.
SEMANTIC={0x1f26c1,0x1f1740,0x1f1749,0x1f17e1,0x1f180a,0x1f1838,0x2e0ae0,0x2e0b4d}
SEMANTIC.update({0x3061c6,0x3061d3,0x3061e0,0x3061e9,0x3061fb,0x306206,0x306213,0x306224,0x30622b,0x30623b,0x306242,0x306249,0x30626c,0x306270,0x306298,0x30629c,0x3062d6,0x3062da,0x3062ea,0x306300,0x30632b,0x30634a,0x306350})
SEMANTIC.update({0x3066ef,0x3066f4,0x306710,0x30672f,0x30674f,0x30676f,0x30678f,0x3067cd,0x30641b,0x306424,0x1f26db,0x1f26ea,0x3066ff,0x306707,0x30671e,0x306726,0x30673e,0x306746,0x30675e,0x306766,0x30677e,0x306786,0x30679e,0x3067a6,0x3067b2,0x3067bc,0x3067c4,0x3067d3,0x3067dc,0x3067e0,0x3067ef,0x3067f3,0x3067f7,0x3067fb,0x3067ff,0x306803})
SEMANTIC.update({0x1f170c,0x1f170f,0x1f1716,0x1f1723,0x1f1726,0x1f172c,0x1f1731,0x1f1737,0x1f1857,0x1f185c,0x1f1861,0x1f1866,0x1f1879,0x1f1885,0x1f189d,0x1f18aa,0x1f18ad,0x1f346d,0x1f3473,0x1f347a})

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--phase');ap.add_argument('--call',type=int);a=ap.parse_args();verify()
 r=json.loads(a.report.read_text());im=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');rows=[];audit=[];fs=set()
 for q in r['instruction_listings']:
  for line in(a.report.parent/q['file']).read_text().splitlines():
   if 'fs:[0]'in line:fs.add(int(line.split()[0],16))
 for c in r['calls']:
  if c['entry']not in SELECT or(a.phase and c['phase']!=a.phase)or(a.call is not None and c['id']!=a.call):continue
  boundary=None
  target=None
  if target:
   boundary=next(q for q in r['calls']if c['order']<q['order']<c['return_order']and q['entry']==target)
  end=boundary['order']if boundary else c['return_order']
  memory={}
  for p,b in full_snapshot(r['states'][c['before_state']]):add(memory,p,b)
  for q in c['packets']:add(memory,int(q['pointer'],16),bytes.fromhex(q['before_hex']))
  current=dict(memory);writes=[];ignored=[]
  events=sorted([('R',e)for e in r['reads']if c['order']<e['order']<end]+[('W',e)for e in r['stores']if c['order']<e['order']<end],key=lambda x:x[1]['order'])
  for kind,e in events:
   p=int(e['address'],16);b=bytes.fromhex(e['raw_hex']if kind=='R'else e['before_hex'])
   for n,v in enumerate(b):
    if p+n not in current:memory[p+n]=v;current[p+n]=v
    assert current[p+n]==v,(c['id'],kind,e)
   if kind=='W':
    site=int(e['instruction'],16)
    (writes if site not in fs and(not e['source_stack']or site in SEMANTIC)else ignored).append(e)
    for n,v in enumerate(bytes.fromhex(e['written_hex'])):current[p+n]=v
  # Exact vtable words needed by native complete owner validation. Values are
  # original executable bytes, not a chosen callback or native output.
  for p,n in((0x1f2ebc,7*4),):
   add(memory,p,im.read(p,n))
  after=dict(memory)
  for e in writes:
   p=int(e['address'],16);logical=bytes(after[p+n]for n in range(e['size']))
   assert e['source_stack']or logical==bytes.fromhex(e['before_hex']),(c['id'],e)
   for n,v in enumerate(bytes.fromhex(e['written_hex'])):after[p+n]=v
  matched=0
  if not boundary:
   for p,b in full_snapshot(r['states'][c['after_state']]):
    for n,v in enumerate(b):
     if p+n in after:assert after[p+n]==v,(c['id'],f'{p+n:08x}');matched+=1
  rows.append(' '.join(['C',f'call_{c["id"]}_{c["entry"]}',c['entry'],c['registers']['eax'],c['registers']['ecx'],c['returned_eax'],c['entry_esp'],c['x87_control_word'],c['mxcsr'],c['registers']['edx'],c['registers']['esi'],c['registers']['edi']]+c['stack16']))
  for p,b in segments(memory):rows.append(f'M {p:08x} {b.hex()}')
  for p,b in segments(after):rows.append(f'A {p:08x} {b.hex()}')
  for e in writes:rows.append(f'{"WS"if e["source_stack"]else"W"} {e["instruction"]} {e["address"]} {e["before_hex"]} {e["written_hex"]}')
  if boundary:
   count=4 if c['entry']=='0031dee0'else 2
   rows.append(' '.join(['B',boundary['call_site'],boundary['entry'],boundary['registers']['ecx'],boundary['entry_esp'],'0',str(count)]+boundary['stack16'][:count]))
  audit.append(dict(call_id=c['id'],entry=c['entry'],input_bytes=len(memory),stores=len(writes),excluded_abi_stack_and_fs_stores=len(ignored),retained_snapshot_correlated_bytes=matched,complete_original_call=boundary is None,required_original_owner=None if boundary is None else dict(call_id=boundary['id'],entry=boundary['entry'],instruction=boundary['call_site'],entry_esp=boundary['entry_esp'],receiver=boundary['registers']['ecx'])))
 fixture=a.output/'fixtures.tsv';frozen_write(fixture,('\n'.join(rows)+'\n').encode());frozen_write(a.output/'report.json',encoded(dict(schema=1,source_report_sha256=sha(a.report),exporter_sha256=sha(__file__),fixture_sha256=sha(fixture),cases=audit,native_expected_data_used=False,prefix_expectations='Literal original read/store timelines and whole real retained before/after buffers. No native expected data used.')))
 print(json.dumps(dict(cases=len(audit),complete=sum(q['complete_original_call']for q in audit),required_stops=sum(not q['complete_original_call']for q in audit),stores=sum(c['stores']for c in audit),fixture=sha(fixture))))
if __name__=='__main__':main()
