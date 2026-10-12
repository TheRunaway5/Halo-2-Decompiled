#!/usr/bin/env python3
"""Audit new source-creation evidence without altering any prior freeze."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,sha,save,encoded
from inspect_multiplayer import Cache
PRIOR=ROOT/'analysis/multiplayer/integration/sound-promotion-native/final-manifest.json'
PRIOR_PIN='eb053b6680b4917f9d03eec27b078c725932a9b4e4cb369e13ebbc2a7c11940e'
def prior_pins():
 assert sha(PRIOR)==PRIOR_PIN
 pins={str(PRIOR.relative_to(ROOT)):PRIOR_PIN};m=json.loads(PRIOR.read_text())
 for group in('owned_current_files','evidence_files','prior_pins','original_inputs'):
  for rel,q in m[group].items():
   value=q if isinstance(q,str)else q['sha256'];assert rel not in pins or pins[rel]==value;pins[rel]=value
 return pins

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();pins=prior_pins()
 for rel,pin in pins.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==pin,rel
 repeats=[]
 for source,repeat in(('paired-v2','paired-repeat'),('measured','repeated')):
  for f in sorted((d/source).rglob('*')):
   if not f.is_file():continue
   q=d/repeat/f.relative_to(d/source);assert f.read_bytes()==q.read_bytes(),str(f)
   repeats.append(dict(file=str(f.relative_to(d)),repeat=str(q.relative_to(d)),sha256=sha(f)))
 for name in('fixtures.tsv','fixtures.json'):
  assert(d/name).read_bytes()==(d/'export-repeat'/name).read_bytes();repeats.append(dict(file=name,repeat='export-repeat/'+name,sha256=sha(d/name)))
 rows=0
 for image,tag in((OLD,'old'),(NEW,'u5')):
  x=Xbe(image)
  for f in list((d/'paired-v2/instructions').glob(f'*-{tag}.asm'))+list((d/'measured/instructions').glob(f'{tag}-*.asm')):
   for line in f.read_text().splitlines():
    q=line.split();raw=bytes.fromhex(q[1]);assert x.read(int(q[0],16),len(raw))==raw,(f,line);rows+=1
 r=json.loads((d/'measured/report.json').read_text());fixture=json.loads((d/'fixtures.json').read_text());providers=json.loads((d/'measured/providers.json').read_text())
 assert fixture['calls']==138 and fixture['stores']==14858 and len(r['maps'])==46
 for q in providers['words']:
  assert Xbe(OLD).read(int(q['old_va'],16),4).hex()==q['raw_hex']==Xbe(NEW).read(int(q['u5_va'],16),4).hex()
 ranges=[];map_pairs=0;continuations=[]
 for old,new in zip(r['maps'][::2],r['maps'][1::2]):
  assert old['image']=='old'and new['image']=='u5'
  for name in('system','tag_table','current_matg','current_ugh','current_scenario','sources','controllers','class_fades','local_rng','environment','cache_pool'):assert old['state'][name]==new['state'][name]
  assert old['regions'][:-2]==new['regions'][:-2]
  for a,b in zip(old['cases'],new['cases']):
   for field in('source_bc_hex','final_rng_hex','final_ugh_hex','final_arena_hex','final_request_hex','sound','raw_request_hex'):assert a[field]==b[field]
   clean=lambda q:[{k:v for k,v in z.items()if k!='actual_instruction'}for z in q['stores']]
   assert clean(a)==clean(b)
   for field in('sound','chunk','flags'):assert a['cache_request'][field]==b['cache_request'][field]
  path=Path(old['map']['path']);assert sha(path)==old['map']['sha256'];c=Cache(path)
  ugh=next(t for t in c.tags.values()if t['group']=='ugh!')
  counts=[int.from_bytes(c.data[q+10:q+12],'little',signed=True)for q in c.block(ugh['offset']+0x20,12)]
  assert all(1<=n<=31 for n in counts);ranges.append(dict(path=str(path),rows=len(counts),minimum=min(counts),maximum=max(counts)))
  c.data.close();c.stream.close();map_pairs+=1
  if old['continuation']:
   q=old['continuation'];assert q['stop']['entry']=='800000ea'and q['stop']['original_return_site']=='002dc8d5'and q['stop']['arguments'][:4]==['00000000','00000001','00000000','00000000'];assert not q['whole127750_returned']and not q['voice_or_IO_success_supplied'];continuations.append(q['stop'])
 assert len(continuations)==1
 for directory in('native','native-repeat-run/native'):
  commands=json.loads((d/directory/'commands.json').read_text());assert all(q['exit_code']==0 for q in commands['commands'])and commands['fixture_sha256']==sha(d/'fixtures.tsv')
  for tag in('normal-build','sanitizer-build','normal','sanitizer'):assert not(d/directory/f'{tag}.stderr').read_bytes()
 for tag in('normal','sanitizer'):assert(d/'native'/f'{tag}.stdout').read_bytes()==(d/'native-repeat-run/native'/f'{tag}.stdout').read_bytes()
 packet=dict(schema='sound-create-native-audit-v1',audit_tool_sha256=sha(__file__),prior_pins_verified=pins,independent_byte_equal_files=repeats,
  original_instruction_rows_checked=rows,old_u5_full_output_bank_pairs=map_pairs,owned_permutation_range_domains=ranges,resource_continuation_stops=continuations,
  original_report_sha256=sha(d/'measured/report.json'),providers_sha256=sha(d/'measured/providers.json'),fixture_sha256=sha(d/'fixtures.tsv'),
  normal_and_sanitizer_pass=True,cache_results_or_admission_supplied=False,native_audio_connected=False)
 save(d/'audit.json',encoded(packet));print(json.dumps(dict(audit_sha256=sha(d/'audit.json'),prior_pins=len(pins),independent_equal_files=len(repeats),instruction_rows=rows,bank_pairs=map_pairs,mismatches=0)))
if __name__=='__main__':main()
