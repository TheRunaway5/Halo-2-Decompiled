#!/usr/bin/env python3
"""Independent original-only pair manager/constructor inputs, buffers and stores."""
import argparse,json
from pathlib import Path
from oracle_mp_power_pair_manager_1_1 import ROOT,verify,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from export_mp_physics_contact_lifecycle import add,segments
from export_mp_power_body_slots import full_snapshot
from oracle_smg_firing_1_1 import Xbe
SELECT={'0031c430','0031c4b0','00312010','00311f80','00378eb0','002e5f50','00314020','003140f0','0031d360','0031d980','00313ef0','00313f80',
 '00379140','0031f770','0031f6c0','00322590','00322060','003220d0','00300170','002efb60','0027da90','0027e4a0'}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();verify()
 r=json.loads(a.report.read_text());im=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');rows=[];audit=[];fs=set()
 for q in r['instruction_listings']:
  for line in(a.report.parent/q['file']).read_text().splitlines():
   if 'fs:[0]'in line:fs.add(int(line.split()[0],16))
 semantic_sites={0x2e5f78,0x2e5f99,0x27daa6,0x313f10,0x313f12,0x313f15,0x313f5a,0x313f62,0x313f65,0x313f6c}
 for c in r['calls']:
  if c['entry']not in SELECT:continue
  frame=int(c['entry_esp'],16)
  if c['entry']=='0031d980':frame=int(next(p['pointer']for p in c['packets']if p['name']=='island_frame_34'),16)
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
    site=int(e['instruction'],16)
    (writes if site not in fs and(not e['source_stack']or site in semantic_sites)else ignored).append(e)
    for n,v in enumerate(bytes.fromhex(e['written_hex'])):current[p+n]=v
  # Original virtual identities independently imported from original bytes,
  # including allocator methods validated by the represented native owner.
  for p,n in((0x480d98,4),(0x45c760,0x10),(0x41b608,0x14),(0x41b6b0,0x14),
   (0x461ba8,0x10),(0x461bc8,0x10),(0x418930,0x20),(0x460fb0,0x20),(0x41b8f0,0x20),(0x41b870,0x20)):
   # Initialized runtime globals are source snapshot authority, not image.
   if p==0x480d98:continue
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
  rows.append(' '.join(['C',f'call_{c["id"]}_{c["entry"]}',c['entry'],c['registers']['eax'],c['registers']['ecx'],c['returned_eax'],f'{frame:08x}']+c['stack16']))
  for p,b in segments(memory):rows.append(f'M {p:08x} {b.hex()}')
  for p,b in segments(after):rows.append(f'A {p:08x} {b.hex()}')
  for e in writes:rows.append(f'{"WS"if e["source_stack"]else"W"} {e["instruction"]} {e["address"]} {e["before_hex"]} {e["written_hex"]}')
  audit.append(dict(call_id=c['id'],entry=c['entry'],input_bytes=len(memory),stores=len(writes),excluded_abi_stack_and_fs_stores=len(ignored),retained_snapshot_correlated_bytes=matched))
 fixture=a.output/'fixtures.tsv';frozen_write(fixture,('\n'.join(rows)+'\n').encode());frozen_write(a.output/'report.json',encoded(dict(schema=1,source_report_sha256=sha(a.report),exporter_sha256=sha(__file__),fixture_sha256=sha(fixture),cases=audit,native_expected_data_used=False)))
 print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture=sha(fixture))))
if __name__=='__main__':main()
