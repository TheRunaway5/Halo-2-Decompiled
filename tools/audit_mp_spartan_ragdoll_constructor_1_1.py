#!/usr/bin/env python3
"""Verify original positive Spartan construction, complete buffers and repeat."""
import argparse
import json
import struct
from pathlib import Path
import audit_mp_spartan_death_physical_1_1 as prior
from oracle_smg_firing_1_1 import ROOT, Xbe


def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--directory',type=Path,required=True)
 ap.add_argument('--repeat',type=Path,required=True)
 ap.add_argument('--output',type=Path,required=True)
 a=ap.parse_args();r=json.loads((a.directory/'report.json').read_text())
 files=sorted(p.relative_to(a.directory)for p in a.directory.rglob('*')if p.is_file()and p.name not in ('raw-host-context.json','runtime-stderr.txt'))
 for p in files:assert (a.directory/p).read_bytes()==(a.repeat/p).read_bytes(),p
 assert prior.sha(ROOT/'tools/oracle_mp_spartan_ragdoll_constructor_1_1.py')==r['oracle_sha256']
 for p,h in r['prior_pins'].items():assert prior.sha(ROOT/p)==h,p
 assert r['entire_frozen_moving_death_byte_identical']
 assert not r['instruction_patches'] and not r['new_supplied_guest_fields_or_results']
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 listed=0;executed=set()
 for listing in r['instructions']:
  p=a.directory/listing['file'];assert prior.sha(p)==listing['sha256']
  for line in p.read_text().splitlines():
   at,raw,*_=line.split(' ',2);at=int(at,16);raw=bytes.fromhex(raw)
   assert image.read(at,len(raw))==raw,(p,at);listed+=1
   if listing.get('kind')=='complete_new_execution':executed.add(at)
 for row in r['stores']+r['reads']:assert int(row['instruction'],16)in executed
 assert all(s['completed'] and s['invalid'] is None and not s['used_services'] for s in r['stages'])
 assert not r['events']
 group=r['stages'][-1];o=prior.unit(group['after']);component=bytes.fromhex(o['component_hex'])
 assert len(o['bodies'])==10
 assert struct.unpack_from('<I',component,0x74)[0]==10
 assert struct.unpack_from('<I',component,0x80)[0]==9
 assert prior.fields(group['before'])['body_count']==0
 assert prior.fields(group['after'])['mode34b']==0
 assert prior.fields(group['after'])['physical_state']==3
 assert all(body['world']=='00000000'for body in o['bodies'])
 c=next(c for c in r['calls']if c['entry']=='001d9330')
 assert c['returned_eax']=='00000001' and c['return_esp_delta']==20
 assert sum(c['entry']=='001d3a70'for c in r['calls'])==10
 assert sum(c['entry']=='001db1c0'for c in r['calls'])==1
 callphase={c['id']:c['phase']for c in r['calls']}
 intervals=reads=stores=changed=0;summaries=[]
 for stage in r['stages']:
  events=[(s['order'],'store',s)for s in r['stores']if s['phase']==stage['name']]
  events +=[(q['order'],'read',q)for q in r['reads']if callphase[q['call']]==stage['name']]
  events.sort();before=prior.segments(stage['before']);after=prior.segments(stage['after'])
  for key,(base,raw)in before.items():
   if key not in after or after[key][0]!=base or len(after[key][1])!=len(raw):continue
   state=bytearray(raw)
   for order,kind,e in events:
    at=int(e['address'],16);size=e['size']
    if not(base<=at and at+size<=base+len(raw)):continue
    off=at-base;expected=bytes.fromhex(e['before_hex']if kind=='store'else e['raw_hex'])
    assert state[off:off+size]==expected,(stage['name'],key,order,e['instruction'])
    if kind=='store':state[off:off+size]=bytes.fromhex(e['written_hex']);stores+=1
    else:reads+=1
   assert state==after[key][1],(stage['name'],key,'complete output')
   intervals+=1;changed+=sum(a!=b for a,b in zip(raw,state))
  summaries.append(dict(name=stage['name'],before=prior.fields(stage['before']),after=prior.fields(stage['after'])))
 frame=c['owning_frame'];base=int(frame['pointer'],16);state=bytearray.fromhex(frame['before_hex'])
 first=next(s['order']for s in r['stores']if s['instruction']=='001d9363')
 frame_stores=frame_reads=0
 events=[(s['order'],'store',s)for s in r['stores']if first<=s['order']<c['return_order']]
 events +=[(q['order'],'read',q)for q in r['reads']if first<=q['order']<c['return_order']]
 for order,kind,e in sorted(events):
  at=int(e['address'],16);size=e['size']
  if not(base<=at and at+size<=base+len(state)):continue
  off=at-base;expected=bytes.fromhex(e['before_hex']if kind=='store'else e['raw_hex'])
  assert state[off:off+size]==expected,('owning_frame',order,e['instruction'])
  if kind=='store':state[off:off+size]=bytes.fromhex(e['written_hex']);frame_stores+=1
  else:frame_reads+=1
 assert state==bytes.fromhex(frame['after_hex'])
 result=dict(schema=1,tool_sha256=prior.sha(__file__),report_sha256=prior.sha(a.directory/'report.json'),
  byte_identical_files=len(files),instruction_rows=listed,executed_addresses=len(executed),
  calls=len(r['calls']),reads=len(r['reads']),stores=len(r['stores']),
  replayed_intervals=intervals,overlapping_reads=reads,overlapping_stores=stores,changed_bytes=changed,
  owning_frame_reads=frame_reads,owning_frame_stores=frame_stores,
  body_count=10,constraint_count=9,source_group_return_eax='00000001',source_mode34b=0,
  world_inserted=False,natural_ragdoll_admission_proven=False,new_supplied_services=0,phases=summaries)
 a.output.parent.mkdir(parents=True,exist_ok=True);raw=prior.encoded(result)
 if a.output.exists():assert a.output.read_bytes()==raw
 else:a.output.write_bytes(raw)
 print(json.dumps({k:v for k,v in result.items()if k!='phases'}))

if __name__=='__main__':main()
