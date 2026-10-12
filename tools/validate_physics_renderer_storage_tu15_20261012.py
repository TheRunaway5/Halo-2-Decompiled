#!/usr/bin/env python3
"""Seal renderer raw-storage bodies and native original-prefix comparison."""
import hashlib,json,sys,xml.etree.ElementTree as ET
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
if ROOT.name=='source-v1':ROOT=ROOT.parents[4]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_smg_firing_1_1 import Xbe,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
E=ROOT/'analysis/multiplayer/sandbox-source/physics-renderer-storage-tu15-20261012'
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
 p=E/'correspondence-v1/complete-report.json';assert sha(p)=='59dd620260f89a6ccc4900879f7c39281117feeec6eff042a6722f6c055f83d5'
 paired=json.loads(p.read_text());original=json.loads((E/'original-v1/fixture.json').read_text())
 assert paired['tool_sha256']==sha(ROOT/'tools/research_physics_renderer_storage_tu15_20261012.py')
 assert original['oracle_sha256']==sha(ROOT/'tools/oracle_physics_renderer_storage_tu15_20261012.py')
 assert original['correspondence_sha256']==sha(p) and original['source_sha256']==NEW_PIN
 assert not original['instruction_patches'] and not original['supplied_source_results']
 assert not original['complete_renderer_producer_return'] and not original['live_renderer_or_physics_parity']
 oldpath=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';assert sha(oldpath)==PIN and sha(NEW_DEFAULT)==NEW_PIN
 old,new=Xbe(oldpath),Xbe(NEW_DEFAULT)
 assert paired['complete_bodies']==3 and len(paired['instruction_map'])==192
 for routine in paired['routines']:
  for row in routine['instructions']:
   for image,side in ((old,'old'),(new,'later')):
    raw=bytes.fromhex(row[side+'_bytes']);assert image.read(row[side],len(raw))==raw
 pool=E.parent/'physics-pool-startup-tu15-20261012/correspondence-v1/complete-report.json'
 assert original['pool_correspondence_sha256']==sha(pool)
 g=json.loads((E/'generated-v1.json').read_text());assert g==json.loads((E/'generated-repeat.json').read_text())
 assert g['generator_sha256']==sha(ROOT/'tools/generate_physics_renderer_storage_tu15_20261012.py') and g['correspondence_sha256']==sha(p)
 assert g['generated_cpp_sha256']==sha(ROOT/'src/physics_renderer_storage_runtime_generated.cpp')==sha(E/'runtime-repeat.cpp')
 assert len(g['lowered_instructions'])==192 and len(g['complete_native_entries'])==3
 assert not g['production_opcode_decoder'] and not g['production_original_code_execution']
 f=json.loads((E/'call-frames.json').read_text());assert f['tool_sha256']==sha(ROOT/'tools/export_physics_renderer_storage_frames_tu15_20261012.py')
 assert f['report_sha256']==sha(p) and f['header_sha256']==sha(ROOT/'src/physics_renderer_storage_call_frames.hpp') and len(f['frames'])==23
 cases=original['cases'];assert len(cases)==12
 assert original['ordered_writes']==sum(len(c['stores'])for c in cases)==60
 assert original['original_instruction_count']==744 and original['executed_sites']==62
 assert sum(b['bytes']for c in cases for b in c['final_banks'])==20263200
 assert original['missing_allocator_target']==0x32a3bb and original['missing_allocator_return']==0x137483
 assert len({c['token']for c in cases})==3 and {c['shift']for c in cases}=={0,4}
 for c in cases:
  assert c['renderer_base']==c['context_base']==c['token'] and c['pending_stack']=='8374130018000000'
  assert len(c['resources'])==1
  r=c['resources'][0];assert r['request']==[0,0,0x452e8,0x101000,4] and r['result']==[0,c['token'],0x46000]
  assert r['initial_sha256']==hashlib.sha256(bytes(0x46000)).hexdigest()
 names={'native_physics_pool_startup_tu15','native_physics_renderer_storage_tu15'}
 builds={}
 for mode in ('release','sanitizer','thread'):
  b=E/f'build-{mode}';tests=ET.parse(b/'tests.xml').findall('.//testcase');log=(b/'tests.log').read_text()
  assert len(tests)==2 and {t.attrib['name']for t in tests}==names
  assert all(not t.findall(tag)for t in tests for tag in ('failure','error','skipped'))
  assert '100% tests passed'in log and '12 fresh TU1.5 renderer producer prefixes'in log
  commands=json.loads((b/'compile_commands.json').read_text())
  for row in commands:
   p=Path(row['file']);assert p.is_relative_to(source) and sha(p)==m['files'][str(p.relative_to(source))]
   assert '-fno-fast-math'in row['command'] and '-ffp-contract=off'in row['command'] and '-frounding-math'in row['command']
   if mode=='sanitizer':assert '-fsanitize=address,undefined'in row['command']
   if mode=='thread':assert '-fsanitize=thread'in row['command']
  builds[mode]=dict(status='passed',binary_sha256=sha(b/'physics_renderer_storage_tests'),results_sha256=sha(b/'tests.xml'),tests_log_sha256=sha(b/'tests.log'),compile_commands_sha256=sha(b/'compile_commands.json'),tests=2)
 d=dict(schema=1,source_manifest_sha256=sha(E/'source-manifest-v1.json'),correspondence_sha256=sha(E/'correspondence-v1/complete-report.json'),original_fixture_sha256=sha(E/'original-v1/fixture.json'),generated_manifest_sha256=sha(E/'generated-v1.json'),
  fresh_prefix_cases=12,required_allocator_frontiers=12,ordered_writes=60,actual_original_instructions=744,executed_sites=62,new_compiled_sites=192,final_bank_bytes=20263200,native_virtual_allocations=12,
  missing_raw_owner_frontiers=12,native_resource_retirement=True,builds=builds,
  parity_target='English TU1.5 only',required_allocator_target='0032a3bb',required_allocator_return='00137483',whole_renderer_initializer_return=False,live_physics_parity=False,complete_occupied_world_startup=False)
 p=E/'validation.json';raw=json.dumps(d,sort_keys=True,indent=2)+'\n'
 if p.exists():assert p.read_text()==raw,'Preserve prior seal'
 else:p.write_text(raw)
 print('PASS fresh TU1.5 renderer storage source/native audit '+sha(p))
if __name__=='__main__':main()
