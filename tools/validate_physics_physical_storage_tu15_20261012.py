#!/usr/bin/env python3
"""Seal source authenticity, repeated original runs and exact native builds."""
import hashlib,json,sys,xml.etree.ElementTree as ET
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
if ROOT.name=='source-v1':ROOT=ROOT.parents[4]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_smg_firing_1_1 import Xbe,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
E=ROOT/'analysis/multiplayer/sandbox-source/physics-physical-storage-tu15-20261012'
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
 same_tree(E/'correspondence-v1',E/'correspondence-repeat');same_tree(E/'original-v1',E/'original-repeat')
 report=E/'correspondence-v1/complete-report.json';assert sha(report)=='4e81e7a5fd05edae4d24a35152896df4191aed827da55ba1d87b559bcb8e540f'
 paired=json.loads(report.read_text());original=json.loads((E/'original-v1/fixture.json').read_text())
 assert paired['tool_sha256']==sha(ROOT/'tools/research_physics_physical_storage_tu15_20261012.py')
 assert original['tool_sha256']==sha(ROOT/'tools/oracle_physics_physical_storage_tu15_20261012.py')
 assert original['correspondence_sha256']==sha(report) and original['original_sha256']==NEW_PIN
 oldpath=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';assert sha(oldpath)==PIN and sha(NEW_DEFAULT)==NEW_PIN
 old,new=Xbe(oldpath),Xbe(NEW_DEFAULT)
 for routine in paired['routines']:
  for row in routine['instructions']:
   for image,side in ((old,'old'),(new,'later')):
    raw=bytes.fromhex(row[side+'_bytes']);assert image.read(row[side],len(raw))==raw
 for row in paired['imports'].values():assert new.read(row['later'],4).hex()==row['hex']
 g=json.loads((E/'generated-v1.json').read_text());assert g==json.loads((E/'generated-repeat.json').read_text())
 assert g['generator_sha256']==sha(ROOT/'tools/generate_physics_physical_storage_tu15_20261012.py') and g['correspondence_sha256']==sha(report)
 assert g['generated_cpp_sha256']==sha(ROOT/'src/physics/physics_physical_storage_runtime_generated.cpp')==sha(E/'runtime-repeat.cpp')
 assert sum(len(s['steps'])for s in original['sequences'])==48 and sum(len(r['stores'])for s in original['sequences']for r in s['steps'])==216
 assert not original['captured_storage'] and not original['source_calls_replaced'] and not original['live_world_startup']
 builds={}
 for mode in ('release','sanitizer'):
  b=E/f'build-{mode}';tests=ET.parse(b/'tests.xml').findall('.//testcase')
  assert len(tests)==1 and tests[0].attrib['name']=='native_physics_physical_storage_tu15'
  assert all(not tests[0].findall(tag)for tag in ('failure','error','skipped'))
  log=(b/'tests.log').read_text();assert '100% tests passed'in log and '48 fresh TU1.5 physical stage lifecycle calls'in log
  commands=json.loads((b/'compile_commands.json').read_text())
  for row in commands:
   p=Path(row['file']);assert p.is_relative_to(source) and sha(p)==m['files'][str(p.relative_to(source))]
   assert '-fno-fast-math'in row['command'] and '-ffp-contract=off'in row['command']
   if mode=='sanitizer':assert '-fsanitize=address,undefined'in row['command']
  builds[mode]=dict(binary_sha256=sha(b/'physics_physical_storage_tests'),results_sha256=sha(b/'tests.xml'),tests_log_sha256=sha(b/'tests.log'),compile_commands_sha256=sha(b/'compile_commands.json'),tests=1)
 d=dict(schema=1,source_manifest_sha256=sha(E/'source-manifest-v1.json'),correspondence_sha256=sha(report),original_fixture_sha256=sha(E/'original-v1/fixture.json'),generated_manifest_sha256=sha(E/'generated-v1.json'),
  calls=48,ordered_writes=216,actual_original_instructions=1689,executed_sites=82,compiled_sites=86,fresh_allocations=3,protection_calls=35,final_retained_bytes=153796608,
  builds=builds,parity_target='English TU1.5 only',live_physics_parity=False,complete_game_arena_startup=False,complete_world_startup=False)
 p=E/'validation.json';raw=json.dumps(d,sort_keys=True,indent=2)+'\n'
 if p.exists():assert p.read_text()==raw,'Preserve prior seal'
 else:p.write_text(raw)
 print('PASS fresh TU1.5 physical stage/partition, source and native builds '+sha(p))
if __name__=='__main__':main()
