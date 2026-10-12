#!/usr/bin/env python3
"""Original-only retained BSP child/triangle producer fixtures."""
import argparse,json
from pathlib import Path
from oracle_mp_power_child_agents_1_1 import ROOT,verify,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from export_mp_physics_contact_lifecycle import add,segments
from export_mp_power_body_slots import full_snapshot
from oracle_smg_firing_1_1 import Xbe
SELECT={'002efc30','002ef8f0','002ef970','002efa10','001c6c30','002e5e20','002ef680','002f48a0','002e13b0','00305560','00307fe0'}
# Complete source owning temporary stores, excluding saved registers/ABI/SEH.
SEMANTIC={0x3055e0,0x305617,0x305620,0x305624,0x30562a,0x30562d,0x305637,0x305642,0x2e42da,0x2e42f9,0x2e42fc,0x2e430f,0x2e4312,0x2ef990,0x2ef992,0x2ef995,0x2ef9e2,0x2ef9e9,0x2ef9ec,0x2ef9ee,0x2ef95d,0x2ef932,0x2ef938,0x1c6c47,0x1c6ce4,0x2e13e1,0x2f48c6,
 0x3055b9,0x3055e3,0x30560a,0x2e0ae0,0x2e0b4d,0x2e0be5,0x2e0d26,
 0x2e22f3,0x2e2311,0x2e2314,0x303802,0x303828,0x303838,0x303842,0x303892,0x30392b,0x1f1d66}

from export_mp_power_bsp_children import SEMANTIC as BSP_SEMANTIC
from export_mp_power_current_broadphase import SEMANTIC as CURRENT_SEMANTIC
SEMANTIC.update(BSP_SEMANTIC|CURRENT_SEMANTIC)
SEMANTIC.update({0x2e5e63,0x2e5e42,0x1c6d1b,0x3037c9,0x3037d1,0x3037da,0x3037dd,0x2e4368,0x303965,0x30396b,0x303972,0x30397b,0x1f1d91})
SEMANTIC.update({0x2efc71,0x2efc8f,0x2efca2,0x2efccb,0x2efcf1,0x2efd0b,0x2efd2d,0x2efd34,0x2efd3b,0x2efd6e,0x2efd72,0x2efd76,0x2f0021,0x2f005b,0x2f0062,0x2f006e,0x2f00b3,0x2f00d1,0x2f011c,0x2f0123,0x2f01db,0x2f01df})

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
  for p,n in((0x303988,0x2b),(0x1f2ebc,7*4),(0x418c00,0x30)):
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
   count=2
   rows.append(' '.join(['B',boundary['call_site'],boundary['entry'],boundary['registers']['ecx'],boundary['entry_esp'],'0',str(count)]+boundary['stack16'][:count]))
  audit.append(dict(call_id=c['id'],entry=c['entry'],input_bytes=len(memory),stores=len(writes),excluded_abi_stack_and_fs_stores=len(ignored),retained_snapshot_correlated_bytes=matched,complete_original_call=boundary is None,required_original_owner=None if boundary is None else dict(call_id=boundary['id'],entry=boundary['entry'],instruction=boundary['call_site'],entry_esp=boundary['entry_esp'],receiver=boundary['registers']['ecx'])))
 fixture=a.output/'fixtures.tsv';frozen_write(fixture,('\n'.join(rows)+'\n').encode());frozen_write(a.output/'report.json',encoded(dict(schema=1,source_report_sha256=sha(a.report),exporter_sha256=sha(__file__),fixture_sha256=sha(fixture),cases=audit,native_expected_data_used=False,prefix_expectations='Literal original read/store timelines and whole real retained before/after buffers. No native expected data used.')))
 print(json.dumps(dict(cases=len(audit),complete=sum(q['complete_original_call']for q in audit),required_stops=sum(not q['complete_original_call']for q in audit),stores=sum(c['stores']for c in audit),fixture=sha(fixture))))
if __name__=='__main__':main()
