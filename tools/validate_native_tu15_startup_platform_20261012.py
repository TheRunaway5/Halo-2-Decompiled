#!/usr/bin/env python3
"""Validate original caller behavior with the BIOS-free native boot contract."""
import hashlib,json,sys,xml.etree.ElementTree as ET
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
E=ROOT/'analysis/multiplayer/sandbox-source/native-tu15-startup-platform-20261012'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read(p):return json.loads(Path(p).read_text())
def main():
 m=read(E/'source-manifest-v1.json');source=E/'source-v1'
 for name,pin in m['files'].items():assert sha(source/name)==pin==sha(ROOT/name),name
 assert sha(source/'CMakeLists.txt')==m['top_sha256'] and sha(E/'dependencies.d')==m['dependencies_sha256']
 parent=E.parent/'tu15-timer-startup-20261012/source-manifest-v1.json'
 assert sha(parent)==m['parent_manifest_sha256']
 p=read(parent)
 for name in m['archive_compiler_inputs']:assert sha(ROOT/name)==p['files'][name]
 for mode,row in m['archives'].items():assert sha(source/row['file'])==row['sha256']==p['archives'][mode]['sha256']
 f=read(E/'original-v1/fixture.json');assert f==read(E/'original-repeat/fixture.json')
 assert f['tool_sha256']==sha(ROOT/'tools/oracle_native_tu15_startup_platform_20261012.py')
 assert f['helper_tool_sha256']==sha(ROOT/'tools/oracle_tu15_startup_records_20261011.py')
 assert f['original_sha256']==sha(NEW_DEFAULT)==NEW_PIN and len(f['source_instructions'])==182
 xbe=Xbe(NEW_DEFAULT)
 for pc,row in f['source_instructions'].items():assert xbe.read(int(pc,16),len(bytes.fromhex(row['raw'])))==bytes.fromhex(row['raw'])
 env=f['native_environment'];assert env['hardware_address']==0x90000000 and env['hardware_hex']=='0000000000000000'
 assert not env['desktop_arcade'] and env['consumed_hardware_mask']==8
 assert not env['bios_input_used'] and not env['eeprom_input_used'] and not env['kernel_tables_used']
 assert not f['source_instruction_patches'] and not f['older_image_executed'] and not f['captured_world_input_used']
 assert not f['timer_armed'] and not f['process_heap_installed'] and not f['full_startup_complete'] and not f['live_physics_parity']
 cases=f['cases'];assert len(cases)==48 and sum(c['automatic_power_down']for c in cases)==24
 assert sum(len(c['instructions'])for c in cases)==2832 and sum(len(c['stores'])for c in cases)==1800
 assert sum(len(v['stores'])for c in cases for v in c['platform_calls'])==816
 assert sum(len(v['stores'])for c in cases for v in c['native_calls'])==144
 for c in cases:
  assert [v['target']for v in c['native_calls']]==[0x80000018,0x8000012d]
  assert c['boundary']['target']==0x80000095 and c['boundary']['width']==20
  assert c['boundary']['instruction']==0x2e0096
  assert c['native_calls'][1]['before']['gp'][0]==c['native_calls'][1]['after']['gp'][0]==0
 # The only native status domain is independently proved by the unchanged
 # named reference converter. Its resource stays validation-only.
 reference=E.parent/'tu15-status-startup-20261012/original-v1/fixture.json';r=read(reference)
 assert (0,0) in {(c['kernel_calls'][0]['before']['gp'][0],c['kernel_calls'][0]['after']['gp'][0])for c in r['cases']}
 builds={};expected={'native_tu15_startup_without_bios','original_runtime_image_tu15'}
 for mode in ('release','sanitizer'):
  b=E/f'build-{mode}';tests=ET.parse(b/'tests.xml').findall('.//testcase');log=(b/'tests.log').read_text()
  assert len(tests)==2 and {t.attrib['name']for t in tests}==expected
  assert all(not t.findall(tag)for t in tests for tag in ('failure','error','skipped'))
  assert '100% tests passed'in log and '48 fresh prefixes, 2832 original instructions'in log
  commands=read(b/'compile_commands.json')
  for row in commands:
   path=Path(row['file'])
   if path==b/'registration.cpp':assert path.read_text()=='// Actual CMake consumer\n'
   else:assert path.is_relative_to(source) and sha(path)==m['files'][str(path.relative_to(source))]
   assert all(flag in row['command']for flag in ('-fno-fast-math','-ffp-contract=off','-frounding-math'))
   if mode=='sanitizer':assert '-fsanitize=address,undefined'in row['command']
  builds[mode]=dict(tests=2,status='passed',binary_sha256=sha(b/'native_tu15_startup_platform_tests'),results_sha256=sha(b/'tests.xml'),compile_commands_sha256=sha(b/'compile_commands.json'))
 trace=(E/'file-access-release.log').read_text()
 assert 'exited with 0'in trace
 for line in trace.splitlines():
  if 'openat('in line:assert all(name not in line.lower()for name in ('/bios/','complex_4627','.bin"','eeprom','ranges.bin','values.bin','kernel-text')),line
 d=dict(schema=1,validator_sha256=sha(__file__),source_manifest_sha256=sha(E/'source-manifest-v1.json'),original_fixture_sha256=sha(E/'original-v1/fixture.json'),
  reference_status_fixture_sha256=sha(reference),file_access_trace_sha256=sha(E/'file-access-release.log'),
  fresh_prefixes=48,original_instructions=2832,source_stores=1800,primary_record_registry_stores=816,native_configuration_writes=144,
  native_abi_declared=True,bios_runtime_required=False,eeprom_runtime_required=False,kernel_table_runtime_required=False,builds=builds,
  timer_owner_still_required=True,heap_world_integration_complete=False,live_physics_parity=False,physical_xbox_kernel_abi_proven=False)
 out=E/'validation.json';raw=json.dumps(d,sort_keys=True,indent=2)+'\n'
 if out.exists():assert out.read_text()==raw,'Preserve prior seal'
 else:out.write_text(raw)
 print('PASS native TU1.5 startup without BIOS '+sha(out))
if __name__=='__main__':main()
