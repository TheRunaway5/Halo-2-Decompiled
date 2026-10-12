#!/usr/bin/env python3
"""Export only actual original broadphase inputs/state/stores, never native data."""
import argparse,json
from pathlib import Path
from oracle_mp_power_broadphase_1_1 import ROOT,OUT,verify,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from export_mp_physics_contact_lifecycle import add,segments
from export_mp_power_body_slots import full_snapshot
from oracle_smg_firing_1_1 import Xbe
SELECT={'002e8a00','002e7500','002e6240','002e7da0','002e8090','00313c90','00313e50'}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();verify()
 r=json.loads(a.report.read_text());im=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');rows=[];audit=[];fs=set()
 for q in r['instruction_listings']:
  for line in(a.report.parent/q['file']).read_text().splitlines():
   if 'fs:[0]'in line:fs.add(int(line.split()[0],16))
 for c in r['calls']:
  chosen=c['entry']in SELECT or(c['entry']in('003144d0','002e2380','00303e30')and(c['registers']['ecx']=='0601ad20'or c['registers']['ecx']=='8025cc40'or(c['entry']=='003144d0'and c['stack16'][0]=='0601a480')))
  if not chosen:continue
  scratch=[0,0];stop=None;owned=[]
  descendants=[q for q in r['calls']if c['order']<q['order']<c['return_order']]
  if c['entry']=='003144d0':
   q=[q for q in descendants if q['entry']in('00378eb0','0031d980')];assert len(q)>=1;q=q[0]
   stop=q['entry'];c={**c,'return_order':q['order'],'after_state':q['before_state']}
   scratch[0]=int(next(x['pointer']for x in c['packets']if x['name']=='owning_frame_8c'),16)
   owned.extend([(scratch[0]+12,20),(scratch[0]+32,32),(scratch[0]+0x4c,20)])
   b=next(q for q in descendants if q['entry']=='002e8a00');scratch[1]=int(next(x['pointer']for x in b['packets']if x['name']=='owning_frame_8c'),16)+0x30
   owned.append((scratch[1],20))
  elif c['entry']=='002e8a00':
   scratch[1]=int(next(x['pointer']for x in c['packets']if x['name']=='owning_frame_8c'),16)+0x30
   owned.extend([(scratch[1],20),(int(c['stack16'][2],16),20)])
  elif c['entry']in('002e7da0','002e8090','00313c90','00313e50'):owned.append((int(c['registers']['ecx'],16),20))
  elif c['entry']in('002e2380','00303e30'):owned.append((int(c['stack16'][2],16),32))
  memory={}
  for p,b in full_snapshot(r['states'][c['before_state']]):add(memory,p,b)
  for q in c['packets']:
   if 'captured_at'not in q:add(memory,int(q['pointer'],16),bytes.fromhex(q['before_hex']))
  current=dict(memory);writes=[];ignored=[]
  events=sorted([('R',e)for e in r['reads']if c['order']<e['order']<c['return_order']]+[('W',e)for e in r['stores']if c['order']<e['order']<c['return_order']],key=lambda x:x[1]['order'])
  semantic_sites={0x2e7dc0,0x2e7dc2,0x2e7dc5,0x2e7e09,0x2e7e11,0x2e7e14,0x2e7e1b,0x313cb0,0x313cb2,0x313cb5,0x313cf9,0x313d01,0x313d04,0x313d0b,0x2e8e4f,0x314579,0x314581,0x314585,0x31458d,0x314595,0x31421a}
  for kind,e in events:
   p=int(e['address'],16);b=bytes.fromhex(e['raw_hex']if kind=='R'else e['before_hex'])
   for n,v in enumerate(b):
    if p+n not in current:memory[p+n]=v;current[p+n]=v
    assert current[p+n]==v,(c['id'],kind,e)
   if kind=='W':
    site=int(e['instruction'],16)
    semantic=any(at<=p and p+e['size']<=at+n for at,n in owned)and(site in semantic_sites or site in(0x1d1c66,0x1d1c74,0x1d1cb7,0x1d1cc1,0x2e23e8,0x2e23f5))
    included=site not in fs and(not e['source_stack']or semantic)
    (writes if included else ignored).append(e)
    for n,v in enumerate(bytes.fromhex(e['written_hex'])):current[p+n]=v
  # Include authentic virtual identities and SSE quantization constants,
  # independently of native reads/output and any source-listed branch.
  for p,n in((0x418c00,0x50),(0x45d188,0x20),(0x418930,0x20),(0x46502c,4)):
   add(memory,p,im.read(p,n))
  after=dict(memory)
  for e in writes:
   p=int(e['address'],16);logical=bytes(after[p+n]for n in range(e['size']))
   assert e['source_stack']or logical==bytes.fromhex(e['before_hex']),(c['id'],e)
   for n,v in enumerate(bytes.fromhex(e['written_hex'])):after[p+n]=v
  matched=0
  for p,b in full_snapshot(r['states'][c['after_state']]):
   for n,v in enumerate(b):
    if p+n in after:assert after[p+n]==v,(c['id'],f'{p+n:08x}');matched+=1
  rows.append(' '.join(['C',f'call_{c["id"]}_{c["entry"]}',c['entry'],c['registers']['eax'],c['registers']['ecx'],c['returned_eax']]+[f'{v:08x}'for v in scratch]+[c['x87_control_word'],c['mxcsr']]+c['stack16']))
  for p,b in segments(memory):rows.append(f'M {p:08x} {b.hex()}')
  for p,b in segments(after):rows.append(f'A {p:08x} {b.hex()}')
  for e in writes:rows.append(f'{"WS"if e["source_stack"]else"W"} {e["instruction"]} {e["address"]} {e["before_hex"]} {e["written_hex"]}')
  audit.append(dict(call_id=c['id'],entry=c['entry'],input_bytes=len(memory),stores=len(writes),excluded_abi_stack_and_fs_stores=len(ignored),retained_snapshot_correlated_bytes=matched,stopped_at=stop))
 fixture=a.output/'fixtures.tsv';frozen_write(fixture,('\n'.join(rows)+'\n').encode());frozen_write(a.output/'report.json',encoded(dict(schema=1,source_report_sha256=sha(a.report),exporter_sha256=sha(__file__),fixture_sha256=sha(fixture),cases=audit,native_expected_data_used=False)))
 print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture=sha(fixture))))
if __name__=='__main__':main()
