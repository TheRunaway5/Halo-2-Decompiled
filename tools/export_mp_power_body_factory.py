#!/usr/bin/env python3
"""Export genuine factory inputs/stores; no native-produced expectations."""
import argparse,json,hashlib
from pathlib import Path
from oracle_mp_power_body_factory_1_1 import ROOT,OUT,verify,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from export_mp_physics_contact_lifecycle import add,segments
from export_mp_power_body_bridge import snapshot
from oracle_smg_firing_1_1 import Xbe
FS_SITES=set();OUTPUT_SITES=set()
PACKET_SITES={0x1dab44,0x1dab51,0x1dab5f,0x1dab6b,0x1dab77,0x1dab86,0x1dab93,0x1daba0,0x1dabac,0x1dabb0,0x1dabb7,0x1dabd5,0x1dabe3,0x1dabe7,0x1dac03,0x1dac27,0x1dac4c,0x1dac8b}
OUTPUT_ENTRIES={0x317890,0x31c1a0,0x31c110,0x2e0860,0x1d1c00}
SELECT={'001daac0','00318100','00317ea0','00320cc0','00321320','00378ab0','002e0860','001dc750','00303e30'}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,default=OUT/'measured/report.json');ap.add_argument('--output',type=Path,default=OUT/'exported');args=ap.parse_args();verify();r=json.loads(args.report.read_text());rows=[];audits=[];image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 for q in r['instruction_listings']:
  for line in (args.report.parent/q['file']).read_text().splitlines():
   if 'fs:[0]'in line:FS_SITES.add(line.split()[0])
   if int(q['entry'],16)in OUTPUT_ENTRIES:
    parts=line.split(' ',3)
    dest=parts[-1].split(',')[0]
    if '['in dest and '[esp'not in dest and '[ebp'not in dest:OUTPUT_SITES.add(parts[0])
 for c in r['calls']:
  if c['entry']not in SELECT:continue
  children=[x for x in r['calls']if c['order']<=x['order']and x['return_order']<=c['return_order']]
  def owned(entry,caller=None):
   q=[x for x in children if x['entry']==entry and(caller is None or x['caller']==caller)]
   assert len(q)<=1,(c['id'],entry);return int(q[0]['owned_pointer'],16)if q else 0
  info=owned('00317890');quat=owned('001dc750')
  def bound(caller):
   q=[x for x in children if x['entry']=='00303e30'and x['caller']==caller];assert len(q)<=1
   return int(q[0]['output_pointer'],16)if q else 0
  aux,rad=bound('0031abf4'),bound('00317d30');spans=[]
  for p,n in((info,0xb8),(quat,16),(aux,32),(rad,32)):
   if p:spans.append((p,n))
  if 'owned_pointer'in c:spans.append((int(c['owned_pointer'],16),c['owned_size']))
  if 'output_pointer'in c:spans.append((int(c['output_pointer'],16),c['output_size']))
  memory={}
  for a,b in snapshot(r['states'][c['before_state']]):add(memory,a,b)
  for k,p in(('owned_before_hex','owned_pointer'),('output_before_hex','output_pointer'),('info_b8_hex','info_pointer')):
   if k in c:add(memory,int(c[p],16),bytes.fromhex(c[k]))
  for p in(0x41b8f0,0x41b7f0,0x45d188):add(memory,p,image.read(p,0x60))
  current=dict(memory);native=[];ignored=[]
  events=sorted([('R',x)for x in r['reads']if c['order']<x['order']<c['return_order']]+[('W',x)for x in r['stores']if c['order']<x['order']<c['return_order']],key=lambda x:x[1]['order'])
  for kind,e in events:
   a=int(e['address'],16);raw=bytes.fromhex(e['raw_hex']if kind=='R'else e['before_hex'])
   for n,b in enumerate(raw):
    if a+n not in current:memory[a+n]=b;current[a+n]=b
    assert current[a+n]==b,(c['id'],kind,e)
   if kind=='W':
    owner=r['calls'][e['call']];site=int(e['instruction'],16)
    produced=site in PACKET_SITES or site==0x1dc7b5 or e['instruction']in OUTPUT_SITES or (owner['entry']=='00378ab0'and site>=0x378b24)
    included=(not e['source_stack'] and e['instruction']not in FS_SITES) or (produced and any(p<=a and a+e['size']<=p+n for p,n in spans))
    (native if included else ignored).append(e)
    for n,b in enumerate(bytes.fromhex(e['written_hex'])):current[a+n]=b
  # AABB scratch is uninitialized caller stack whose earlier bytes are
  # changed by excluded call-frame plumbing. Admit its actual owning-call
  # entry bytes as scratch input, never as geometry/contact output.
  for q in children:
   if q['entry']=='00303e30':
    p=int(q['output_pointer'],16)
    for n,b in enumerate(bytes.fromhex(q['output_before_hex'])):memory[p+n]=b
  after=dict(memory)
  for e in native:
   a=int(e['address'],16);assert bytes(after[a+n]for n in range(e['size']))==bytes.fromhex(e['before_hex']),(c['id'],e)
   for n,b in enumerate(bytes.fromhex(e['written_hex'])):after[a+n]=b
  matched=0
  for a,b in snapshot(r['states'][c['after_state']]):
   for n,x in enumerate(b):
    if a+n in after:assert after[a+n]==x,(c['id'],f'{a+n:08x}');matched+=1
  math=[x for x in r['math']if c['order']<x['order']<c['return_order']];estimates=[]
  for left,right in zip(math[::2],math[1::2]):
   assert left['instruction']=='001dc782'and right['instruction']=='001dc786'
   estimates.append((int(left['xmm0'][-8:],16),int(right['xmm1'][-8:],16)))
  rows.append(' '.join(['C',f'call_{c["id"]}_{c["entry"]}',c['entry'],c['registers']['eax'],c['registers']['ecx'],c['registers']['edi'],c['returned_eax'],c['x87_control_word'],c['mxcsr']]+[f'{x:08x}'for x in(info,quat,aux,rad)]+c['stack16']))
  for a,b in segments(memory):rows.append(f'M {a:08x} {b.hex()}')
  for a,b in segments(after):rows.append(f'A {a:08x} {b.hex()}')
  for e in native:rows.append(f'W {e["instruction"]} {e["address"]} {e["before_hex"]} {e["written_hex"]}')
  for operand,result in estimates:rows.append(f'Q {operand:08x} {result:08x}')
  audits.append(dict(call_id=c['id'],entry=c['entry'],input_bytes=len(memory),stores=len(native),excluded_internal_stack=len(ignored),retained_snapshot_matched_bytes=matched,rsqrtss_estimates=estimates))
 shape_report=OUT/'shape-measured-fp/report.json'
 if shape_report.exists():
  sr=json.loads(shape_report.read_text());c=sr['call'];memory={};add(memory,int(c['model_pointer'],16),bytes.fromhex(c['raw_before_hex']))
  for e in sr['reads']:add(memory,int(e['address'],16),bytes.fromhex(e['raw_hex']))
  native=[e for e in sr['stores']if not e['source_stack']];after=dict(memory)
  for e in native:
   a=int(e['address'],16);assert bytes(after[a+n]for n in range(e['size']))==bytes.fromhex(e['before_hex'])
   for n,b in enumerate(bytes.fromhex(e['written_hex'])):after[a+n]=b
  assert bytes(after[int(c['model_pointer'],16)+n]for n in range(c['model_size']))==bytes.fromhex(c['raw_after_hex'])
  rows.append(' '.join(['C','selected_convex_model','001ee380',c['registers']['eax'],c['registers']['ecx'],c['registers']['edi'],c['returned_eax'],c['x87_control_word'],c['mxcsr']]+['00000000']*4+c['stack16']))
  for a,b in segments(memory):rows.append(f'M {a:08x} {b.hex()}')
  for a,b in segments(after):rows.append(f'A {a:08x} {b.hex()}')
  for e in native:rows.append(f'W {e["instruction"]} {e["address"]} {e["before_hex"]} {e["written_hex"]}')
  audits.append(dict(entry='001ee380',input_bytes=len(memory),stores=len(native),source_report_sha256=sha(shape_report),retained_snapshot_matched_bytes=c['model_size']))
 fixture=args.output/'fixtures.tsv';frozen_write(fixture,('\n'.join(rows)+'\n').encode());report=dict(schema=1,source_report_sha256=sha(args.report),exporter_sha256=sha(__file__),fixture_sha256=sha(fixture),cases=audits,native_expected_data_used=False,rsqrtss_estimates='Measured actual original emulator instruction boundary only, test-only; not a live Xbox platform provider.')
 frozen_write(args.output/'report.json',encoded(report));print(json.dumps(dict(cases=len(audits),stores=sum(x['stores']for x in audits),fixture=sha(fixture))))
if __name__=='__main__':main()
