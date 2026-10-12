#!/usr/bin/env python3
"""Read-only audit of complete SDK accounting original/native checkpoint."""
import argparse,json,hashlib
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,sha,save,encoded
from oracle_smg_audio_pool_prefix_1_1 import FROZEN
BASE=ROOT/'analysis/multiplayer/integration/sound-capabilities-native'
def canonical(v):
 if isinstance(v,dict):return {k:canonical(x)for k,x in v.items()if k not in('actual_host_address','host_pointer')}
 if isinstance(v,list):return [canonical(x)for x in v]
 return v
def add(pins,name,value):
 h=value if isinstance(value,str)else value['sha256'];assert name not in pins or pins[name]==h,('Conflicting prior pin',name);pins[name]=h
def run(out):
 prior=ROOT/'analysis/multiplayer/integration/fp-sound-submit-composition/final-manifest.json'
 assert sha(prior)=='3101ff47d8e17449acc4e035004220bd6d243bf9925fcd858a5a30e1c403273b'
 d=json.loads(prior.read_text());protected={str(prior.relative_to(ROOT)):sha(prior)}
 for key in('owned_current_files','evidence_files','protected_prior_pins'):
  for name,v in d[key].items():add(protected,name,v)
 for name,h in FROZEN.items():add(protected,name,h)
 for name,h in protected.items():assert sha(ROOT/name)==h,('Protected prior file changed',name)
 report=json.loads((BASE/'measured-v2/report.json').read_text());repeat=json.loads((BASE/'repeat-v2/report.json').read_text())
 assert report['oracle_sha256']==repeat['oracle_sha256']==sha(ROOT/'tools/oracle_sound_capabilities_native.py')
 assert canonical(report)==canonical(repeat),'Original repeat differs beyond actual native mapping addresses'
 equal=[]
 for a,b in(('paired-v2/report.json','paired-repeat/report.json'),('paired-v2/providers.json','paired-repeat/providers.json'),('fixtures-v2/fixtures.tsv','export-repeat/fixtures.tsv')):
  assert(BASE/a).read_bytes()==(BASE/b).read_bytes();equal.append(dict(left=a,right=b,sha256=sha(BASE/a)))
 pair=json.loads((BASE/'paired-v2/report.json').read_text());assert pair['tool_sha256']==sha(ROOT/'tools/research_sound_capabilities_u5.py')and len(pair['owners'])==27
 images=dict(old=Xbe(OLD),u5=Xbe(NEW));rows=0
 for f in sorted((BASE/'paired-v2/instructions').glob('*.asm')):
  tag='u5'if f.stem.endswith('-u5')else'old'
  other=BASE/'paired-repeat/instructions'/f.name;assert f.read_bytes()==other.read_bytes()
  for line in f.read_text().splitlines():
   p=line.split();a=int(p[0],16);raw=bytes.fromhex(p[1]);assert images[tag].read(a,len(raw))==raw,(f,a);rows+=1
 source_stores=platform_stores=bytes_checked=0
 for c in report['cases']:
  memory={int(q['address'],16):bytearray.fromhex(q['raw_hex'])for q in c['initial_regions']}
  for service in c['actual_raw_pool_services']:
   if service['kernel_ordinal']==15:
    n=service['raw_capacity_bytes'];raw=bytearray(n);assert hashlib.sha256(raw).hexdigest()==service['raw_initial_bytes_sha256'];memory[int(service['resulting_guest_alias'],16)]=raw
  def locate(a,n):
   hits=[(lo,q)for lo,q in memory.items()if lo<=a and a+n<=lo+len(q)];assert len(hits)==1,(c['name'],hex(a),n);lo,q=hits[0];return q,a-lo
  for store in c['stores']:
   if int(store['instruction'],16)>=0x400000:platform_stores+=1;continue
   q,off=locate(int(store['address'],16),store['bytes']);assert q[off:off+store['bytes']].hex()==store['before_hex'];q[off:off+store['bytes']]=bytes.fromhex(store['after_hex']);source_stores+=1
  for q in c['output_regions']:
   expected=bytes.fromhex(q['raw_hex']);actual,off=locate(int(q['address'],16),len(expected));assert actual[off:off+len(expected)]==expected,(c['name'],q['address']);bytes_checked+=len(expected)
 assert len(report['cases'])==10 and source_stores==20537 and platform_stores==14 and bytes_checked==450648 and report['source_initialized_78']==0
 for name in('validation-final','validation-final-repeat'):
  native=json.loads((BASE/name/'report.json').read_text());assert native['validator_sha256']==sha(ROOT/'tools/validate_sound_capabilities_native.py')and native['fixture_sha256']==sha(BASE/'fixtures-v2/fixtures.tsv')
  assert all(q['exit_code']==0 for q in json.loads((BASE/name/'commands.json').read_text()))
  for source,h in native['observed_source_snapshots'].items():assert sha(BASE/name/'source'/source)==sha(ROOT/source)==h,source
 for mode in('normal','sanitizer'):
  a=f'validation-final/{mode}-test.stdout';b=f'validation-final-repeat/{mode}-test.stdout';assert(BASE/a).read_bytes()==(BASE/b).read_bytes();equal.append(dict(left=a,right=b,sha256=sha(BASE/a)))
 result=dict(schema='sound-capabilities-accounting-audit-v1',auditor_sha256=sha(__file__),protected_prior_pins=protected,prior_pin_count=len(protected),canonical_original_repeat_equal_except_actual_native_mapping_addresses=True,equal_files=equal,complete_paired_bodies=27,original_instruction_rows_checked=rows,cases=10,ordered_source_stores=source_stores,compatibility_platform_stores_separately_retained=platform_stores,whole_output_bytes=bytes_checked,normal_and_actual_asan_ubsan=True,source_initialized_78=0,normal_startup_saved_setting_or_hardware_completed=False,native_live_audio_connected=False)
 save(out,encoded(result));print(json.dumps(dict(audit_sha256=sha(out),protected=len(protected),instructions=rows,stores=source_stores,bytes=bytes_checked)))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);run(p.parse_args().output)
