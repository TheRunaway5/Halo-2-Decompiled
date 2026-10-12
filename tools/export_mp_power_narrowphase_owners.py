#!/usr/bin/env python3
"""Export actual original narrowphase owner inputs/stores; never a contact replay callback."""
import argparse,json
from pathlib import Path
from oracle_mp_power_narrowphase_owners_1_1 import ROOT,verify,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from export_mp_physics_contact_lifecycle import add,segments
from export_mp_power_body_slots import full_snapshot
from oracle_smg_firing_1_1 import Xbe
SELECT={'002e1430','002e14e0','002f4bd0','00309860','002e0c70'}
SEMANTIC={0x2e14f0,0x2e14ff,0x2e1504,0x2e1508,0x2e155a,0x2e1676,0x2e1681,
 0x2e15f7,0x2e160f,0x2e1617,0x2e171b,0x2e173a,0x2e173e,0x2e1780,
 0x2f4bf4,0x2f4bf8,0x2f4c78,0x2f4cfd,0x2f4d33,0x2f4d50,0x2f4d7f,0x2f4d95,0x3098aa}
SEMANTIC.update({0x2e1455,0x2e1621,0x2e1748,0x2e15ec,0x2e160b,0x2e1710,0x2e1736,
 0x2e1762,0x2e17ce,0x2e17f4,0x2e1805,0x2e180c,0x2f4c89,0x2f4d5c,0x2f4dac,
 0x2f4e1e,0x2f4e22,0x2f4e44,0x2f4e55,0x2f4e5d,0x309873,0x309883,0x30988f,
 0x309896,0x3098c8,0x3098cf,0x3098e1,0x3098e8,0x2e0cae})
from export_mp_power_child_agents import SEMANTIC as CHILD_SEMANTIC
SEMANTIC.update(CHILD_SEMANTIC)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();verify()
 r=json.loads(a.report.read_text());im=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');rows=[];audit=[]
 jobs=[]
 for c in r['calls']:
  if c['entry']not in SELECT:continue
  boundary=next((q for q in r['calls']if c['order']<q['order']<c['return_order']and q['entry']=='0030ae40'),None)if c['entry']in('002e14e0','002f4bd0')else None
  jobs.append((c,boundary,False))
  if c['entry']=='002f4bd0':jobs.append((c,boundary,'separation'))
  if c['entry']=='002e14e0':
   child=next(q for q in r['calls']if c['order']<q['order']<c['return_order']and q['entry']=='002f4bd0')
   jobs.append((c,child,'motion'))
 for c,boundary,resume in jobs:
  begin=boundary['return_order']if resume else c['order'];end=c['return_order']if resume or not boundary else boundary['order']
  memory={};state=boundary['after_state']if resume else c['before_state']
  for p,b in full_snapshot(r['states'][state]):add(memory,p,b)
  for q in(boundary if resume else c)['packets']:add(memory,int(q['pointer'],16),bytes.fromhex(q['after_hex'if resume else'before_hex']))
  current=dict(memory);writes=[];ignored=[]
  events=sorted([('R',e)for e in r['reads']if begin<e['order']<end]+[('W',e)for e in r['stores']if begin<e['order']<end],key=lambda x:x[1]['order'])
  for kind,e in events:
   p=int(e['address'],16);b=bytes.fromhex(e['raw_hex']if kind=='R'else e['before_hex'])
   for n,v in enumerate(b):
    if p+n not in current:memory[p+n]=v;current[p+n]=v
    assert current[p+n]==v,(c['id'],resume,kind,e)
   if kind=='W':
    site=int(e['instruction'],16)
    (writes if not e['source_stack']or site in SEMANTIC else ignored).append(e)
    for n,v in enumerate(bytes.fromhex(e['written_hex'])):current[p+n]=v
  after=dict(memory)
  for e in writes:
   p=int(e['address'],16);logical=bytes(after[p+n]for n in range(e['size']))
   assert e['source_stack']or logical==bytes.fromhex(e['before_hex']),(c['id'],e)
   for n,v in enumerate(bytes.fromhex(e['written_hex'])):after[p+n]=v
  matched=0
  if not boundary or resume:
   for p,b in full_snapshot(r['states'][c['after_state']]):
    for n,v in enumerate(b):
     if p+n in after:assert after[p+n]==v,(c['id'],resume,f'{p+n:08x}');matched+=1
  entry=('002e17ed'if resume=='motion'else'002f4d0a')if resume else c['entry'];eax=boundary['returned_eax']if resume else c['registers']['eax']
  rows.append(' '.join(['C',f'call_{c["id"]}_{entry}',entry,eax,c['registers']['ecx'],c['returned_eax'],c['entry_esp'],c['x87_control_word'],c['mxcsr'],c['registers']['edx'],c['registers']['esi'],c['registers']['edi']]+c['stack16']))
  for p,b in segments(memory):rows.append(f'M {p:08x} {b.hex()}')
  for p,b in segments(after):rows.append(f'A {p:08x} {b.hex()}')
  for e in writes:rows.append(f'{"WS"if e["source_stack"]else"W"} {e["instruction"]} {e["address"]} {e["before_hex"]} {e["written_hex"]}')
  if boundary and not resume:
   site='002f4d05';count=5
   rows.append(' '.join(['B',site,boundary['entry'],boundary['registers']['ecx'],boundary['entry_esp'],'0',str(count)]+boundary['stack16'][:count]))
  audit.append(dict(call_id=c['id'],entry=entry,input_bytes=len(memory),stores=len(writes),excluded_abi_stack_stores=len(ignored),retained_snapshot_correlated_bytes=matched,complete_original_call=boundary is None,complete_original_continuation=bool(resume),continuation_kind=resume or None,required_original_owner=None if not boundary or resume else dict(call_id=boundary['id'],entry=boundary['entry'],instruction='002f4d05',entry_esp=boundary['entry_esp'],receiver=boundary['registers']['ecx']),continuation_input_provenance=None if not resume else dict(completed_original_call_id=boundary['id'],completed_original_entry=boundary['entry'],returned_eax=boundary['returned_eax'],workspace_packets=boundary['packets'])))
 fixture=a.output/'fixtures.tsv';frozen_write(fixture,('\n'.join(rows)+'\n').encode());frozen_write(a.output/'report.json',encoded(dict(schema=1,source_report_sha256=sha(a.report),exporter_sha256=sha(__file__),fixture_sha256=sha(fixture),cases=audit,native_expected_data_used=False,contact_callback_outputs_exported=False,prefix_expectations='Literal original timelines/full retained buffers; required GJK stops. Continuation inputs come from actual completed original GJK, never native-derived expected data or a replay service.')))
 print(json.dumps(dict(cases=len(audit),complete=sum(q['complete_original_call']for q in audit),continuations=sum(q['complete_original_continuation']for q in audit),required_stops=sum(q['required_original_owner']is not None for q in audit),stores=sum(c['stores']for c in audit),fixture=sha(fixture))))
if __name__=='__main__':main()
