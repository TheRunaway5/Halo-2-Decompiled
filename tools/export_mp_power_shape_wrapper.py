#!/usr/bin/env python3
"""Export original measured convex wrapper owners; native output is never used."""
import argparse,json
from pathlib import Path
from oracle_mp_power_body_prepare_1_1 import ROOT,OUT,verify,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from export_mp_physics_contact_lifecycle import add,segments
from export_mp_power_body_slots import full_snapshot
from oracle_smg_firing_1_1 import Xbe
SELECT={'001dacf0','001dadf0','001dad80','00184510'}
MATRIX_SITES={0x184528,0x184533,0x18453e,0x184549,0x184554,0x18455f,0x18456a,0x184575,0x18458a,0x18459f,0x1845a5,0x1845ab,0x1845c2}
EXTENT_SITES={0x1845d0,0x1845e1,0x184f5d,0x184f70}
CONFIG_SITES={0x2e5762,0x2e572d,0x2e25a8,0x2e25af,0x2e25b6,0x1dadc0,0x1dadd1,0x1dadd5}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();verify();r=json.loads(a.report.read_text());rows=[];audit=[];image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 for c in r['calls']:
  if c['entry']not in SELECT:continue
  children=[q for q in r['calls']if c['order']<=q['order']and q['return_order']<=c['return_order']]
  def child(entry):
   q=[q for q in children if q['entry']==entry];assert len(q)<=1;return q[0]if q else None
  ext=child('00184510');calc=child('00184800');config=child('002e25a0')
  scratch=[int(ext['registers']['ecx'],16)if ext else 0,int(ext['registers']['edx'],16)if ext else 0,int(calc['stack16'][2],16)if calc else 0,int(config['registers']['ecx'],16)if config else 0]
  spans=[(scratch[0],4,EXTENT_SITES),(scratch[1],4,EXTENT_SITES),(scratch[2],0x40,MATRIX_SITES),(scratch[3],0x14,CONFIG_SITES)]
  def included(e):
   if not e['source_stack']:return True
   p=int(e['address'],16);site=int(e['instruction'],16)
   return any(at and at<=p and p+e['size']<=at+n and site in sites for at,n,sites in spans)
  memory={}
  for p,b in full_snapshot(r['states'][c['before_state']]):add(memory,p,b)
  for q in c['packets']:add(memory,int(q['pointer'],16),bytes.fromhex(q['before_hex']))
  current=dict(memory);native=[];ignored=[];events=sorted([('R',x)for x in r['reads']if c['order']<x['order']<c['return_order']]+[('W',x)for x in r['stores']if c['order']<x['order']<c['return_order']],key=lambda x:x[1]['order'])
  for kind,e in events:
   p=int(e['address'],16);raw=bytes.fromhex(e['raw_hex']if kind=='R'else e['before_hex'])
   for n,b in enumerate(raw):
    if p+n not in current:memory[p+n]=b;current[p+n]=b
    assert current[p+n]==b,(c['id'],kind,e)
   if kind=='W':
    (native if included(e)else ignored).append(e)
    for n,b in enumerate(bytes.fromhex(e['written_hex'])):current[p+n]=b
  # Scratch call-frame bytes may be changed before their first owning store.
  # Retain the measured before bytes at that first owner, not an output value.
  touched=set()
  for e in native:
   if not e['source_stack']:continue
   p=int(e['address'],16)
   for n,b in enumerate(bytes.fromhex(e['before_hex'])):
    if p+n not in touched:memory[p+n]=b;touched.add(p+n)
  for p in (0x41b7f0,0x41b8f0,0x45d188,0x418930):add(memory,p,image.read(p,0x60))
  add(memory,0x1850c8,image.read(0x1850c8,0x20));add(memory,0x1850b8,image.read(0x1850b8,0x10))
  after=dict(memory)
  for e in native:
   p=int(e['address'],16);assert bytes(after[p+n]for n in range(e['size']))==bytes.fromhex(e['before_hex']),(c['id'],e)
   for n,b in enumerate(bytes.fromhex(e['written_hex'])):after[p+n]=b
  matched=0
  for p,b in full_snapshot(r['states'][c['after_state']]):
   for n,v in enumerate(b):
    if p+n in after:assert after[p+n]==v,(c['id'],f'{p+n:08x}');matched+=1
  for q in c['packets']:
   p=int(q['pointer'],16)
   for n,v in enumerate(bytes.fromhex(q['after_hex'])):
    if p+n in touched:assert after[p+n]==v
  rows.append(' '.join(['C',f'call_{c["id"]}_{c["entry"]}',c['entry'],c['registers']['eax'],c['registers']['ecx'],c['registers']['edi'],c['returned_eax'],c['returned_xmm']['0'][-8:],c['x87_control_word'],c['mxcsr']]+[f'{v:08x}'for v in scratch]+c['stack16']))
  for p,b in segments(memory):rows.append(f'M {p:08x} {b.hex()}')
  for p,b in segments(after):rows.append(f'A {p:08x} {b.hex()}')
  for e in native:rows.append(f'W {e["instruction"]} {e["address"]} {e["before_hex"]} {e["written_hex"]}')
  audit.append(dict(call_id=c['id'],entry=c['entry'],input_bytes=len(memory),stores=len(native),excluded_internal_stack=len(ignored),retained_snapshot_correlated_bytes=matched,scratch=scratch))
 f=a.output/'fixtures.tsv';frozen_write(f,('\n'.join(rows)+'\n').encode());frozen_write(a.output/'report.json',encoded(dict(schema=1,exporter_sha256=sha(__file__),original_report_sha256=sha(a.report),fixture_sha256=sha(f),cases=audit,native_expected_data_used=False,source_data_or_admission_supplied=False)))
 print(json.dumps(dict(cases=len(audit),stores=sum(q['stores']for q in audit),fixture=sha(f))))
if __name__=='__main__':main()
