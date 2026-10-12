#!/usr/bin/env python3
"""Audit original CPU startup repeats, immutable operands and native snapshots."""
import argparse,json,re
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,sha,save,encoded
from oracle_smg_audio_rng_startup_1_1 import FROZEN

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory
 comparisons=[]
 for left,right in(('measured','repeated'),('paired','paired-repeat'),('','export-repeat')):
  base=d/left;other=d/right
  paths=[q for q in base.rglob('*')if q.is_file()]if left else[d/'fixtures.tsv',d/'fixtures.json']
  for q in paths:
   rel=q.relative_to(base);copy=other/rel;assert copy.is_file()and q.read_bytes()==copy.read_bytes(),(q,copy)
   comparisons.append(dict(left=str(q.relative_to(d)),right=str(copy.relative_to(d)),sha256=sha(q)))
 r=json.loads((d/'measured/report.json').read_text());pair=json.loads((d/'paired/report.json').read_text());providers=json.loads((d/'paired/providers.json').read_text())
 images={'old':Xbe(OLD),'u5':Xbe(NEW)};rows=0;addresses=set()
 for q in pair['owners']:
  for tag in images:
   listing=d/'paired/instructions'/f'{q["old"]}-{tag}.asm'
   for line in listing.read_text().splitlines():
    addr,raw,*_=line.split();assert images[tag].read(int(addr,16),len(bytes.fromhex(raw)))==bytes.fromhex(raw),(listing,line)
    rows+=1
    if tag=='old':addresses.add(int(addr,16))
 for q in(d/'measured/instructions').glob('*.asm'):
  for line in q.read_text().splitlines():
   addr,raw,*_=line.split();assert images['old'].read(int(addr,16),len(bytes.fromhex(raw)))==bytes.fromhex(raw)
 for key,extent in(('ambient_transition_bits',4),('voice_policy',16)):
  q=providers[key];raw=bytes.fromhex(q[f'raw{extent}_hex'])
  for tag in images:assert images[tag].read(int(q['old_va'if tag=='old'else'u5_va'],16),extent)==raw
 for rel in('include/halo2/sound_startup.hpp','src/sound_startup.cpp','tests/sound_startup_tests.cpp'):
  assert (ROOT/rel).read_bytes()==(d/'native/source'/rel).read_bytes(),rel
 # Every nonzero represented instruction is present in the complete original
 # body set. Native byte copies for strncpy are explicitly identity0; the
 # complete output comparison does not claim its individual C-helper stores.
 cpp=(ROOT/'src/sound_startup.cpp').read_text()
 sites={int(q,16)for q in re.findall(r'(?:write|clear|global)\(0x([0-9a-f]+)',cpp)}
 sites|={int(q,16)for q in re.findall(r'allocate\(0x[0-9a-f]+,0x([0-9a-f]+)',cpp)}
 assert sites<=addresses,sorted(hex(q)for q in sites-addresses)
 for rel,pin in FROZEN.items():assert sha(ROOT/rel)==pin,rel
 for log in('normal.stdout','sanitizer.stdout'):
  text=(d/'native'/log).read_text();assert 'PASS original CPU startup prefix/full arena, 2188' in text and '741 complete commit stores' in text,text
 audit=dict(schema='source-sound-startup-native-audit-v1',audit_tool_sha256=sha(__file__),
   source_report_sha256=sha(d/'measured/report.json'),paired_report_sha256=sha(d/'paired/report.json'),providers_sha256=sha(d/'paired/providers.json'),
   original_constructor_prefix_stores=len(r['stores']),represented_native_prefix_stores=2188,
   original_complete_commit_fragment_stores=len(r['commit_fragment']['stores']),complete_arena_bytes=r['initial_static_arena_protocol']['bytes'],
   whole_paired_bodies=len(pair['owners']),paired_original_instruction_rows_verified=rows,
   independent_byte_equal_files=comparisons,prior_pins_verified=FROZEN,
   native_results={q:dict(binary_sha256=sha(d/'native'/q),stdout_sha256=sha(d/'native'/log))for q,log in(('sound_startup_tests','normal.stdout'),('sound_startup_tests_asan','sanitizer.stdout'))},
   limits=r['bounds']+['Native cache fixture checks its separately executed original zero effects on this module-owned arena; no native complete cache startup or successful platform return is used by the prefix fixture.',
     'Separate native commit-fragment check is source CPU arithmetic/state evidence, not a successful real device, full portable startup or audible playback session.'])
 save(d/'audit.json',encoded(audit));print(json.dumps(dict(audit_sha256=sha(d/'audit.json'),independent_files=len(comparisons),paired_instruction_rows=rows,prior_pins=len(FROZEN))))

if __name__=='__main__':main()
