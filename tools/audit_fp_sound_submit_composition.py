#!/usr/bin/env python3
"""Read-only audit of original/native full FP raw-submit composition evidence."""
import argparse,json,hashlib
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb')as f:
  for b in iter(lambda:f.read(8<<20),b''):h.update(b)
 return h.hexdigest()
def encoded(x):return(json.dumps(x,sort_keys=True,indent=2)+'\n').encode()
def save(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,('Existing audit changed',str(p))
 else:p.write_bytes(b)
def value(x):return x if isinstance(x,str)else x['sha256']
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();base=ROOT/'analysis/multiplayer/integration/fp-sound-submit-composition';protected={}
 prior=['analysis/multiplayer/integration/fp-sound-request-native/final-manifest.json','analysis/multiplayer/integration/sound-object-request-native/final-manifest.json','analysis/multiplayer/integration/sound-submit-native/final-manifest.json']
 for name in prior:
  path=ROOT/name;d=json.loads(path.read_text());protected[name]=sha(path)
  for key in('files','evidence_files','original_inputs','owned_current_files','prior_pins'):
   for name,v in d.get(key,{}).items():
    h=value(v);assert name not in protected or protected[name]==h,('Conflicting prior pin',name);protected[name]=h
 for name,h in protected.items():assert sha(ROOT/name)==h,('Protected prior file changed',name)
 equal=[]
 for left,right in(('measured-final/report.json','repeat-final/report.json'),('fixtures-final/fixtures.tsv','export-repeat-final/fixtures.tsv'),('fixtures-final/fixtures.json','export-repeat-final/fixtures.json'),('fixtures/fixtures.tsv','fixtures-final/fixtures.tsv')):
  assert(base/left).read_bytes()==(base/right).read_bytes();equal.append(dict(left=left,right=right,sha256=sha(base/left)))
 report=json.loads((base/'measured-final/report.json').read_text());assert report['tool_sha256']==sha(ROOT/'tools/oracle_fp_sound_submit_composition.py')
 for c in report['cases']:
  raw=bytearray.fromhex(c['raw_before_hex']);assert c['submit_return']=='ffffffff'and not c['submit_stores']and c['object_request']['result_al']==1
  for s in c['raw_stores']:
   p,n=s['offset'],s['bytes'];assert int.from_bytes(raw[p:p+n],'little')==int(s['before'],16);raw[p:p+n]=int(s['after'],16).to_bytes(n,'little')
  assert raw.hex()==c['request_at_submit_hex']==c['request_after_hex'];assert c['source88_hex'][96:136]==raw[60:80].hex()
 for mode in('normal','sanitizer'):
  left=base/'validation-final'/mode/'test.stdout';right=base/'validation-repeat-final'/mode/'test.stdout';assert left.read_bytes()==right.read_bytes();equal.append(dict(left=str(left.relative_to(base)),right=str(right.relative_to(base)),sha256=sha(left)))
 for folder in('validation-final','validation-repeat-final'):
  d=json.loads((base/folder/'report.json').read_text());assert d['fixture_sha256']==sha(base/'fixtures-final/fixtures.tsv')
  for name,h in d['source_pins'].items():assert sha(base/folder/'source'/name)==sha(ROOT/name)==h,name
  assert all(q['exit_code']==0 for q in json.loads((base/folder/'commands.json').read_text()))
 images=dict(old=Xbe(OLD),u5=Xbe(NEW));rows=0
 fp=ROOT/'analysis/multiplayer/integration/fp-sound-request-native/u5-final/instructions'
 submit=ROOT/'analysis/multiplayer/integration/sound-submit-native/paired-owners/instructions'
 files=list(fp.glob('*.asm'))+[submit/'0021f220-old.asm',submit/'0021f220-u5.asm']
 for f in files:
  label='u5'if f.stem.endswith('-u5')else'old';image=images[label]
  for line in f.read_text().splitlines():
   parts=line.split();address=int(parts[0],16);raw=bytes.fromhex(parts[1]);assert image.read(address,len(raw))==raw,(f,address);rows+=1
 result=dict(schema='fp-world-raw-submit-composition-audit-v1',auditor_sha256=sha(__file__),protected_prior_pins=protected,prior_pin_count=len(protected),equal_files=equal,original_instruction_rows_checked=rows,cases=3,world_stores=30,raw_stores=99,source_owner_or_submit_replies_replayed=0,new_availability_or_admission_values=0,normal_and_actual_asan_ubsan=True,scope='The genuine source cold return and previous fixture world/local/clock controls remain explicit; no live reload/device/startup completion claim.')
 save(a.output,encoded(result));print(json.dumps(dict(audit_sha256=sha(a.output),protected=len(protected),rows=rows,equal_files=len(equal))))
if __name__=='__main__':main()
