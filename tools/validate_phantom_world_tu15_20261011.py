#!/usr/bin/env python3
"""Seal the TU1.5 phantom/world activation source and independent comparisons."""
import argparse,ast,hashlib,json,re,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/phantom-world-tu15-20261011'
P=ROOT/'analysis/multiplayer/sandbox-source/phantom-force-tu15-20261011'
W=ROOT/'analysis/multiplayer/integration/physics-engine-20261010'
PARENT_MANIFEST='ebe8596ee6e5306b2dbc60de8012a7edfcf903596a3c9ee58b4b15beddd56f9d'
PARENT_VALIDATION='9b106d2991f0b2521668c03b360440c731cc869848a13b83c1cc41f29c831a2b'
WORLD_MANIFEST='3a6db1913e6d881b8d81c2852643e7541ee8d7bd042ff808d71e725bea0d5ae0'
OWNED=('include/halo2/physics_phantom_world_tu15.hpp','src/physics_phantom_world_tu15_generated.cpp',
 'tests/physics_phantom_world_tu15_tests.cpp','cmake/PhysicsPhantomWorldTu15.cmake',
 'tools/research_phantom_world_tu15_20261011.py','tools/generate_phantom_world_tu15_20261011.py',
 'tools/oracle_phantom_world_tu15_20261011.py','tools/oracle_phantom_world_hardware_tu15_20261011.py',
 'tools/validate_phantom_world_tu15_20261011.py','docs/PHYSICS_PHANTOM_WORLD_TU15_20261011.md')
REGISTRATION='include(cmake/PhysicsPhantomWorldTu15.cmake)'
TOP='''cmake_minimum_required(VERSION 3.20)
project(halo2_phantom_world_tu15 LANGUAGES CXX)
include(CTest)
option(HALO2_SANITIZERS "Instrument the complete native graph" OFF)
find_package(nlohmann_json CONFIG REQUIRED)
# World registration expects this consumer. No replacement physics is supplied.
add_library(halo2_systems INTERFACE)
include(cmake/PhysicsVehicleRuntime.cmake)
include(cmake/PhysicsVehicleUpdate.cmake)
include(cmake/PhysicsVehicleMotion.cmake)
include(cmake/PhysicsVehicleMotionMath.cmake)
include(cmake/PhysicsVehicleContactState.cmake)
include(cmake/PhysicsVehicleConstraintCleanup.cmake)
include(cmake/PhysicsVehicleEnvironmentState.cmake)
include(cmake/PhysicsPhantomForceTu15.cmake)
include(cmake/PhysicsWorldRuntime.cmake)
include(cmake/PhysicsPhantomWorldTu15.cmake)
'''
# A static consumer is needed because the existing CMake registration uses PUBLIC.
TOP=TOP.replace('add_library(halo2_systems INTERFACE)','file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/registration.cpp "// CMake registration consumer\\n")\nadd_library(halo2_systems STATIC ${CMAKE_CURRENT_BINARY_DIR}/registration.cpp)')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read(p):return json.loads(Path(p).read_text())
def dependencies():
 assert sha(P/'source-manifest-v1.json')==PARENT_MANIFEST and sha(P/'validation.json')==PARENT_VALIDATION
 assert sha(W/'source-manifest-v18.json')==WORLD_MANIFEST
 parent=read(P/'source-manifest-v1.json');world=read(W/'source-manifest-v18.json')
 for p,pin in parent['files'].items():assert sha(ROOT/p)==pin==sha(P/'source-v1'/p),p
 extras=('include/halo2/physics_world_runtime.hpp','src/physics_world_runtime_generated.cpp','src/physics_world_runtime_internal.hpp','cmake/PhysicsWorldRuntime.cmake','tests/physics_world_runtime_tests.cpp')
 for p in extras:assert sha(ROOT/p)==world['files'][p]==sha(W/'source-v18'/p),p
 files=set(parent['files'])|set(OWNED)|set(extras);pending=list(files);seen=set()
 while pending:
  name=pending.pop()
  if name in seen:continue
  seen.add(name);p=ROOT/name;found=[]
  if p.suffix in ('.cpp','.hpp'):
   for inc in re.findall(r'^#include "([^"]+)"',p.read_text(),re.M):
    matches=[q.resolve() for q in (p.parent/inc,ROOT/'include'/inc,ROOT/'src'/inc) if q.is_file()];assert matches,(p,inc);found.append(matches[0])
  elif p.suffix=='.py':
   for n in ast.walk(ast.parse(p.read_text())):
    modules=[n.module] if isinstance(n,ast.ImportFrom) and n.module else [v.name for v in n.names] if isinstance(n,ast.Import) else []
    for module in modules:
     matches=[q for q in (ROOT/'tools'/(module.replace('.','/')+'.py'),ROOT/'tools/reference/halo2-decompiled/tools'/(module.replace('.','/')+'.py')) if q.is_file()]
     if matches:found.append(matches[0])
  for q in found:
   relative=str(q.relative_to(ROOT));files.add(relative)
   if relative not in seen:pending.append(relative)
 return sorted(files)
def freeze():
 assert REGISTRATION in (ROOT/'CMakeLists.txt').read_text()
 target=E/'source-v5';target.mkdir();files={p:sha(ROOT/p) for p in dependencies()}
 for name,pin in files.items():
  q=target/name;q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,q);assert sha(q)==pin==sha(ROOT/name)
 (target/'CMakeLists.txt').write_text(TOP);(target/'analysis').symlink_to(ROOT/'analysis',target_is_directory=True)
 result=dict(schema=1,parity_target='English TU1.5 only',files=files,owned=OWNED,top_sha256=sha(target/'CMakeLists.txt'),
  parent_source_manifest_sha256=PARENT_MANIFEST,parent_validation_sha256=PARENT_VALIDATION,world_manifest_sha256=WORLD_MANIFEST,
  registration=REGISTRATION,parent_sources_changed=False,parent_archives_reused=False)
 (E/'source-manifest-v5.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(source_files=len(files),source_manifest_sha256=sha(E/'source-manifest-v5.json'))))
def validate():
 m=read(E/'source-manifest-v5.json');assert sorted(m['files'])==dependencies() and m['owned']==list(OWNED)
 for p,pin in m['files'].items():assert sha(ROOT/p)==pin==sha(E/'source-v5'/p),p
 assert (E/'source-v5/CMakeLists.txt').read_text()==TOP and sha(E/'source-v5/CMakeLists.txt')==m['top_sha256']
 assert REGISTRATION in (ROOT/'CMakeLists.txt').read_text()
 for stage,name in (('research','report.json'),('generated','manifest.json'),('original','report.json'),('original','u5.json'),('hardware','report.json'),('hardware','u5.json'),('hardware','u5-native.json')):
  assert sha(E/(stage+'-verified')/name)==sha(E/(stage+'-repeat')/name),(stage,name)
 r=read(E/'research-verified/report.json');g=read(E/'generated-verified/manifest.json');o=read(E/'original-verified/report.json');f=read(E/'hardware-verified/u5-native.json')
 assert r['extension_tool_sha256']==sha(ROOT/OWNED[4]) and g['generator_sha256']==sha(ROOT/OWNED[5]) and f['tool_sha256']==sha(ROOT/OWNED[6])
 assert f['hardware_wrapper_sha256']==sha(ROOT/OWNED[7]) and f['hardware_tool_sha256']==sha(ROOT/'tools/oracle_vehicle_motion_math_hardware_20261011.py')
 assert g['generated_cpp_sha256']==sha(ROOT/OWNED[1])==sha(E/'generated-repeat/native.cpp')
 assert o['fixture_sha256']==sha(E/'original-verified/u5.json') and f['correspondence_sha256']==sha(E/'research-verified/report.json')
 assert f['image']=='1.5' and len(f['cases'])==671 and o['complete']==521 and o['required_owner_prefixes']==150
 assert r['complete_bodies']==609 and len(r['instruction_map'])==51007 and r['new_native_bodies']==5 and len(r['new_sites'])==477
 assert sum(o['body_coverage'].values())==249 and f['single_stepped_original_instructions']==233894
 counts=dict(instructions=sum(len(c['instructions']) for c in f['cases']),writes=sum(len(c['writes']) for c in f['cases']),bytes=sum(len(bytes.fromhex(b['hex'])) for c in f['cases'] for b in c['final_banks']))
 assert counts==dict(instructions=234090,writes=46208,bytes=191303),counts
 from oracle_smg_firing_1_1 import Xbe,_Code
 from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
 assert sha(NEW_DEFAULT)==NEW_PIN==r['original_sha256'];image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
 listing=E/'research-verified'/r['listing_file'];assert sha(listing)==r['listing_sha256']
 for line in listing.read_text().splitlines():
  p,raw,mn,operand=line.split('\t');i=code.at(int(p,16));assert (i.bytes.hex(),i.mnemonic,i.op_str)==(raw,mn,operand)
 for b in r['virtual_bindings']:
  assert image.read(b['table']+b['offset'],4).hex()==b['slot_raw_hex'] and code.at(b['store']).bytes.hex()==b['store_raw_hex']
 assert not f['native_expected_data_used'] and f['instruction_patches']==[] and not f['paired_instruction_bytes_changed']
 assert {c['exit_entry'] for c in f['cases'] if not c['complete']}=={0x37ce0c}
 logs={};binaries={};expected='PASS original TU1.5 phantom/world activation state: 671 cases, 234090 instructions, 46208 ordered writes, 191303 full-state bytes'
 for mode in ('release','sanitizer'):
  for stage in ('configure','build','test'):logs[mode+'-'+stage]=sha(E/(mode+'-'+stage+'.log'))
  output=(E/(mode+'-test.log')).read_text();assert output.count(expected)==1 and '100% tests passed' in output and 'Skipped' not in output
  assert len(re.findall(r'Test\s+#\d+:.*?Passed',output))==9
  directory=E/('build-'+mode+'-v5');binaries[mode]=sha(directory/'physics_phantom_world_tu15_tests')
  for name in ('physics_vehicle_runtime_generated.cpp','physics_phantom_force_tu15_generated.cpp','physics_world_runtime_generated.cpp','physics_phantom_world_tu15_generated.cpp'):
   section=(directory/'build.ninja').read_text().split('/'+name+'.o:')[1].split('\n\n')[0]
   for flag in ('-fno-fast-math','-ffp-contract=off','-frounding-math'):assert flag in section
   if mode=='sanitizer':assert '-fsanitize=address,undefined' in section
 result=dict(schema=1,parity_target='English TU1.5 only',validator_sha256=sha(__file__),source_manifest_sha256=sha(E/'source-manifest-v5.json'),
  parent_validation_sha256=PARENT_VALIDATION,world_manifest_sha256=WORLD_MANIFEST,source_files=len(m['files']),fixture_sha256=sha(E/'hardware-verified/u5-native.json'),
  new_bodies=5,new_sites=477,new_sites_exercised=249,complete_graph_bodies=609,complete_graph_sites=51007,cases=671,complete=521,required_owner_prefixes=150,
  hardware_instructions=233894,normalized_instruction_observations=counts['instructions'],ordered_writes=counts['writes'],retained_bytes=counts['bytes'],
  repeated_recovery_generation_original_hardware_identical=True,release_sanitizer_passed=True,parent_sources_changed=False,parent_archives_reused=False,
  original_code_used_in_production=False,older_image_executed=False,raw_body_world_island_listener_inputs=True,source_factory_proof=False,
  live_collision_proof=False,live_physics_parity=False,required_owners=['37CE0C CRT power','unexercised pair agent/constraint builders','full TU1.5 startup/factory/contact producer'],logs=logs,binaries=binaries)
 (E/'validation.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(validation_sha256=sha(E/'validation.json'))))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--freeze',action='store_true');a=p.parse_args();freeze() if a.freeze else validate()
