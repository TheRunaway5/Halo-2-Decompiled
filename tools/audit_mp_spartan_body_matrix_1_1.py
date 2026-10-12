#!/usr/bin/env python3
"""Verify repeated original ten-body matrix calls and complete source store replay."""
import argparse
import json
from pathlib import Path
import audit_mp_spartan_death_physical_1_1 as audit
from oracle_smg_firing_1_1 import ROOT, Xbe

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--directory',type=Path,required=True)
 ap.add_argument('--repeat',type=Path,required=True);ap.add_argument('--output',type=Path,required=True)
 a=ap.parse_args();r=json.loads((a.directory/'report.json').read_text())
 files=sorted(p.relative_to(a.directory)for p in a.directory.rglob('*')if p.is_file()and p.name not in ('raw-host-context.json','runtime-stderr.txt'))
 for p in files:assert (a.directory/p).read_bytes()==(a.repeat/p).read_bytes(),p
 assert audit.sha(ROOT/'tools/oracle_mp_spartan_body_matrix_1_1.py')==r['oracle_sha256']
 assert audit.sha(ROOT/'analysis/multiplayer/integration/spartan-ragdoll-constructor-20261010/frozen/manifest.json')==r['prior_manifest_sha256']
 assert r['entire_frozen_constructor_byte_identical'] and not r['new_guest_inputs_or_results_supplied'] and not r['instruction_patches'] and not r['events']
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');instructions=set();listed=0
 for listing in r['instructions']:
  p=a.directory/listing['file'];assert audit.sha(p)==listing['sha256']
  for line in p.read_text().splitlines():
   at,raw,*_=line.split(' ',2);at=int(at,16);raw=bytes.fromhex(raw);assert image.read(at,len(raw))==raw
   instructions.add(at);listed+=1
 for w in r['stores']+r['reads']:assert int(w['instruction'],16)in instructions
 intervals=compared_bytes=0
 for index,(case,stage,call)in enumerate(zip(r['cases'],r['stages'],r['calls'])):
  assert index==case['slot_index'] and case['completed'] and stage['completed'] and not stage['invalid'] and not stage['used_services']
  assert call['entry']=='001d42d0' and call['return_esp_delta']==4 and int(call['returned_eax'],16)==int(case['motion'],16)+0x80
  assert int.from_bytes(bytes.fromhex(case['component_a0_hex'])[4:8],'little')&0x40==0
  before=audit.segments(stage['before']);after=audit.segments(stage['after'])
  for name,(p,raw)in before.items():
   if name not in after or after[name][0]!=p or len(after[name][1])!=len(raw):continue
   assert after[name][1]==raw,(index,name,'source backing must remain unchanged')
   intervals+=1
  out=int(case['output'],16);state=bytearray.fromhex(case['before_output34_hex'])
  stores=r['stores'][case['store_begin']:case['store_end']]
  for w in stores:
   p=int(w['address'],16)
   if not(out<=p and p+w['size']<=out+len(state)):continue
   off=p-out;assert state[off:off+w['size']]==bytes.fromhex(w['before_hex'])
   state[off:off+w['size']]=bytes.fromhex(w['written_hex']);compared_bytes+=w['size']
  assert state==bytes.fromhex(case['after_output34_hex'])
 result=dict(schema=1,auditor_sha256=audit.sha(__file__),report_sha256=audit.sha(a.directory/'report.json'),
  byte_identical_files=len(files),instruction_rows=listed,cases=len(r['cases']),calls=len(r['calls']),
  reads=len(r['reads']),stores=len(r['stores']),unchanged_full_backing_intervals=intervals,
  replayed_output_bytes=compared_bytes,new_supplied_services=0,
  bit6_correction_measured=False,natural_ragdoll_admission_world_solver_or_render_proven=False)
 a.output.parent.mkdir(parents=True,exist_ok=True);raw=audit.encoded(result)
 if a.output.exists():assert a.output.read_bytes()==raw
 else:a.output.write_bytes(raw)
 print(json.dumps(result))
if __name__=='__main__':main()
