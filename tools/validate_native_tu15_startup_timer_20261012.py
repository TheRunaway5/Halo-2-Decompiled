#!/usr/bin/env python3
"""Seal the native timer resource, original heap prefix and original power DPC."""
import hashlib,json,sys,xml.etree.ElementTree as ET
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
E=ROOT/'analysis/multiplayer/sandbox-source/native-tu15-startup-timer-20261012'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read(p):return json.loads(Path(p).read_text())
def main():
 m=read(E/'source-manifest-v1.json');source=E/'source-v1'
 for name,pin in m['files'].items():assert sha(source/name)==pin==sha(ROOT/name),name
 assert sha(source/'CMakeLists.txt')==m['top_sha256'] and sha(E/'dependencies.d')==m['dependencies_sha256']
 parent=E.parent/'native-tu15-startup-platform-20261012/source-manifest-v1.json';assert sha(parent)==m['parent_manifest_sha256'];p=read(parent)
 for name in m['archive_compiler_inputs']:assert sha(ROOT/name)==p['files'][name]
 for mode,row in m['archives'].items():assert sha(source/row['file'])==row['sha256']==p['archives'][mode]['sha256']
 f=read(E/'original-v5/fixture.json');assert f==read(E/'original-repeat/fixture.json')
 assert f['tool_sha256']==sha(ROOT/'tools/oracle_native_tu15_startup_timer_20261012.py')
 assert f['original_sha256']==sha(NEW_DEFAULT)==NEW_PIN and len(f['source_instructions'])==1143
 xbe=Xbe(NEW_DEFAULT)
 for pc,row in f['source_instructions'].items():assert xbe.read(int(pc,16),len(bytes.fromhex(row['raw'])))==bytes.fromhex(row['raw'])
 assert not f['source_instruction_patches'] and not f['older_image_executed'] and not f['captured_world_input_used'] and not f['native_expected_data_used']
 assert f['timer_armed'] and f['process_heap_installed_in_full_startup'] and not f['full_startup_complete'] and not f['live_physics_parity']
 assert not any(f['native_environment'][k]for k in ('bios_input_used','eeprom_input_used','kernel_tables_used'))
 cases=f['cases'];assert len(cases)==48 and sum(c['automatic_power_down']for c in cases)==24
 assert sum(len(c['instructions'])for c in cases)==31656 and sum(len(c['stores'])for c in cases)==13008
 assert sum(len(v['stores'])for c in cases for v in c['platform_calls'])==1008
 assert sum(len(v['stores'])for c in cases for v in c['native_calls'])==672
 assert sum(len(c['resources'])for c in cases)==48
 assert sum(len(bytes.fromhex(b['hex']))for c in cases for b in c['final_banks'])==121536
 assert sum(c['heap_pointer']==0x10000000 for c in cases)==24 and sum(c['heap_pointer']==0 for c in cases)==24
 assert sum(len(c['power_dpc']['instructions'])for c in cases)==144 and sum(len(c['power_dpc']['stores'])for c in cases)==24
 for c in cases:
  assert not (c['before']['eflags']&0x400)
  assert [v['target']for v in c['native_calls']]==[0x80000018,0x8000012d,0x80000095]
  if c['heap_pointer']:
   assert c['boundary']['target']==0x2dba2d and c['boundary']['frame']=='a8ba2d0038de4600'
   assert [v['request']for v in c['resources']]==[[0,0,1048576,8192,4],[0x10000000,0,4096,4096,4]]
   assert c['resources'][1]['initial_hex']==bytes(4096).hex()
  else:assert c['boundary']['kind']=='returned_setup' and not c['resources']
  d=c['power_dpc'];assert bool(d['boundary'])==c['automatic_power_down']
  if d['boundary']:assert d['boundary']['target']==0x80000168 and d['boundary']['frame']=='ac002e00'
 g=read(E/'generated-v1.json');assert g==read(E/'generated-repeat.json')
 assert g['generator_sha256']==sha(ROOT/'tools/generate_native_tu15_power_dpc_20261012.py')
 assert g['generated_sha256']==sha(ROOT/'src/native_tu15_power_dpc_generated.cpp')==sha(E/'power-dpc-repeat.cpp')
 assert g['sites']==4 and not g['production_decoder'] and not g['production_original_code_execution']
 builds={};expected={'native_tu15_startup_without_bios','native_tu15_startup_timer','original_runtime_image_tu15'}
 for mode in ('release','sanitizer'):
  b=E/f'build-{mode}';tests=ET.parse(b/'tests.xml').findall('.//testcase');log=(b/'tests.log').read_text()
  assert len(tests)==3 and {t.attrib['name']for t in tests}==expected
  assert all(not t.findall(tag)for t in tests for tag in ('failure','error','skipped'))
  assert '100% tests passed'in log and '48 prefixes, 31656 original instructions'in log and 'real rearm/cancel/expiry/DPC/retirement pass'in log
  for row in read(b/'compile_commands.json'):
   path=Path(row['file'])
   if path==b/'registration.cpp':assert path.read_text()=='// Actual CMake consumer\n'
   else:assert path.is_relative_to(source) and sha(path)==m['files'][str(path.relative_to(source))]
   assert all(flag in row['command']for flag in ('-fno-fast-math','-ffp-contract=off','-frounding-math'))
   if mode=='sanitizer':assert '-fsanitize=address,undefined'in row['command']
  builds[mode]=dict(tests=3,status='passed',binary_sha256=sha(b/'native_tu15_startup_timer_tests'),results_sha256=sha(b/'tests.xml'),compile_commands_sha256=sha(b/'compile_commands.json'))
 trace=(E/'native-resource-trace.log').read_text();assert 'exited with 0'in trace
 assert all(name in trace for name in ('timerfd_create(', 'timerfd_settime(', 'timerfd_gettime(', 'poll('))
 for line in trace.splitlines():
  if 'openat('in line:assert all(name not in line.lower()for name in ('/bios/','complex_4627','.bin"','eeprom','ranges.bin','values.bin','kernel-text')),line
 d=dict(schema=1,validator_sha256=sha(__file__),source_manifest_sha256=sha(E/'source-manifest-v1.json'),original_fixture_sha256=sha(E/'original-v5/fixture.json'),generated_manifest_sha256=sha(E/'generated-v1.json'),
  native_resource_trace_sha256=sha(E/'native-resource-trace.log'),fresh_prefixes=48,returned_setup_calls=24,source_installed_heaps=24,original_instructions=31656,source_stores=13008,
  primary_record_registry_lock_stores=1008,native_configuration_timer_stores=672,native_virtual_resource_calls=48,final_bank_bytes=121536,original_dpc_instructions=144,original_dpc_stores=24,
  real_rearm_cancel_expiry_dpc_and_retirement=True,clear_direction_entry_abi_required=True,bios_runtime_required=False,builds=builds,
  full_startup_return=False,required_post_heap_target='002dba2d',required_shutdown_target='80000168',renderer_and_live_world_still_required=True,live_multiplayer_parity=False)
 out=E/'validation.json';raw=json.dumps(d,sort_keys=True,indent=2)+'\n'
 if out.exists():assert out.read_text()==raw,'Preserve prior seal'
 else:out.write_text(raw)
 print('PASS native TU1.5 timer and source heap startup '+sha(out))
if __name__=='__main__':main()
