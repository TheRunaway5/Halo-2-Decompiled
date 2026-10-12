#!/usr/bin/env python3
"""Export literal original retained world-island state, never native expectations."""
import argparse,json
from pathlib import Path
from oracle_mp_power_world_insertion_1_1 import ROOT,OUT,verify,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from export_mp_physics_contact_lifecycle import add,segments
from export_mp_power_body_slots import full_snapshot
from oracle_smg_firing_1_1 import Xbe
SELECT={'00315820','00312dd0','0031d730','0031d250','0031e610'}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();verify()
 r=json.loads(a.report.read_text());im=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');rows=[];audit=[];fs=set()
 for q in r['instruction_listings']:
  for line in(a.report.parent/q['file']).read_text().splitlines():
   if 'fs:[0]'in line:fs.add(int(line.split()[0],16))
 for c in r['calls']:
  if c['entry']not in SELECT:continue
  # The outer315820 fixture stops at its real next owning3144D0 entry.
  # No backend output is imported into the native service or expected state.
  if c['entry']=='00315820':
   q=[q for q in r['calls']if q['entry']=='003144d0'and c['order']<q['order']<c['return_order']]
   assert len(q)==1;c={**c,'return_order':q[0]['order'],'after_state':q[0]['before_state']}
  scratch=[0,0]
  memory={}
  for p,b in full_snapshot(r['states'][c['before_state']]):add(memory,p,b)
  for q in c['packets']:
   if 'captured_at'not in q:add(memory,int(q['pointer'],16),bytes.fromhex(q['before_hex']))
  current=dict(memory);writes=[];ignored=[]
  events=sorted([('R',e)for e in r['reads']if c['order']<e['order']<c['return_order']]+[('W',e)for e in r['stores']if c['order']<e['order']<c['return_order']],key=lambda x:x[1]['order'])
  for kind,e in events:
   p=int(e['address'],16);b=bytes.fromhex(e['raw_hex']if kind=='R'else e['before_hex'])
   for n,v in enumerate(b):
    if p+n not in current:memory[p+n]=v;current[p+n]=v
    assert current[p+n]==v,(c['id'],kind,e)
   if kind=='W':
    included=int(e['instruction'],16)not in fs and not e['source_stack']
    (writes if included else ignored).append(e)
    for n,v in enumerate(bytes.fromhex(e['written_hex'])):current[p+n]=v
  # Required virtual identities are original executable data. Include whole
  # slots independently of the native code, including the small free provider.
  for p in(0x41b610,0x41b7f0,0x41b8f0):add(memory,p,im.read(p,0x20))
  for e in writes:
   if e['source_stack']:
    p=int(e['address'],16)
    for n,v in enumerate(bytes.fromhex(e['before_hex'])):memory[p+n]=v
  after=dict(memory)
  for e in writes:
   p=int(e['address'],16);logical=bytes(after[p+n]for n in range(e['size']))
   assert e['source_stack']or logical==bytes.fromhex(e['before_hex']),(c['id'],e)
   for n,v in enumerate(bytes.fromhex(e['written_hex'])):after[p+n]=v
  matched=0
  for p,b in full_snapshot(r['states'][c['after_state']]):
   for n,v in enumerate(b):
    if p+n in after:assert after[p+n]==v,(c['id'],f'{p+n:08x}');matched+=1
  rows.append(' '.join(['C',f'call_{c["id"]}_{c["entry"]}',c['entry'],c['registers']['eax'],c['registers']['ecx'],c['returned_eax']]+[f'{v:08x}'for v in scratch]+c['stack16']))
  for p,b in segments(memory):rows.append(f'M {p:08x} {b.hex()}')
  for p,b in segments(after):rows.append(f'A {p:08x} {b.hex()}')
  for e in writes:rows.append(f'{"WS"if e["source_stack"]else"W"} {e["instruction"]} {e["address"]} {e["before_hex"]} {e["written_hex"]}')
  audit.append(dict(call_id=c['id'],entry=c['entry'],input_bytes=len(memory),stores=len(writes),excluded_abi_stack_and_fs_stores=len(ignored),retained_snapshot_correlated_bytes=matched,stopped_at='003144d0'if c['entry']=='00315820'else None))
 fixture=a.output/'fixtures.tsv';frozen_write(fixture,('\n'.join(rows)+'\n').encode());frozen_write(a.output/'report.json',encoded(dict(schema=1,source_report_sha256=sha(a.report),exporter_sha256=sha(__file__),fixture_sha256=sha(fixture),cases=audit,native_expected_data_used=False)))
 print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture=sha(fixture))))
if __name__=='__main__':main()
