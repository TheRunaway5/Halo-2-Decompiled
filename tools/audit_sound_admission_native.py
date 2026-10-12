#!/usr/bin/env python3
"""Audit original admission controls, independent repeats and prior immutable pins."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,sha,save,encoded
PRIOR=ROOT/'analysis/multiplayer/integration/sound-driver-update-native/final-manifest.json'
PRIOR_PIN='04b6dbc10522848f8511c3adf4e2f001b3c4970c771c1c530ec14a7dbb0a750d'
def prior_pins():
 assert sha(PRIOR)==PRIOR_PIN;r=json.loads(PRIOR.read_text());pins=dict(r['prior_pins']);pins[str(PRIOR.relative_to(ROOT))]=PRIOR_PIN
 for field in('owned_current_files','evidence_files','original_inputs'):
  for rel,q in r[field].items():
   assert rel not in pins or pins[rel]==q['sha256'];pins[rel]=q['sha256']
 return pins

def audit(d):
 pins=prior_pins()
 for rel,pin in pins.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==pin,rel
 pairs=[]
 for a,b in(('paired-v2','paired-repeat'),('measured-v5','measured-repeat')):
  for p in(d/a).rglob('*'):
   if not p.is_file():continue
   q=d/b/p.relative_to(d/a);assert q.is_file()and p.read_bytes()==q.read_bytes(),(p,q);pairs.append([str(p.relative_to(ROOT)),str(q.relative_to(ROOT)),sha(p)])
 # The initial successful measurement reused its actual14CC00 preparation;
 # the independent repeat reexecutes that constructor. Preserve both listings.
 p=d/'measured/instructions/actual14cc00-preparation.asm';q=d/'measured-repeat/instructions/actual14cc00-preparation.asm';assert p.read_bytes()==q.read_bytes();pairs.append([str(p.relative_to(ROOT)),str(q.relative_to(ROOT)),sha(p)])
 for p in(d/'export-repeat').iterdir():q=d/'fixtures'/p.name;assert p.read_bytes()==q.read_bytes();pairs.append([str(p.relative_to(ROOT)),str(q.relative_to(ROOT)),sha(p)])
 for name in('normal.stdout','normal.stderr','sanitizer.stdout','sanitizer.stderr'):
  p=d/'validation-v2/native'/name;q=d/'validation-v2-repeat/native'/name;assert p.read_bytes()==q.read_bytes();pairs.append([str(p.relative_to(ROOT)),str(q.relative_to(ROOT)),sha(p)])
 for rel in('include/halo2/sound_admission.hpp','src/sound_admission.cpp','tests/sound_admission_tests.cpp'):
  for group in('validation-v2/native/source','validation-v2-repeat/native/source','production/source'):assert(ROOT/rel).read_bytes()==(d/group/rel).read_bytes(),rel
 images={'old':Xbe(OLD),'u5':Xbe(NEW)};rows=0
 for p in(d/'paired-v2/instructions').glob('*.asm'):
  image=images[p.stem.rsplit('-',1)[1]]
  for line in p.read_text().splitlines():a,b,*_=line.split();raw=bytes.fromhex(b);assert image.read(int(a,16),len(raw))==raw;rows+=1
 for p in(d/'measured-repeat/instructions').glob('*.asm'):
  image=images['old'if p.stem.startswith('actual14cc00')else p.stem.split('-')[0]]
  for line in p.read_text().splitlines():a,b,*_=line.split();raw=bytes.fromhex(b);assert image.read(int(a,16),len(raw))==raw;rows+=1
 report=json.loads((d/'measured-v5/report.json').read_text());meta=json.loads((d/'fixtures/fixtures.json').read_text());assert meta['calls']==3864 and meta['stores']==552
 assert report['supplied_availability_listener_seed_voice_or_platform_results']==[] and not report['instruction_patches']
 assert all(b['availability_hex']=='000100'and b['local_slots']==['ffffffff']*4 for b in report['banks'])
 full=[c for b in report['banks']for c in b['cases']if c['entry']=='00128380'];assert len(full)==276 and all(c['result']=='00000000'for c in full)
 slices=[c for b in report['banks']for c in b['cases']if c['entry']=='001283f4'];assert len(slices)==138 and all(len(c['stores'])==1 and c['stores'][0]['instruction']=='00128408'for c in slices)
 prep=report['preparation'];assert prep['constructor_returned']and not prep['reset14CCD0_executed']and not prep['local_binding_supplied'];assert len(prep['stores'])==57
 prod=json.loads((d/'production/commands.json').read_text());assert all(c['exit_code']==0 for c in prod['commands'])
 assert '-fsanitize=address,undefined'in(d/'production/sanitizer/sound_admission_tests-flags.make').read_text()
 for tag in('normal','sanitizer'):
  text=(d/'production'/(tag+'-ctest.stdout')).read_text();assert '100% tests passed'in text and 'PASS 3864 original controls'in text and 'PASS 138 original creation prefixes'in text
 registration=(d/'production/source/CMakeLists.txt').read_text();assert 'src/sound_admission.cpp'in registration and 'NAME original_sound_admission'in registration
 result=dict(schema='sound-admission-audit-v1',auditor_sha256=sha(__file__),prior_pins_verified=pins,exact_repeat_pairs=pairs,original_instruction_rows_checked=rows,cases=3864,ordered_stores=552,full_fixture_bytes=1881423264,full_cold_owner_calls=276,explicit_skip_slices=138,direct_queries=3450,actual_local_table_constructor_stores=57,successful_full_admission=False,production_source_drift=prod['source_drift'],mismatches=[])
 save(d/'audit.json',encoded(result));print(json.dumps(dict(audit_sha256=sha(d/'audit.json'),prior_pins=len(pins),equal_files=len(pairs),instruction_rows=rows,mismatches=0)))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();audit(a.directory.resolve())
