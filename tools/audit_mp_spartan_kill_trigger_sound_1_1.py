#!/usr/bin/env python3
"""Independent selected kill-fatal sound continuation repeat/storage audit."""
import argparse,json,hashlib
from pathlib import Path
import audit_mp_spartan_death_physical_1_1 as old
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN,CACHE_PIN

def segments(s):
 r=old.segments(s)
 a=s.get('selected_sound_storage')
 if a:
  for p,word in a['globals'].items():r['sound-global-'+p]=(int(p,16),int(word,16).to_bytes(4,'little'))
  for k,pk in(('bit_bank1880_hex','0052627c'),('arena_slice1880_hex','00526280')):
   if a[k]is not None:r[k]=(int(a['globals'][pk],16),bytes.fromhex(a[k]))
 return r

def same_prefix(old,new):
 if isinstance(old,dict):return all(k in new and same_prefix(v,new[k])for k,v in old.items())
 if isinstance(old,list):return len(old)==len(new)and all(same_prefix(a,b)for a,b in zip(old,new))
 return old==new

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--directory',type=Path,required=True);ap.add_argument('--repeat',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 files=sorted(p.relative_to(a.directory)for p in a.directory.rglob('*')if p.is_file()and p.name not in('raw-host-context.json','runtime-stderr.txt','raw-host-allocations.json'))
 for p in files:assert(a.directory/p).read_bytes()==(a.repeat/p).read_bytes(),p
 r=json.loads((a.directory/'report.json').read_text());assert old.sha(ROOT/'tools/oracle_mp_spartan_kill_trigger_sound_1_1.py')==r['oracle_sha256']
 for p,h in r['prior_pins'].items():assert old.sha(ROOT/p)==h,p
 assert r['source_sha256']==PIN and r['cache_sha256']==CACHE_PIN and r['entire_frozen_prefix_byte_identical'] and r['prior_runtime_correlation']['byte_identical']
 base=ROOT/'analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe/report.json';assert old.sha(base)==r['frozen_prefix_sha256']
 assert not r['instruction_patches']and not r['new_supplied_gameplay_or_audio_success']and r['original_stack_unchanged_during_selected_roots']
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');listed=0;executed=set()
 for q in r['instructions']:
  p=a.directory/q['file'];assert old.sha(p)==q['sha256']
  for line in p.read_text().splitlines():
   at,raw,*_=line.split(' ',2);at=int(at,16);raw=bytes.fromhex(raw);assert image.read(at,len(raw))==raw;listed+=1
   if q.get('kind')=='all_execution':executed.add(at)
 for e in r['reads']+r['stores']:assert int(e['instruction'],16)in executed
 stages=r['stages'];assert len(stages)==12
 assert stages[7]['completed']and stages[8]['completed']
 assert stages[9]['invalid']['instruction']=='00126e22'and stages[9]['invalid']['address']=='000001e4'
 assert stages[10]['invalid']['instruction']=='0020efc2'and stages[10]['invalid']['address']=='00000024'
 assert stages[11]['entry']=='0020efbc'
 original=json.loads(base.read_text());prefix=r['prefix_counts']
 assert stages[:prefix['stages']]==original['stages']
 for key in('reads','stores','events'):assert r[key][:prefix[key]]==original[key],key
 for a0,b0 in zip(original['calls'],r['calls'][:prefix['calls']]):assert same_prefix(a0,b0),a0['id']
 assert stages[-1]['completed']and stages[-1]['invalid']is None
 initial=old.fields(r['initial']);final=old.fields(r['final'])
 assert initial['velocity_hex']==final['velocity_hex']=='d9ef333c55bea33eb29d6fbe'
 assert initial['physical_state']==1 and final['physical_state']==3 and final['mode34b']==0 and final['body_count']==1
 before=bytes.fromhex(r['initial']['unit_literal']['raw800_hex']);after=bytes.fromhex(r['final']['unit_literal']['raw800_hex'])
 assert before[0x13c:0x144]==bytes.fromhex('000070ecffffffff')and after[0x13c:0x144]==bytes.fromhex('ffffffff000070ec')
 pb=bytes.fromhex(r['initial']['player']['records'][0]['raw_hex']);pa=bytes.fromhex(r['final']['player']['records'][0]['raw_hex'])
 assert pb[0x2c:0x34]==bytes.fromhex('00006fe2ffffffff')and pa[0x2c:0x34]==bytes.fromhex('ffffffff00006fe2')
 assert not any(c['entry']in('000c5190','000eff50','001d9330')for c in r['calls'])
 for e in('000defc0','000d9180','000dac40','000cb410','000cbdc0','000e6050','000e6dc0','001c70d0'):
  assert any(c['entry']==e and'returned_eax'in c for c in r['calls']),e
 unitp=int(r['final']['unit_literal']['pointer'],16)
 assert not any(int(w['address'],16)<unitp+0x94 and int(w['address'],16)+w['size']>unitp+0x88 for w in r['stores']if w['phase']==stages[-1]['name'])
 assert [(q['service'],q['return_value'])for q in stages[-1]['used_services']if q.get('supplied')]==[('00170060','00000000')]*2
 assert r['raw_os_providers']==[dict(phase=stages[7]['name'],entry='0033785c',return_site='0020e8fe',arguments=['00000000','00001880','00101000','00000004'],returned_alias='0c100000',requested_bytes=0x1880,rounded_bytes=0x2000,
  actual_provider='libc mmap anonymous private PROT_READ|PROT_WRITE',actual_errno=0,original_VirtualAlloc_implementation_executed=False,scope='Actual raw OS memory only; source constructor owns every sound field. No retail Xbox address/status claim.')]
 assert stages[8]['after']['selected_sound_storage']['bit_bank1880_hex']=='00'*0x1880
 assert stages[9]['after']['selected_sound_storage']['globals']['004ed950']=='00000000'
 assert stages[9]['after']['selected_sound_storage']['globals']['00526464']=='804eee90'
 call_phase={q['id']:q['phase']for q in r['calls']};intervals=reads=stores=changed=0
 for stage in stages:
  name=stage['name'];b=segments(stage['before']);end=segments(stage['after'])
  events=[(w['order'],'store',w)for w in r['stores']if w['phase']==name]+[(w['order'],'read',w)for w in r['reads']if w.get('phase',call_phase[w['call']])==name];events.sort()
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
 result=dict(schema=1,auditor_sha256=old.sha(__file__),report_sha256=old.sha(a.directory/'report.json'),byte_identical_files=len(files),instruction_rows=listed,executed_addresses=len(executed),
  calls=len(r['calls']),reads=len(r['reads']),stores=len(r['stores']),whole_intervals_replayed=intervals,overlapping_reads=reads,overlapping_stores=stores,changed_bytes=changed,
  original_storage_constructor_and_reset_complete=True,cold_map_prefix_still_unavailable=True,selected_composition_not_natural_map_startup=True,
  held_original_stack_preserved=True,final_completed=stages[-1]['completed'],final_stop=stages[-1]['invalid'],final_unit_fields=old.fields(r['final']))
 a.output.parent.mkdir(parents=True,exist_ok=True);raw=old.encoded(result)
 if a.output.exists():assert a.output.read_bytes()==raw
 else:a.output.write_bytes(raw)
 print(json.dumps(result))
if __name__=='__main__':main()
