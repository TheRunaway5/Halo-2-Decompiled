#!/usr/bin/env python3
"""Seal TU1.5-only phantom force source and independent original comparisons."""
import argparse,ast,hashlib,json,re,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/phantom-force-tu15-20261011'
P=ROOT/'analysis/multiplayer/sandbox-source/vehicle-environment-state-tu15-20261011'
PARENT_MANIFEST='7808ff880faf972e553ac807afc5c01a68a1c98da3a693e9c2b874ae7d73b1b5'
PARENT_VALIDATION='239bb07c92de97930be7ccb27400982621915e21e1377e8165fd1c48c8a8c50a'
OWNED=('include/halo2/physics_phantom_force_tu15.hpp','src/physics_phantom_force_tu15_generated.cpp',
 'tests/physics_phantom_force_tu15_tests.cpp','cmake/PhysicsPhantomForceTu15.cmake',
 'tools/research_phantom_force_tu15_20261011.py','tools/generate_phantom_force_tu15_20261011.py',
 'tools/oracle_phantom_force_tu15_20261011.py','tools/validate_phantom_force_tu15_20261011.py',
 'docs/PHYSICS_PHANTOM_FORCE_TU15_20261011.md','docs/PHYSICS_TU15_OWNER_SCOPE_CORRECTION_20261011.md')
REGISTRATION='include(cmake/PhysicsPhantomForceTu15.cmake)'
TOP='''cmake_minimum_required(VERSION 3.20)
project(halo2_phantom_force_tu15 LANGUAGES CXX)
include(CTest)
option(HALO2_SANITIZERS "Instrument the complete native graph" OFF)
find_package(nlohmann_json CONFIG REQUIRED)
include(cmake/PhysicsVehicleRuntime.cmake)
include(cmake/PhysicsVehicleUpdate.cmake)
include(cmake/PhysicsVehicleMotion.cmake)
include(cmake/PhysicsVehicleMotionMath.cmake)
include(cmake/PhysicsVehicleContactState.cmake)
include(cmake/PhysicsVehicleConstraintCleanup.cmake)
include(cmake/PhysicsVehicleEnvironmentState.cmake)
include(cmake/PhysicsPhantomForceTu15.cmake)
'''
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read(p):return json.loads(Path(p).read_text())
def dependencies():
 assert sha(P/'source-manifest-v3.json')==PARENT_MANIFEST and sha(P/'validation.json')==PARENT_VALIDATION
 parent=read(P/'source-manifest-v3.json')
 for p,pin in parent['files'].items():assert sha(ROOT/p)==pin==sha(P/'source-v3'/p),p
 files=set(parent['files'])|set(OWNED);pending=list(files);seen=set()
 while pending:
  name=pending.pop()
  if name in seen:continue
  seen.add(name);p=ROOT/name;found=[]
  if p.suffix in ('.cpp','.hpp'):
   for inc in re.findall(r'^#include "([^"]+)"',p.read_text(),re.M):
    matches=[q.resolve() for q in (p.parent/inc,ROOT/'include'/inc,ROOT/'src'/inc) if q.is_file()]
    assert matches,(p,inc);found.append(matches[0])
  elif p.suffix=='.py':
   for n in ast.walk(ast.parse(p.read_text())):
    modules=[n.module] if isinstance(n,ast.ImportFrom) and n.module else [v.name for v in n.names] if isinstance(n,ast.Import) else []
    for module in modules:
     q=ROOT/'tools'/(module.replace('.','/')+'.py')
     if q.is_file():found.append(q)
  for q in found:
   relative=str(q.relative_to(ROOT));files.add(relative)
   if relative not in seen:pending.append(relative)
 return sorted(files)
def freeze():
 assert REGISTRATION in (ROOT/'CMakeLists.txt').read_text()
 target=E/'source-v1';target.mkdir();files={p:sha(ROOT/p) for p in dependencies()}
 for name,pin in files.items():
  q=target/name;q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,q)
  assert sha(q)==pin==sha(ROOT/name)
 (target/'CMakeLists.txt').write_text(TOP);(target/'analysis').symlink_to(ROOT/'analysis',target_is_directory=True)
 result=dict(schema=1,parity_target='English TU1.5 only',files=files,owned=OWNED,top_sha256=sha(target/'CMakeLists.txt'),
  parent_source_manifest_sha256=PARENT_MANIFEST,parent_validation_sha256=PARENT_VALIDATION,registration=REGISTRATION,
  parent_sources_changed=False,parent_archives_reused=False)
 (E/'source-manifest-v1.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(source_files=len(files),source_manifest_sha256=sha(E/'source-manifest-v1.json'))))
def validate():
 m=read(E/'source-manifest-v1.json');assert sorted(m['files'])==dependencies() and m['owned']==list(OWNED)
 for p,pin in m['files'].items():assert sha(ROOT/p)==pin==sha(E/'source-v1'/p),p
 assert (E/'source-v1/CMakeLists.txt').read_text()==TOP and sha(E/'source-v1/CMakeLists.txt')==m['top_sha256']
 assert REGISTRATION in (ROOT/'CMakeLists.txt').read_text()
 for stage,name in (('research','report.json'),('generated','manifest.json'),('original','report.json'),('original','u5.json'),('hardware','report.json'),('hardware','u5.json'),('hardware','u5-native.json')):
  assert sha(E/(stage+'-verified')/name)==sha(E/(stage+'-repeat')/name),(stage,name)
 r=read(E/'research-verified/report.json');g=read(E/'generated-verified/manifest.json');o=read(E/'original-verified/report.json');f=read(E/'hardware-verified/u5-native.json')
 assert r['extension_tool_sha256']==sha(ROOT/OWNED[4]) and g['generator_sha256']==sha(ROOT/OWNED[5]) and f['tool_sha256']==sha(ROOT/OWNED[6])
 assert g['generated_cpp_sha256']==sha(ROOT/OWNED[1])==sha(E/'generated-repeat/native.cpp')
 assert o['fixture_sha256']==sha(E/'original-verified/u5.json') and f['correspondence_sha256']==sha(E/'research-verified/report.json')
 assert f['image']=='1.5' and len(f['cases'])==528 and o['complete']==377 and o['required_owner_prefixes']==151
 assert r['complete_bodies']==158 and len(r['instruction_map'])==16402 and len(g['lowered_instructions'])==1314
 assert sum(o['body_coverage'].values())==1214 and f['single_stepped_original_instructions']==194838
 assert sum(len(c['instructions']) for c in f['cases'])==195014 and sum(len(c['writes']) for c in f['cases'])==39156
 assert sum(len(bytes.fromhex(b['hex'])) for c in f['cases'] for b in c['final_banks'])==153663
 from oracle_smg_firing_1_1 import Xbe,_Code,_trace
 from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
 assert sha(NEW_DEFAULT)==NEW_PIN==g['original_sha256'];image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
 for row in r['tu15_new_owners']:
  listing=E/'research-verified'/row['listing_file'];assert sha(listing)==row['listing_sha256']
  assert len(listing.read_text().splitlines())==row['instructions']
  for line in listing.read_text().splitlines():
   p,raw,mn,operand=line.split('\t');i=code.at(int(p,16));assert (i.bytes.hex(),i.mnemonic,i.op_str)==(raw,mn,operand)
 for row in r['phantom_virtual_bindings']:assert image.read(row['table']+row['offset'],4).hex()==row['raw_hex']
 assert not f['native_expected_data_used'] and f['instruction_patches']==[] and not f['paired_instruction_bytes_changed']
 assert {c['exit_entry'] for c in f['cases'] if not c['complete']}=={0x37ce0c,0x3165d0}
 counts={};logs={};binaries={}
 expected='PASS original TU1.5 phantom force state: 528 cases, 195014 instructions, 39156 ordered writes, 153663 full-state bytes'
 for mode in ('release','sanitizer','production'):
  for stage in ('configure','build','test'):logs[mode+'-'+stage]=sha(E/(mode+'-'+stage+'.log'))
  output=(E/(mode+'-test.log')).read_text();assert output.count(expected)==1 and '100% tests passed' in output and 'Skipped' not in output
  assert re.search(r'original_physics_phantom_force_tu15\s+\.+\s+Passed',output)
  assert len(re.findall(r'Test\s+#\d+:.*?Passed',output))==8
  directory=ROOT/'build-vehicle-control-20261011' if mode=='production' else E/('build-'+mode)
  binaries[mode]=sha(directory/'physics_phantom_force_tu15_tests')
  for name in ('physics_vehicle_runtime_generated.cpp','physics_vehicle_motion_math_generated.cpp','physics_phantom_force_tu15_generated.cpp'):
   section=(directory/'build.ninja').read_text().split('/'+name+'.o:')[1].split('\n\n')[0]
   for flag in ('-fno-fast-math','-ffp-contract=off','-frounding-math'):assert flag in section
   if mode=='sanitizer':assert '-fsanitize=address,undefined' in section
 result=dict(schema=1,parity_target='English TU1.5 only',validator_sha256=sha(__file__),source_manifest_sha256=sha(E/'source-manifest-v1.json'),
  parent_validation_sha256=PARENT_VALIDATION,source_files=len(m['files']),fixture_sha256=sha(E/'hardware-verified/u5-native.json'),
  new_bodies=12,new_sites=1314,new_sites_exercised=1214,complete_graph_bodies=158,complete_graph_sites=16402,
  cases=528,complete=377,required_owner_prefixes=151,hardware_instructions=194838,normalized_instruction_observations=195014,
  ordered_writes=39156,retained_bytes=153663,full_updater_returns=193,motion_delivery_cases=153,
  linear_velocity_changed_cases=81,angular_velocity_changed_cases=15,
  repeated_recovery_generation_original_hardware_identical=True,release_sanitizer_production_passed=True,
  parent_sources_changed=False,parent_archives_reused=False,original_code_used_in_production=False,older_image_executed=False,
  raw_body_motion_membership_inputs=True,source_factory_proof=False,live_collision_proof=False,live_physics_parity=False,
  required_owners=['37CE0C CRT power','3165D0 world activation','actual empty-membership allocator'],logs=logs,binaries=binaries)
 (E/'validation.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(validation_sha256=sha(E/'validation.json'))))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--freeze',action='store_true');a=p.parse_args()
 freeze() if a.freeze else validate()
