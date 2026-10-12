#!/usr/bin/env python3
"""Audit new cache owner against original bytes/repeats and prior freezes."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,sha,save,encoded
BASE=ROOT/'analysis/multiplayer/integration/sound-cache-request-native'
PRIOR=((ROOT/'analysis/multiplayer/integration/sound-create-native/final-manifest.json','6483852f56ef577383d3fd80edb7edec16a942f0067d654742e9370c95026318'),(ROOT/'analysis/multiplayer/integration/spawn-source/animation-async-worker/manifest.json','c08523918642751fc570cc7b3949327ae2ee19443c2f1d04a9e88bf871d18475'))
def prior_pins():
 pins={}
 def put(rel,pin):
  if rel in pins:assert pins[rel]==pin,rel
  pins[rel]=pin
 for path,pin in PRIOR:
  assert sha(path)==pin;put(str(path.relative_to(ROOT)),pin);r=json.loads(path.read_text())
  for field in ('prior_pins','protected_prior_pins','artifact_pins','dependency_pins','new_source_pins'):
   for rel,value in r.get(field,{}).items():put(rel,value)
  for field in('owned_current_files','evidence_files','original_inputs'):
   for rel,value in r.get(field,{}).items():put(rel,value['sha256'])
 return pins

def audit(d):
 pins=prior_pins();mismatches=[]
 for rel,pin in pins.items():
  path=Path(rel);path=path if path.is_absolute()else ROOT/path
  if sha(path)!=pin:mismatches.append(dict(path=rel,expected=pin,actual=sha(path)))
 assert not mismatches,mismatches
 equal=[]
 for old,new in(('paired','paired-repeat'),('measured','repeated')):
  for p in(d/old).rglob('*'):
   if p.is_file():q=d/new/p.relative_to(d/old);assert q.is_file()and p.read_bytes()==q.read_bytes(),(p,q);equal.append([str(p.relative_to(ROOT)),str(q.relative_to(ROOT)),sha(p)])
 images={'old':Xbe(OLD),'u5':Xbe(NEW)};rows=0
 for p in(d/'paired/instructions').glob('*.asm'):
  image=images[p.stem.rsplit('-',1)[1]]
  for line in p.read_text().splitlines():a,b,*_=line.split();raw=bytes.fromhex(b);assert image.read(int(a,16),len(raw))==raw;rows+=1
 for line in(d/'measured/instructions/executed-old.asm').read_text().splitlines():
  a,b,*_=line.split();raw=bytes.fromhex(b);assert images['old'].read(int(a,16),len(raw))==raw;rows+=1
 for rel in('include/halo2/sound_cache_request.hpp','src/sound_cache_request.cpp','tests/sound_cache_request_tests.cpp'):
  assert(ROOT/rel).read_bytes()==(d/'validation-v2/native/source'/rel).read_bytes(),rel
  assert(ROOT/rel).read_bytes()==(d/'validation-v2-repeat/native/source'/rel).read_bytes(),rel
 for name in('normal.stdout','sanitizer.stdout','normal.stderr','sanitizer.stderr'):
  a=d/'validation-v2/native'/name;b=d/'validation-v2-repeat/native'/name;assert a.read_bytes()==b.read_bytes();equal.append([str(a.relative_to(ROOT)),str(b.relative_to(ROOT)),sha(a)])
 for p in(d/'export-repeat').iterdir():q=d/p.name;assert p.read_bytes()==q.read_bytes();equal.append([str(p.relative_to(ROOT)),str(q.relative_to(ROOT)),sha(p)])
 report=json.loads((d/'measured/report.json').read_text());meta=json.loads((d/'fixtures.json').read_text())
 assert len(report['cases'])==3 and meta['stages']==18 and meta['stores']==8346
 for c in report['cases']:
  assert [s['eax']for s in c['stages'][3:]]==['00000002','00000006','00000006']
  for io in c['io']:
   with Path(report['shared_file']['path']).open('rb')as f:f.seek(io['offset']);import hashlib;assert hashlib.sha256(f.read(io['size'])).hexdigest()==io['sha256']
 result=dict(schema='sound-cache-request-native-audit-v1',auditor_sha256=sha(__file__),prior_pins_verified=pins,exact_repeat_pairs=equal,original_instruction_rows_checked=rows,source_requests=3,phases=18,ordered_stores=8346,actual_owned_file_reads=3,full_fixture_region_bytes=meta['full_region_bytes'],mismatches=mismatches,native_platform_adaptation=True,source_admission_voice_or_thread=False)
 save(d/'audit.json',encoded(result));print(json.dumps(dict(audit_sha256=sha(d/'audit.json'),prior_pins=len(pins),independent_equal_files=len(equal),instruction_rows=rows,mismatches=0)))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();audit(a.directory.resolve())
