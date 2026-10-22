#!/usr/bin/env python3
"""Seal fresh arena source authenticity and exact native comparison evidence."""
import hashlib,json,sys,xml.etree.ElementTree as ET
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
if ROOT.name=='source-v1':ROOT=ROOT.parents[4]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_smg_firing_1_1 import Xbe,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
E=ROOT/'analysis/multiplayer/sandbox-source/physics-game-arena-tu15-20261012'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def same_tree(a,b):
 def files(d):return {str(p.relative_to(d)):sha(p)for p in d.rglob('*')if p.is_file()}
 assert files(a)==files(b),(a,b)
def main():
 m=json.loads((E/'source-manifest-v1.json').read_text());source=E/'source-v1'
 for p,h in m['files'].items():
  assert sha(source/p)==h,p
  if p!='CMakeLists.txt':assert sha(ROOT/p)==h,p
 assert sha(E/'dependencies.d')==m['dependencies_sha256']
 same_tree(E/'correspondence-v1',E/'correspondence-repeat');same_tree(E/'original-v2',E/'original-v2-repeat')
 report=E/'correspondence-v1/complete-report.json';assert sha(report)=='616bee89417916769418d9f2a70075ab036c2bba8a1decadd46698c5d57f1933'
 paired=json.loads(report.read_text());original=json.loads((E/'original-v2/report.json').read_text())
 assert paired['tool_sha256']==sha(ROOT/'tools/research_physics_game_arena_tu15_20261012.py')
 assert original['oracle_sha256']==sha(ROOT/'tools/oracle_physics_game_arena_tu15_20261012.py')
 assert original['correspondence_sha256']==sha(report) and original['source_sha256']==NEW_PIN
 assert not original['instruction_patches'] and not original['supplied_gameplay_results'] and not original['live_match_proof']
 oldpath=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';assert sha(oldpath)==PIN and sha(NEW_DEFAULT)==NEW_PIN
 old,new=Xbe(oldpath),Xbe(NEW_DEFAULT)
 for routine in paired['routines']:
  for row in routine['instructions']:
   for image,side in ((old,'old'),(new,'later')):
    raw=bytes.fromhex(row[side+'_bytes']);assert image.read(row[side],len(raw))==raw
 g=json.loads((E/'generated-v1.json').read_text());assert g==json.loads((E/'generated-repeat.json').read_text())
 assert g['generator_sha256']==sha(ROOT/'tools/generate_physics_game_arena_tu15_20261012.py') and g['correspondence_sha256']==sha(report)
 assert g['generated_cpp_sha256']==sha(ROOT/'src/physics/physics_game_arena_runtime_generated.cpp')==sha(E/'runtime-repeat.cpp')
 frame=json.loads((E/'call-frames.json').read_text());assert frame['tool_sha256']==sha(ROOT/'tools/export_physics_game_arena_frames_tu15_20261012.py')
 assert frame['correspondence_sha256']==sha(report) and frame['header_sha256']==sha(ROOT/'src/physics/physics_game_arena_call_frames.hpp') and frame['frames']==37
 assert len(original['steps'])==13 and original['stores']==1049656 and original['executed_original_instructions']==1065823
 assert original['store_sha256']==sha(E/'original-v2'/original['store_file'])
 builds={}
 for mode in ('release','sanitizer','thread'):
  b=E/f'build-{mode}';tests=ET.parse(b/'tests.xml').findall('.//testcase');log=(b/'tests.log').read_text()
  assert {t.attrib['name']for t in tests}=={'native_physics_physical_storage_tu15','native_physics_game_arena_tu15'}
  commands=json.loads((b/'compile_commands.json').read_text())
  for row in commands:
   p=Path(row['file']);assert p.is_relative_to(source) and sha(p)==m['files'][str(p.relative_to(source))]
   assert '-fno-fast-math'in row['command'] and '-ffp-contract=off'in row['command']
   if mode=='sanitizer':assert '-fsanitize=address,undefined'in row['command']
   if mode=='thread':assert '-fsanitize=thread'in row['command']
  passed=all(not t.findall(tag)for t in tests for tag in ('failure','error','skipped'))
  if not passed:
   assert mode=='thread' and ('FATAL: ThreadSanitizer' in log or 'ThreadSanitizer: unexpected memory mapping' in log),'Actual native test failure: '+mode
   status='runtime unavailable; no race-free claim'
  else:
   assert '100% tests passed'in log and '13 complete TU1.5 arena startup/client calls'in log;status='passed'
  builds[mode]=dict(status=status,binary_sha256=sha(b/'physics_game_arena_tests'),results_sha256=sha(b/'tests.xml'),tests_log_sha256=sha(b/'tests.log'),compile_commands_sha256=sha(b/'compile_commands.json'),tests=len(tests))
 d=dict(schema=1,source_manifest_sha256=sha(E/'source-manifest-v1.json'),correspondence_sha256=sha(report),original_fixture_sha256=sha(E/'original-v2/report.json'),generated_manifest_sha256=sha(E/'generated-v1.json'),
  calls=13,ordered_writes=1049656,actual_original_instructions=1065823,executed_sites=577,compiled_arena_sites=730,compiled_physical_sites=86,final_bank_bytes=52667416,
  actual_files=2,native_ungated_startup=True,missing_owner_frontiers=True,native_stop_join=True,builds=builds,
  parity_target='English TU1.5 only',live_physics_parity=False,complete_occupied_world_startup=False)
 p=E/'validation.json';raw=json.dumps(d,sort_keys=True,indent=2)+'\n'
 if p.exists():assert p.read_text()==raw,'Preserve prior seal'
 else:p.write_text(raw)
 print('PASS complete fresh TU1.5 arena source/native audit '+sha(p))
if __name__=='__main__':main()
