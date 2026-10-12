#!/usr/bin/env python3
"""Export actual complete retained slot owners; no native expected output."""
import argparse,json
from pathlib import Path
from oracle_mp_power_body_slots_1_1 import ROOT,OUT,verify,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from export_mp_physics_contact_lifecycle import add,segments
from export_mp_power_body_bridge import snapshot
from oracle_smg_firing_1_1 import Xbe
SELECT={'001d3a70','001dc0c0','001d29a0'}
def full_snapshot(state):
 r=snapshot(state)
 if 'object_pointer'in state:r.append((int(state['object_pointer'],16),bytes.fromhex(state['object_382_hex'])))
 return r
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);args=ap.parse_args();verify()
 r=json.loads(args.report.read_text());image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');rows=[];audit=[]
 for c in r['calls']:
  if c['entry']not in SELECT:continue
  memory={}
  for a,b in full_snapshot(r['states'][c['before_state']]):add(memory,a,b)
  for p in c['packets']:add(memory,int(p['pointer'],16),bytes.fromhex(p['before_hex']))
  current=dict(memory);writes=[];ignored=[]
  events=sorted([('R',x)for x in r['reads']if c['order']<x['order']<c['return_order']]+[('W',x)for x in r['stores']if c['order']<x['order']<c['return_order']],key=lambda x:x[1]['order'])
  for kind,e in events:
   a=int(e['address'],16);raw=bytes.fromhex(e['raw_hex']if kind=='R'else e['before_hex'])
   for n,b in enumerate(raw):
    if a+n not in current:memory[a+n]=b;current[a+n]=b
    assert current[a+n]==b,(c['id'],kind,e)
   if kind=='W':
    (ignored if e['source_stack']else writes).append(e)
    for n,b in enumerate(bytes.fromhex(e['written_hex'])):current[a+n]=b
  # Getter execution consumes actual constructed motion vtables. Import the
  # original complete slot words even when the trace only reads one of them.
  for p in(0x41b7f0,0x41b8f0):add(memory,p,image.read(p,0x60))
  after=dict(memory)
  for e in writes:
   a=int(e['address'],16);assert bytes(after[a+n]for n in range(e['size']))==bytes.fromhex(e['before_hex'])
   for n,b in enumerate(bytes.fromhex(e['written_hex'])):after[a+n]=b
  matched=0
  for a,b in full_snapshot(r['states'][c['after_state']]):
   for n,v in enumerate(b):
    if a+n in after:assert after[a+n]==v,(c['id'],f'{a+n:08x}');matched+=1
  rows.append(' '.join(['C',f'call_{c["id"]}_{c["entry"]}',c['entry'],c['registers']['eax'],c['registers']['ecx'],c['registers']['edi'],c['returned_eax']]+c['stack16']))
  for a,b in segments(memory):rows.append(f'M {a:08x} {b.hex()}')
  for a,b in segments(after):rows.append(f'A {a:08x} {b.hex()}')
  for e in writes:rows.append(f'W {e["instruction"]} {e["address"]} {e["before_hex"]} {e["written_hex"]}')
  audit.append(dict(call_id=c['id'],entry=c['entry'],input_bytes=len(memory),stores=len(writes),excluded_internal_stack=len(ignored),retained_snapshot_correlated_bytes=matched))
 fixture=args.output/'fixtures.tsv';frozen_write(fixture,('\n'.join(rows)+'\n').encode());result=dict(schema=1,source_report_sha256=sha(args.report),exporter_sha256=sha(__file__),fixture_sha256=sha(fixture),cases=audit,native_expected_data_used=False)
 frozen_write(args.output/'report.json',encoded(result));print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture=sha(fixture))))
if __name__=='__main__':main()
