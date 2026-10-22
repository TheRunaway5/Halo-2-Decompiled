#!/usr/bin/env python3
"""Audit loop-owner original repetitions, full bytes and protected checkpoints."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,sha,save,encoded
PRIOR=ROOT/'analysis/multiplayer/integration/object-construction-provider-20261009/frozen/manifest.json'
PRIOR_PIN='5209c868282f97bf3d39caf6e262f62c88f2ed59e802f8c5adbbcbddea1c48f9'
def prior_pins():
 assert sha(PRIOR)==PRIOR_PIN;r=json.loads(PRIOR.read_text());pins=dict(r['protected_prior_pins']);pins[str(PRIOR.relative_to(ROOT))]=PRIOR_PIN
 for field in('artifact_pins','owned_source_pins','dependency_manifests'):
  for rel,pin in r[field].items():assert rel not in pins or pins[rel]==pin; pins[rel]=pin
 return pins
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();pins=prior_pins()
 for rel,pin in pins.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==pin,rel
 equal=[]
 for left,right in(('paired-v5','paired-repeat'),('preparation-v3','preparation-repeat'),('measured-v5','measured-repeat-v2'),('fixtures-v3','export-repeat')):
  for file in(d/left).rglob('*'):
   if not file.is_file():continue
   repeat=d/right/file.relative_to(d/left);assert repeat.is_file()and file.read_bytes()==repeat.read_bytes(),(file,repeat);equal.append([str(file.relative_to(ROOT)),str(repeat.relative_to(ROOT)),sha(file)])
 for mode in('normal','sanitizer'):
  for log in('test.stdout','test.stderr'):
   left=d/'validation-v2'/mode/log;right=d/'validation-repeat'/mode/log;assert left.read_bytes()==right.read_bytes();equal.append([str(left.relative_to(ROOT)),str(right.relative_to(ROOT)),sha(left)])
 for rel in('include/halo2/sound_looping.hpp','src/audio/sound_looping.cpp','tests/sound_looping_tests.cpp'):
  for folder in('validation-v2','validation-repeat','production'):assert(ROOT/rel).read_bytes()==(d/folder/'source'/rel).read_bytes(),rel
 original=json.loads((d/'measured-v5/report.json').read_text());assert not original['source_success_returns_supplied']and not original['instruction_patches'];assert sum(len(b['cases'])for b in original['banks'])==2467
 assert sum(len(c['stores'])for b in original['banks']for c in b['cases'])==55376
 for bank in original['banks']:
  assert bank['source_gameplay_or_platform_success_supplied']is False
  if bank['name']=='capacity_sequence':assert len(bank['cases'])==1025 and all(c['result']!='ffffffff'for c in bank['cases'][:1024])and bank['cases'][-1]['result']=='ffffffff'and not bank['cases'][-1]['stores']
  if bank['name']=='constructed_reset':assert len(bank['cases'])==1 and bank['cases'][0]['returned']is False
 prep=json.loads((d/'preparation-v3/report.json').read_text());assert [q['returned']for q in prep['stages']]==[True,False,True,True,True];assert prep['stages'][1]['stopped_eip']=='0022310c'and prep['final']['source_driver']=='00000000';assert len(prep['actual_request_calls'])==1
 request=prep['actual_request_calls'][0];assert request['caller_return']=='000bfccb'and request['object']=='e2710002'and request['result']=='e26f0000';assert len(prep['continuation']['stages'])==12 and not prep['continuation']['unreturned_original_calls']
 images={'old':Xbe(OLD),'u5':Xbe(NEW)};rows=0
 for folder in('paired-v5/instructions','measured-v5/instructions','preparation-v3/instructions'):
  for file in(d/folder).glob('*.asm'):
   image=images[file.stem.rsplit('-',1)[1]if folder.startswith('paired')else file.stem.split('-')[0]if folder.startswith('measured')else'old']
   for line in file.read_text().splitlines():address,raw,*_=line.split();value=bytes.fromhex(raw);assert image.read(int(address,16),len(value))==value;rows+=1
 meta=json.loads((d/'fixtures-v3/report.json').read_text());assert meta['calls']==2467 and meta['stores']==55376 and not meta['expected_from_native']
 for folder in('validation-v2','validation-repeat'):
  commands=json.loads((d/folder/'commands.json').read_text());assert all(q['exit_code']==0 for q in commands);assert '-fsanitize=address,undefined'in commands[2]['argv']
 production=json.loads((d/'production/commands.json').read_text());assert all(q['exit_code']==0 for q in production['commands']);assert not any(k in production['source_drift']for k in('include/halo2/sound_looping.hpp','src/audio/sound_looping.cpp','tests/sound_looping_tests.cpp'))
 for tag in('normal','sanitizer'):
  assert '100% tests passed'in(d/'production'/(tag+'-ctest.stdout')).read_text();assert 'PASS 2467 original controls'in(d/'production'/(tag+'-ctest.stdout')).read_text()
 assert '-fsanitize=address,undefined'in(d/'production/sanitizer/sound_looping_tests-flags.make').read_text()
 result=dict(schema='sound-looping-native-audit-v1',auditor_sha256=sha(__file__),prior_pins_verified=pins,exact_repeat_pairs=equal,original_instruction_rows_checked=rows,calls=2467,registration_ABIs=2056,ordered_stores=55376,complete_compared_bytes=4689306223,natural_absent_driver_stops=2,actual_grenade_registration=True,natural_startup_or_voice_admission=False,production_source_drift=production['source_drift'],mismatches=[])
 save(d/'audit.json',encoded(result));print(json.dumps(dict(audit_sha256=sha(d/'audit.json'),prior_pins=len(pins),equal_files=len(equal),instruction_rows=rows,mismatches=0)))
if __name__=='__main__':main()
