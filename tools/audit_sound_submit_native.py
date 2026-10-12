#!/usr/bin/env python3
"""Independent pins, actual image bytes, repeats and complete raw outputs."""
import argparse,json,hashlib
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,sha,save,encoded,Xbe
PRIOR=ROOT/'analysis/multiplayer/integration/sound-driver-owners-native/final-manifest.json'
PRIOR_PIN='0c6ed26b546fb0ceee6ec17900af768dee80e9ab2dace026d25f42f68487b04f'
def prior_pins():
 assert sha(PRIOR)==PRIOR_PIN;m=json.loads(PRIOR.read_text());pins={str(PRIOR.relative_to(ROOT)):PRIOR_PIN}
 for group in('owned_current_files','evidence_files','prior_pins','original_inputs'):
  for rel,q in m[group].items():
   value=q if isinstance(q,str)else q['sha256'];assert rel not in pins or pins[rel]==value;pins[rel]=value
 return pins

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();prior=prior_pins()
 for rel,pin in prior.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==pin,rel
 repeats=[]
 for origin,repeat in(('paired-owners','paired-repeat'),('measured-final','repeated')):
  for f in sorted((d/origin).rglob('*')):
   if not f.is_file():continue
   q=d/repeat/f.relative_to(d/origin);assert f.read_bytes()==q.read_bytes(),str(f);repeats.append(dict(file=str(f.relative_to(d)),repeat=str(q.relative_to(d)),sha256=sha(f)))
 for name in('fixtures.tsv','fixtures.json','literal-lockout-regions.bin'):
  assert(d/name).read_bytes()==(d/'export-repeat'/name).read_bytes();repeats.append(dict(file=name,repeat='export-repeat/'+name,sha256=sha(d/name)))
 rows=0
 for path,tag in((OLD,'old'),(NEW,'u5')):
  x=Xbe(path)
  for f in(d/'paired-owners/instructions').glob(f'*-{tag}.asm'):
   for line in f.read_text().splitlines():
    words=line.split();at=int(words[0],16);raw=bytes.fromhex(words[1]);assert x.read(at,len(raw))==raw,(f,line);rows+=1
  for line in(d/'measured-final/instructions'/f'{tag}-executed.asm').read_text().splitlines():
   words=line.split();raw=bytes.fromhex(words[1]);assert x.read(int(words[0],16),len(raw))==raw
 fixture=json.loads((d/'fixtures.json').read_text());assert fixture['cases']==2574 and fixture['ordered_writes']==196
 for directory in('native','native-repeat'):
  command=json.loads((d/directory/'commands.json').read_text());assert all(q['exit_code']==0 for q in command['commands']);assert(command['fixture_sha256']==sha(d/'fixtures.tsv'))
 for tag in('normal','sanitizer'):
  assert(d/'native'/f'{tag}.stdout').read_bytes()==(d/'native-repeat'/f'{tag}.stdout').read_bytes();assert not(d/'native'/f'{tag}.stderr').read_bytes();assert not(d/'native-repeat'/f'{tag}.stderr').read_bytes()
 packet=dict(schema='sound-submit-native-independent-audit-v1',audit_tool_sha256=sha(__file__),prior_pins_verified=prior,byte_equal_independent_files=repeats,original_full_body_instruction_rows_checked=rows,paired_report_sha256=sha(d/'paired-owners/report.json'),original_report_sha256=sha(d/'measured-final/report.json'),fixture_sha256=sha(d/'fixtures.tsv'),native_commands_sha256=sha(d/'native/commands.json'),normal_and_sanitizer_pass=True,full_source_driver_startup_or_voice_admission_claimed=False,native_audio_connected=False)
 save(d/'audit.json',encoded(packet));print(json.dumps(dict(audit_sha256=sha(d/'audit.json'),prior_pins=len(prior),independent_equal_files=len(repeats),original_instruction_rows=rows,mismatches=0)))
if __name__=='__main__':main()
