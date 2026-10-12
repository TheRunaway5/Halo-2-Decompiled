#!/usr/bin/env python3
"""Check independent original round-lifecycle artifacts and record exact pins.

This verifies original instruction evidence, not native game mode operation.
Run oracle_round_lifecycle.py twice as documented before invoking this verifier.
"""
import argparse, hashlib, json, sys
from pathlib import Path
from oracle_round_lifecycle import ROOT, OUT, OWNERS
from oracle_game_over import SPECS
from oracle_mp_full_scoreboard_1_5 import sha

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--directory',type=Path,default=OUT)
 ap.add_argument('--output',type=Path,default=OUT.parent/'round-lifecycle-verification.json')
 a=ap.parse_args();report_path=a.directory/'report.json';report=json.loads(report_path.read_text())
 reproduced=a.directory/'reproduced';fixture=ROOT/'tests/fixtures/round_lifecycle_original.json'
 assert report_path.read_bytes()==(reproduced/'report.json').read_bytes(),'Independent complete reports differ'
 assert fixture.read_bytes()==(reproduced/'fixture.json').read_bytes(),'Independent complete fixtures differ'
 assert sha(fixture)==report['fixture_sha256'] and sha(ROOT/'tools/oracle_round_lifecycle.py')==report['oracle_sha256']
 assert not report['instruction_patches'] and not report['supplied_returns']
 pins={}
 def pin(path,expected=None):
  path=Path(path);digest=sha(path)
  if expected is not None:assert digest==expected,str(path)
  pins[str(path.relative_to(ROOT))]=digest
 for path in(report_path,reproduced/'report.json',fixture,reproduced/'fixture.json',Path(__file__),
  ROOT/'docs/MP_ROUND_LIFECYCLE_RESEARCH.md',ROOT/'docs/GAME_MODE_PROGRESS.md',ROOT/'docs/MP_PLAYER_LIVES_RESEARCH.md'):
  pin(path)
 for relative,digest in report['dependencies'].items():pin(ROOT/relative,digest)
 # Pin the loaded project helpers as well as the oracle's direct dependencies.
 # Third-party interpreter/packages are outside these project-file pins.
 for module in list(sys.modules.values()):
  file=getattr(module,'__file__',None)
  if file:
   path=Path(file).resolve()
   if path.is_relative_to(ROOT/'tools')and path.suffix=='.py':pin(path)
 for name,spec in SPECS.items():
  pin(ROOT/spec['tsv']);assert sha(ROOT/spec['path'])==report['versions'][name]['image_sha256']
 listings=report['instruction_listings'];assert len(listings)==206
 for row in listings:
  original=a.directory/'instructions'/row['file'];independent=reproduced/'instructions'/row['file']
  pin(original,row['sha256']);pin(independent,row['sha256'])
  assert original.read_bytes()==independent.read_bytes()
  assert len(original.read_text().splitlines())==row['instructions']
 assert len(report['complete_source_pairs'])==16 and all(p['complete_shape_equal']for p in report['complete_source_pairs'])
 blobs=report['raw_blobs']
 for digest,raw in blobs.items():assert hashlib.sha256(bytes.fromhex(raw)).hexdigest()==digest
 refs=set()
 def check_refs(value):
  if isinstance(value,dict):
   if 'raw_blob_sha256'in value:
    assert set(value)=={'raw_blob_sha256'}and value['raw_blob_sha256']in blobs;refs.add(value['raw_blob_sha256'])
   else:
    for child in value.values():check_refs(child)
  elif isinstance(value,list):
   for child in value:check_refs(child)
 check_refs(report);assert refs==set(blobs)
 for name,version in report['versions'].items():
  assert version['owners']==OWNERS[name] and version['netgame_rows']==32
  assert len(version['phase_rows'])==145 and len(version['tick_rows'])==432
  for chain in version['first_to_three']:
   assert chain['completion']=='complete_original_return'
   assert chain['variant']['explicit_edits']['11']==0
   if chain['team_mode']:assert chain['variant']['explicit_edits']['91']==1
  for chain in version['reset_map_enabled_dependency_stops']:
   assert chain['completion']=='stopped_original_instruction'
   assert '11'not in chain['variant'].get('explicit_edits',{})
  assert all(p['completion']=='stopped_original_instruction'for p in version['live_object_dependency_probes'])
 assets={}
 for name,spec in SPECS.items():assets[spec['path']]=sha(ROOT/spec['path'])
 for name,key in(('lockout','lockout_sha256'),('mainmenu','mainmenu_sha256')):
  path=ROOT/f'assets/disc/maps/{name}.map';assert sha(path)==report[key];assets[str(path.relative_to(ROOT))]=report[key]
 result=dict(schema='round-lifecycle-verification-v1',scope='Original instruction evidence; no native operational claim',
  complete_original_calls=report['whole_calls'],independent_fixture_equal=True,independent_report_equal=True,
  independent_instruction_listings_equal=len(listings),static_instruction_rows=sum(r['instructions']for r in listings),
  complete_source_pairs=16,edited_empty_world_first_to_three_sequences=6,reset_map_enabled_dependency_stops=6,
  declared_live_object_dependency_stops=12,raw_blobs_verified=len(blobs),files=pins,assets=assets,
  limits=['Current units NONE and empty object pool in positive round sequences',
   'Reset-map disabled through original parameter11 setter in completed sequences',
   'Original full world frame, constructed live teardown/full map reset, native multi-round gameplay, HUD and network not established',
   'No native C++ changes, rebuild, sanitizer, rendered match or physical LAN checks in this research slice'])
 a.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
 assert all(sha(ROOT/p)==digest for p,digest in pins.items())
 print(json.dumps(dict(complete_original_calls=report['whole_calls'],independent_fixture_and_report_equal=True,
  instruction_listings_equal=len(listings),project_and_artifact_pins=len(pins),asset_pins=len(assets)),indent=2))
if __name__=='__main__':main()
