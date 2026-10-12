#!/usr/bin/env python3
"""Export actual source body-matrix inputs/outputs; no native-derived geometry."""
import argparse
import json
from pathlib import Path
import audit_mp_spartan_death_physical_1_1 as audit
from oracle_smg_firing_1_1 import ROOT, Xbe

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--directory',type=Path,required=True)
 ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 r=json.loads((a.directory/'report.json').read_text())
 assert audit.sha(ROOT/'tools/oracle_mp_spartan_body_matrix_1_1.py')==r['oracle_sha256']
 assert r['entire_frozen_constructor_byte_identical'] and not r['new_guest_inputs_or_results_supplied'] and not r['instruction_patches']
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 listed=0
 for listing in r['instructions']:
  p=a.directory/listing['file'];assert audit.sha(p)==listing['sha256']
  for line in p.read_text().splitlines():
   at,raw,*_=line.split(' ',2);assert image.read(int(at,16),len(bytes.fromhex(raw)))==bytes.fromhex(raw);listed+=1
 rows=[];projection=[]
 for index,(c,s,call)in enumerate(zip(r['cases'],r['stages'],r['calls'])):
  assert c['slot_index']==index and c['completed'] and s['completed'] and not s['used_services']
  assert call['entry']=='001d42d0' and call['return_esp_delta']==4
  scratch=((int(call['entry_esp'],16)-4)&~15)-0x40
  stores=r['stores'][c['store_begin']:c['store_end']]
  reads=r['reads'][c['read_begin']:c['read_end']]
  assert len(stores)==22
  body=int(c['body'],16);motion=int(c['motion'],16);slot=int(c['slot'],16);component=int(c['component'],16);out=int(c['output'],16)
  assert int(call['returned_eax'],16)==motion+0x80
  const=next(x for x in reads if x['address']=='00465030')['raw_hex'];assert bytes.fromhex(const)==image.read(0x465030,4)
  initial=bytearray(0x40);final=bytearray(0x40)
  for w in stores:
   at=int(w['address'],16)
   if scratch<=at<scratch+0x40:
    off=at-scratch;initial[off:off+w['size']]=bytes.fromhex(w['before_hex']);final[off:off+w['size']]=bytes.fromhex(w['written_hex'])
  inputs={component:bytes.fromhex(c['component_a0_hex']),slot:bytes.fromhex(c['slot60_hex']),
   body:bytes.fromhex(c['bodyc0_hex']),motion:bytes.fromhex(c['motionc0_hex']),scratch:bytes(initial),
   out:bytes.fromhex(c['before_output34_hex']),0x465030:bytes.fromhex(const)}
  after=audit.segments(s['after'])
  def actual(p,n):
   for base,raw in after.values():
    if base<=p and p+n<=base+len(raw):return raw[p-base:p-base+n]
   raise AssertionError((p,n))
  outputs={component:actual(component,0xa0),slot:actual(slot,0x60),body:actual(body,0xc0),motion:actual(motion,0xc0),
   scratch:bytes(final),out:bytes.fromhex(c['after_output34_hex']),0x465030:bytes.fromhex(const)}
  rows.append(f'C {index} {component:08x} {out:08x} {scratch:08x}')
  for p,raw in sorted(inputs.items()):rows.append(f'M {p:08x} {raw.hex()}')
  for p,raw in sorted(outputs.items()):rows.append(f'A {p:08x} {raw.hex()}')
  projected=[]
  for w in stores:
   at=int(w['address'],16)
   if not(scratch<=at and at+w['size']<=scratch+0x40 or out<=at and at+w['size']<=out+0x34):
    assert w['instruction']=='001d42d0' and w['size']==4;continue
   rows.append(f"W {w['instruction']} {w['address']} {w['before_hex']} {w['written_hex']}")
   projected.append(w)
  assert len(projected)==21
  projection.append(dict(slot=index,raw_stores=22,projected_stores=21,omitted_prologue_push_ebp=1,
   compared_store_bytes=sum(w['size']for w in projected)))
 raw=('\n'.join(rows)+'\n').encode();a.output.parent.mkdir(parents=True,exist_ok=True)
 if a.output.exists():assert a.output.read_bytes()==raw
 else:a.output.write_bytes(raw)
 metadata=dict(schema=1,exporter_sha256=audit.sha(__file__),report_sha256=audit.sha(a.directory/'report.json'),
  fixture_sha256=audit.sha(a.output),original_instruction_rows=listed,cases=len(projection),projection=projection,
  native_expected_geometry_used=False,bit6_correction_original_execution_covered=False)
 p=a.output.with_suffix('.json');raw=audit.encoded(metadata)
 if p.exists():assert p.read_bytes()==raw
 else:p.write_bytes(raw)
 print(json.dumps(metadata))
if __name__=='__main__':main()
