#!/usr/bin/env python3
"""Seal authentic early initializer bodies and complete native prefix comparison."""
import hashlib,json,sys,xml.etree.ElementTree as ET
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
if ROOT.name=='source-v1':ROOT=ROOT.parents[4]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_smg_firing_1_1 import Xbe,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
E=ROOT/'analysis/multiplayer/sandbox-source/physics-early-initializer-tu15-20261012'
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
 assert m['main_production_module_sha256']==sha(ROOT/'cmake/PhysicsMainHeap.cmake')
 assert 'src/physics/physics_main_heap_runtime_generated.cpp' in (ROOT/'cmake/PhysicsMainHeap.cmake').read_text()
 same_tree(E/'correspondence-v1',E/'correspondence-repeat');same_tree(E/'original-v1',E/'original-repeat')
 p=E/'correspondence-v1/complete-report.json';assert sha(p)=='0504f7dd7d9ca46abb6d129f899b5f711214768b28c7db3376850d61787dd084'
 paired=json.loads(p.read_text());original=json.loads((E/'original-v1/report.json').read_text())
 assert paired['tool_sha256']==sha(ROOT/'tools/research_physics_early_initializer_tu15_20261012.py')
 assert original['oracle_sha256']==sha(ROOT/'tools/oracle_physics_early_initializer_tu15_20261012.py')
 assert original['early_correspondence_sha256']==sha(p) and original['source_sha256']==NEW_PIN
 assert not original['instruction_patches'] and not original['supplied_gameplay_results'] and not original['live_match_proof']
 oldpath=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';assert sha(oldpath)==PIN and sha(NEW_DEFAULT)==NEW_PIN
 old,new=Xbe(oldpath),Xbe(NEW_DEFAULT)
 assert paired['complete_bodies']==2 and len(paired['instruction_map'])==45
 for routine in paired['routines']:
  for row in routine['instructions']:
   for image,side in ((old,'old'),(new,'later')):
    raw=bytes.fromhex(row[side+'_bytes']);assert image.read(row[side],len(raw))==raw
 for directory,key in [('physics-game-arena-tu15-20261012','correspondence_sha256'),('physics-physical-storage-tu15-20261012','physical_correspondence_sha256'),('physics-pool-startup-tu15-20261012','pool_correspondence_sha256'),('physics-main-heap-tu15-20261012','heap_correspondence_sha256')]:
  assert original[key]==sha(E.parent/directory/'correspondence-v1/complete-report.json')
 g=json.loads((E/'generated-v1.json').read_text());assert g==json.loads((E/'generated-repeat.json').read_text())
 assert g['generator_sha256']==sha(ROOT/'tools/generate_physics_early_initializer_tu15_20261012.py') and g['correspondence_sha256']==sha(p)
 assert g['generated_cpp_sha256']==sha(ROOT/'src/physics/physics_early_initializer_runtime_generated.cpp')==sha(E/'runtime-repeat.cpp')
 assert len(g['lowered_instructions'])==45 and not g['production_opcode_decoder'] and not g['production_original_code_execution']
 f=json.loads((E/'call-frames.json').read_text());assert f['tool_sha256']==sha(ROOT/'tools/export_physics_early_initializer_frames_tu15_20261012.py')
 assert f['reports'][0]==sha(p) and f['header_sha256']==sha(ROOT/'src/physics/physics_early_initializer_call_frames.hpp') and len(f['frames'])==8
 assert len(original['steps'])==5 and [s['pending_renderer']for s in original['steps']]==[False,False,False,True,False]
 assert original['stores']==1049789 and original['executed_original_instructions']==1066041 and original['executed_sites']==834
 assert original['store_sha256']==sha(E/'original-v1'/original['store_file'])
 assert sum(b['bytes']for b in original['final_banks'])==53842968
 assert len([s for s in original['services']if s['kind']=='virtual_allocate'])==4
 names={'native_physics_physical_storage_tu15','native_physics_game_arena_tu15','native_physics_pool_startup_tu15','native_physics_early_initializer_tu15'}
 builds={}
 for mode in ('release','sanitizer','thread'):
  b=E/f'build-{mode}';tests=ET.parse(b/'tests.xml').findall('.//testcase');log=(b/'tests.log').read_text()
  assert len(tests)==4 and {t.attrib['name']for t in tests}==names
  assert all(not t.findall(tag)for t in tests for tag in ('failure','error','skipped'))
  assert '100% tests passed'in log and '5 TU1.5 startup/initializer phases'in log
  commands=json.loads((b/'compile_commands.json').read_text())
  for row in commands:
   p=Path(row['file']);assert p.is_relative_to(source) and sha(p)==m['files'][str(p.relative_to(source))]
   assert '-fno-fast-math'in row['command'] and '-ffp-contract=off'in row['command'] and '-frounding-math'in row['command']
   if mode=='sanitizer':assert '-fsanitize=address,undefined'in row['command']
   if mode=='thread':assert '-fsanitize=thread'in row['command']
  builds[mode]=dict(status='passed',binary_sha256=sha(b/'physics_early_initializer_tests'),results_sha256=sha(b/'tests.xml'),tests_log_sha256=sha(b/'tests.log'),compile_commands_sha256=sha(b/'compile_commands.json'),tests=4)
 d=dict(schema=1,source_manifest_sha256=sha(E/'source-manifest-v1.json'),correspondence_sha256=sha(E/'correspondence-v1/complete-report.json'),original_fixture_sha256=sha(E/'original-v1/report.json'),generated_manifest_sha256=sha(E/'generated-v1.json'),
  phases=5,complete_returned_calls=4,required_owner_frontiers=1,ordered_writes=1049789,actual_original_instructions=1066041,executed_sites=834,new_compiled_sites=45,final_bank_bytes=53842968,native_virtual_allocations=4,
  async_worker_retired_before_synchronous_pool_owner=True,missing_crc_frontier=True,native_resource_retirement=True,builds=builds,
  parity_target='English TU1.5 only',required_renderer_target='00149680',required_renderer_return='001cb184',whole_early_initializer_return=False,live_physics_parity=False,complete_occupied_world_startup=False)
 p=E/'validation.json';raw=json.dumps(d,sort_keys=True,indent=2)+'\n'
 if p.exists():assert p.read_text()==raw,'Preserve prior seal'
 else:p.write_text(raw)
 print('PASS fresh TU1.5 early initializer source/native audit '+sha(p))
if __name__=='__main__':main()
