#!/usr/bin/env python3
"""Audit repeated original kill-volume admission and complete captured storage."""
import argparse,json,struct
from pathlib import Path
import audit_mp_spartan_death_physical_1_1 as old
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN,CACHE_PIN

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--directory',type=Path,required=True);ap.add_argument('--repeat',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 files=sorted(p.relative_to(a.directory)for p in a.directory.rglob('*')if p.is_file()and p.name not in('raw-host-context.json','runtime-stderr.txt'))
 for p in files:assert(a.directory/p).read_bytes()==(a.repeat/p).read_bytes(),p
 r=json.loads((a.directory/'report.json').read_text());assert old.sha(ROOT/'tools/oracle_mp_spartan_kill_trigger_1_1.py')==r['oracle_sha256']
 for p,h in r['prior_pins'].items():assert old.sha(ROOT/p)==h,p
 assert r['source_sha256']==PIN and r['cache_sha256']==CACHE_PIN and r['prior_movement_byte_identical'] and r['prior_runtime_correlation']['byte_identical']
 assert not r['instruction_patches']and not r['new_supplied_guest_owning_fields_or_success']
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');listed=0;executed=set()
 for q in r['instructions']:
  p=a.directory/q['file'];assert old.sha(p)==q['sha256']
  for line in p.read_text().splitlines():
   at,raw,*_=line.split(' ',2);at=int(at,16);raw=bytes.fromhex(raw);assert image.read(at,len(raw))==raw;listed+=1
   if q.get('kind')=='all_new_execution':executed.add(at)
 for e in r['reads']+r['stores']:assert int(e['instruction'],16)in executed
 assert len(r['stages'])==7 and all(q['completed']and not q['invalid']for q in r['stages'][:-1])
 stop=r['stages'][-1];assert not stop['completed']and stop['invalid']['instruction']=='0020efc2'and stop['invalid']['address']=='00000024'
 checks=[q for q in r['calls']if q['entry']=='001efef0'];assert [int(q['returned_eax'],16)&255 for q in checks]==[0,1,1]
 points=[q for q in r['calls']if q['entry']=='0011da40'];assert [int(q['returned_eax'],16)&255 for q in points]==[0,1,1,1]
 request=next(q for q in r['calls']if q['entry']=='000d9180');packet=bytes.fromhex(request['packet']['entry_hex'])
 assert int.from_bytes(packet[0:4],'little')==0xeae90964 and packet[4:8]==bytes.fromhex('04000000')and packet[0x84]==1
 assert request['stack16'][1:6]==['e26f0000','ffffffff','ffffffff','ffffffff','00000000']
 death=next(q for q in r['calls']if q['entry']=='000d7e00')
 assert death['stack16'][:4]==['e26f0000',f'{int(request["packet"]["pointer"],16)+8:08x}','00000001','00000000']
 before=bytes.fromhex(death['owner_entry']['unit_literal']['raw800_hex']);after=bytes.fromhex(death['owner_return']['unit_literal']['raw800_hex'])
 assert before[0x88:0x94]==after[0x88:0x94]==bytes.fromhex('d9ef333c55bea33eb29d6fbe')
 assert not(before[0x10a]&4)and after[0x10a]&4
 assert before[0x13c:0x144]==after[0x13c:0x144]==bytes.fromhex('000070ecffffffff')
 assert not any(q['entry']in('000c5190','000eff50','000cbdc0','001d9330')for q in r['calls'])
 assert any(q['entry']=='0020ef90'and q['registers']['eax']=='e96c07e7'for q in r['calls'])
 call_phase={q['id']:q['phase']for q in r['calls']};intervals=reads=stores=changed=0
 for stage in r['stages']:
  name=stage['name'];b=old.segments(stage['before']);end=old.segments(stage['after'])
  events=[(w['order'],'store',w)for w in r['stores']if w['phase']==name]+[(w['order'],'read',w)for w in r['reads']if call_phase[w['call']]==name];events.sort()
  for key,(p,raw)in b.items():
   if key not in end or end[key][0]!=p or len(end[key][1])!=len(raw):continue
   state=bytearray(raw)
   for _,kind,e in events:
    at=int(e['address'],16);n=e['size']
    if not(p<=at and at+n<=p+len(raw)):continue
    offset=at-p;assert state[offset:offset+n]==bytes.fromhex(e['before_hex']if kind=='store'else e['raw_hex']),(name,key,e['order'])
    if kind=='store':state[offset:offset+n]=bytes.fromhex(e['written_hex']);stores+=1
    else:reads+=1
   assert state==end[key][1],(name,key,'whole captured after buffer');intervals+=1;changed+=sum(x!=y for x,y in zip(raw,state))
 result=dict(schema=1,auditor_sha256=old.sha(__file__),report_sha256=old.sha(a.directory/'report.json'),byte_identical_files=len(files),instruction_rows=listed,
  executed_addresses=len(executed),calls=len(r['calls']),reads=len(r['reads']),stores=len(r['stores']),whole_intervals_replayed=intervals,
  overlapping_reads=reads,overlapping_stores=stores,changed_bytes=changed,kill_check_AL=[0,1,1],point_check_AL=[0,1,1,1],
  actual_current_bounds_center_used=True,dead_bit_set_velocity_retained_before_sound_stop=True,
  unavailable_sound_global='526464',stop='20EFC2',complete_fatal_cleanup_or_landing_dead_or_natural_fall_proven=False)
 a.output.parent.mkdir(parents=True,exist_ok=True);raw=old.encoded(result)
 if a.output.exists():assert a.output.read_bytes()==raw
 else:a.output.write_bytes(raw)
 print(json.dumps(result))
if __name__=='__main__':main()
