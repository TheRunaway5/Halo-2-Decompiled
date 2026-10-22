#!/usr/bin/env python3
"""Seal and audit the TU1.5-only vehicle constraint cleanup and force composition."""
import argparse,ast,hashlib,json,re,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/vehicle-constraint-cleanup-tu15-20261011'
P=ROOT/'analysis/multiplayer/sandbox-source/vehicle-contact-state-tu15-20261011'
MATH=ROOT/'analysis/multiplayer/sandbox-source/vehicle-motion-math-20261011'
PARENT_PIN='6485e77cf383e2e562e5a07328d1d900d688ab1ec6652e28ddb8f50ffa888dc6'
VALIDATION_PIN='b6c71f4725c803f9e47f4a39b7627699baf6a34327a722f8908bdb4471040430'
OWNED=('include/halo2/physics_vehicle_constraint_cleanup.hpp','src/physics/vehicles/physics_vehicle_constraint_cleanup_tu15_generated.cpp',
 'cmake/PhysicsVehicleConstraintCleanup.cmake','tests/physics_vehicle_constraint_cleanup_tests.cpp',
 'tools/research_vehicle_constraint_cleanup_tu15_20261011.py','tools/generate_vehicle_constraint_cleanup_tu15_20261011.py',
 'tools/oracle_vehicle_constraint_cleanup_tu15_20261011.py','tools/validate_vehicle_constraint_cleanup_tu15_20261011.py',
 'docs/PHYSICS_VEHICLE_CONSTRAINT_CLEANUP_TU15_20261011.md')
REGISTRATION='include(cmake/PhysicsVehicleConstraintCleanup.cmake)'
PARTS=('runtime','update','motion','motion_math','contact_state')
TOP='''cmake_minimum_required(VERSION 3.20)
project(halo2_tu15_vehicle_constraint_cleanup LANGUAGES CXX)
include(CTest)
option(HALO2_SANITIZERS "Instrument the complete component" OFF)
set(HALO2_REUSE_PARENT_BUILD "" CACHE PATH "Unchanged validated contact archives, or empty to compile all sources")
set(HALO2_REUSE_MATH_BUILD "" CACHE PATH "Unchanged validated motion/math archives")
find_package(nlohmann_json CONFIG REQUIRED)
if(HALO2_REUSE_PARENT_BUILD)
  if(NOT HALO2_REUSE_MATH_BUILD)
    message(FATAL_ERROR "Parent archive reuse requires the validated motion/math build")
  endif()
  foreach(part IN ITEMS runtime update motion motion_math contact_state)
    add_library(halo2_physics_vehicle_${part} STATIC IMPORTED GLOBAL)
    if(part STREQUAL "contact_state")
      set(archive_root "${HALO2_REUSE_PARENT_BUILD}")
    else()
      set(archive_root "${HALO2_REUSE_MATH_BUILD}")
    endif()
    set_target_properties(halo2_physics_vehicle_${part} PROPERTIES
      IMPORTED_LOCATION "${archive_root}/libhalo2_physics_vehicle_${part}.a"
      INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include")
  endforeach()
  set_target_properties(halo2_physics_vehicle_update PROPERTIES INTERFACE_LINK_LIBRARIES halo2_physics_vehicle_runtime)
  set_target_properties(halo2_physics_vehicle_motion PROPERTIES INTERFACE_LINK_LIBRARIES halo2_physics_vehicle_update)
  set_target_properties(halo2_physics_vehicle_motion_math PROPERTIES INTERFACE_LINK_LIBRARIES halo2_physics_vehicle_motion)
  set_target_properties(halo2_physics_vehicle_contact_state PROPERTIES INTERFACE_LINK_LIBRARIES halo2_physics_vehicle_motion_math)
else()
  include(cmake/PhysicsVehicleRuntime.cmake)
  include(cmake/PhysicsVehicleUpdate.cmake)
  include(cmake/PhysicsVehicleMotion.cmake)
  include(cmake/PhysicsVehicleMotionMath.cmake)
  include(cmake/PhysicsVehicleContactState.cmake)
endif()
include(cmake/PhysicsVehicleConstraintCleanup.cmake)
'''
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read(p):return json.loads(Path(p).read_text())
def parent():
 assert sha(P/'source-manifest-v1.json')==PARENT_PIN and sha(P/'validation.json')==VALIDATION_PIN
 m=read(P/'source-manifest-v1.json');assert len(m['files'])==90
 for p,pin in m['files'].items():assert sha(ROOT/p)==pin==sha(P/'source-v1'/p),p
 return m
def dependencies():
 files=set(parent()['files'])|set(OWNED);pending=list(files);seen=set()
 while pending:
  name=pending.pop()
  if name in seen:continue
  seen.add(name);path=ROOT/name;found=[]
  if path.suffix in ('.cpp','.hpp'):
   for inc in re.findall(r'^#include "([^"]+)"',path.read_text(),re.M):
    matches=[q.resolve() for q in (path.parent/inc,ROOT/'include'/inc,ROOT/'src'/inc) if q.is_file()]
    assert matches,(path,inc);found.append(matches[0])
  elif path.suffix=='.py':
   for n in ast.walk(ast.parse(path.read_text())):
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
  destination=target/name;destination.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,destination)
  assert sha(destination)==pin==sha(ROOT/name)
 (target/'CMakeLists.txt').write_text(TOP);(target/'analysis').symlink_to(ROOT/'analysis',target_is_directory=True)
 manifest=dict(schema=1,parity_target='English TU1.5 only',files=files,owned=OWNED,
  parent_source_manifest_sha256=PARENT_PIN,parent_validation_sha256=VALIDATION_PIN,
  standalone_top_sha256=sha(target/'CMakeLists.txt'),registration=REGISTRATION,parent_sources_changed=False)
 (E/'source-manifest-v1.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(files=len(files),source_manifest_sha256=sha(E/'source-manifest-v1.json'))))
def validate():
 parent();m=read(E/'source-manifest-v1.json');assert sorted(m['files'])==dependencies() and m['owned']==list(OWNED)
 assert m['parent_source_manifest_sha256']==PARENT_PIN and m['parent_validation_sha256']==VALIDATION_PIN
 for p,pin in m['files'].items():assert sha(ROOT/p)==pin==sha(E/'source-v1'/p),p
 assert (E/'source-v1/CMakeLists.txt').read_text()==TOP and sha(E/'source-v1/CMakeLists.txt')==m['standalone_top_sha256']
 assert REGISTRATION in (ROOT/'CMakeLists.txt').read_text()
 for stage in ('research','generated','original','hardware'):
  assert sha(E/(stage+'-verified')/('manifest.json' if stage=='generated' else 'report.json'))==sha(E/(stage+'-repeat')/('manifest.json' if stage=='generated' else 'report.json')),stage
 r=read(E/'research-verified/report.json');g=read(E/'generated-verified/manifest.json');o=read(E/'original-verified/report.json')
 assert r['extension_tool_sha256']==sha(ROOT/'tools/research_vehicle_constraint_cleanup_tu15_20261011.py')
 assert r['complete_bodies']==120 and len(r['instruction_map'])==13781 and len(r['additional_native_entries'])==21
 assert sum(v['instructions'] for v in r['tu15_new_owners'])==2565 and r['reused_parent_targets']==[0x1d8c80]
 assert not r['new_recovery_requires_older_image'] and r['parity_target']=='English TU1.5 only'
 from oracle_smg_firing_1_1 import Xbe,_Code
 from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
 assert sha(NEW_DEFAULT)==NEW_PIN==r['tu15_owned_xbe_sha256'];image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
 for row in r['tu15_new_owners']:
  for mode in ('verified','repeat'):
   listing=E/('research-'+mode)/row['listing_file'];assert sha(listing)==row['listing_sha256']
   for line in listing.read_text().splitlines():
    address,raw,mnemonic,operand=line.split('\t');i=code.at(int(address,16))
    assert i.bytes.hex()==raw and i.mnemonic==mnemonic and i.op_str==operand
 assert g['generator_sha256']==sha(ROOT/'tools/generate_vehicle_constraint_cleanup_tu15_20261011.py') and g['research_sha256']==sha(E/'research-verified/report.json')
 assert g['generated_cpp_sha256']==sha(ROOT/'src/physics/vehicles/physics_vehicle_constraint_cleanup_tu15_generated.cpp')==sha(E/'generated-repeat/physics_vehicle_constraint_cleanup_tu15_generated.cpp')
 assert len(g['lowered_instructions'])==2565 and not g['production_opcode_decoder'] and not g['production_original_code_execution']
 assert o['tool_sha256']==sha(ROOT/'tools/oracle_vehicle_constraint_cleanup_tu15_20261011.py') and o['cases']==15141 and o['new_owner_covered_instructions']==1351
 for name in ('u5.json',):assert sha(E/'original-verified'/name)==sha(E/'original-repeat'/name)==o['fixture_sha256']
 for name in ('u5.json','u5-native.json'):assert sha(E/'hardware-verified'/name)==sha(E/'hardware-repeat'/name),name
 h=read(E/'hardware-verified/u5.json');f=read(E/'hardware-verified/u5-native.json');inputs=read(E/'original-verified/u5.json')
 assert h['tool_sha256']==sha(ROOT/'tools/oracle_vehicle_motion_math_hardware_20261011.py') and h['trace_observer_sha256']==sha(ROOT/'tools/vehicle_motion_math_hardware_trace_20261011.cpp')
 assert h['original_sha256']==NEW_PIN and h['input_fixture_sha256']==sha(E/'original-verified/u5.json')
 assert not h['paired_instruction_bytes_changed'] and not h['original_code_used_in_production']
 assert f['image']=='1.5' and f['oracle_backend']=='original_i386_hardware_single_step'
 assert len(f['cases'])==15141 and sum(c['complete'] for c in f['cases'])==12689
 seen=set();lookup={c['id']:c for c in h['cases']};staged=[]
 assert len(inputs['cases'])==len(f['cases'])==len(h['cases'])
 for c,initial in zip(f['cases'],inputs['cases']):
  hardware=lookup[c['id']];assert c['id']==initial['id']
  for name in ('entry_abi','initial_banks','subsequent_call'):assert c[name]==initial[name]
  for name in ('exit','final_banks','instructions'):assert c[name]==hardware[name]
  assert [{k:v for k,v in w.items() if k!='kind'} for w in c['writes']]==hardware['writes'];seen.update(c['instructions'])
  if c['subsequent_call']:staged.append(c)
 assert all(c['x87_defined_status_mask']==initial['x87_defined_status_mask'] for c,initial in zip(f['cases'],inputs['cases']))
 assert not f['native_expected_data_used'] and not f['instruction_patches'] and not f['live_vehicle_driving_proof']
 assert len(seen)==8702 and len(staged)==432 and all(c['complete'] for c in staged)
 new_sites=set(g['lowered_instructions']);assert len(seen & {int(p,16) for p in new_sites})==1351
 for entry in (0xde8c0,0x16e6a0,0x17dec0,0x18bbc0,0x18c440,0x18d4b0,0x18d510,0x22e210,0x22fe30,0x230ae0):
  row=next(v for v in r['tu15_new_owners'] if v['entry']==entry)
  sites={int(line.split('\t')[0],16) for line in (E/'research-verified'/row['listing_file']).read_text().splitlines()}
  assert sites<=seen,hex(entry)
 assert len(m['files'])==99
 for definition in f['definitions']:
  path=ROOT/definition['map'];assert sha(path)==definition['map_sha256']
  for record,key in ((definition,'prefix_hex'),*[(t,'raw_hex') for t in definition['dependent_tags']]):
   raw=bytes.fromhex(record[key])
   with path.open('rb') as stream:stream.seek(record['file_offset']);assert stream.read(len(raw))==raw
 logs={};binaries={};archives={};parent_validation=read(P/'validation.json')
 expected=f"PASS original TU1.5 vehicle constraint cleanup: 15141 cases, {sum(len(c['instructions']) for c in f['cases'])} instructions, {sum(len(c['writes']) for c in f['cases'])} ordered writes, {sum(len(bytes.fromhex(b['hex'])) for c in f['cases'] for b in c['final_banks'])} full-state bytes"
 for mode in ('release','sanitizer','production'):
  for stage in ('configure','build','test'):logs[mode+'-'+stage]=sha(E/(mode+'-'+stage+'.log'))
  output=(E/(mode+'-test.log')).read_text();assert output.count(expected)==1 and '100% tests passed' in output and 'Skipped' not in output
  assert re.search(r'original_physics_vehicle_constraint_cleanup_tu15\s+\.+\s+Passed',output)
  directory=ROOT/'build-vehicle-control-20261011' if mode=='production' else E/('build-'+mode)
  binaries[mode]=sha(directory/'physics_vehicle_constraint_cleanup_tests');ninja=(directory/'build.ninja').read_text()
  block=ninja.split('CMakeFiles/halo2_physics_vehicle_constraint_cleanup.dir/src/physics_vehicle_constraint_cleanup_tu15_generated.cpp.o:')[1].split('\n\n')[0]
  for flag in ('-fno-fast-math','-ffp-contract=off','-frounding-math'):assert flag in block
  if mode=='sanitizer':assert '-fsanitize=address,undefined' in block
  if mode!='production':
   parent_build=P/('build-'+mode);cache=(directory/'CMakeCache.txt').read_text()
   assert f'HALO2_REUSE_PARENT_BUILD:PATH={parent_build}' in cache
   assert sha(parent_build/'physics_vehicle_contact_state_tests')==parent_validation['binaries'][mode]
   math_build=MATH/('build-'+mode)
   assert f'HALO2_REUSE_MATH_BUILD:PATH={math_build}' in cache
   assert sha(math_build/'physics_vehicle_motion_math_tests')==read(MATH/'validation.json')['binaries'][mode]
   archives[mode]={part:sha((parent_build if part=='contact_state' else math_build)/f'libhalo2_physics_vehicle_{part}.a') for part in PARTS}
   assert {part:archives[mode][part] for part in PARTS if part!='contact_state'}==parent_validation['reused_parent_archives'][mode]
  else:
   for part in PARTS:
    suffix='_tu15' if part=='contact_state' else '_u5'
    assert re.search('original_physics_vehicle_'+part+suffix+r'\s+\.+\s+Passed',output)
 result=dict(schema=1,parity_target='English TU1.5 only',source_manifest_sha256=sha(E/'source-manifest-v1.json'),
  parent_validation_sha256=VALIDATION_PIN,validator_sha256=sha(__file__),research_sha256=sha(E/'research-verified/report.json'),
  generator_manifest_sha256=sha(E/'generated-verified/manifest.json'),hardware_fixture_sha256=sha(E/'hardware-verified/u5.json'),
  native_fixture_sha256=sha(E/'hardware-verified/u5-native.json'),native_cases=15141,complete_cases=12689,
  required_owner_prefixes=2452,compiled_bodies=120,compiled_sites=13781,new_bodies=21,new_sites=2565,exercised_new_sites=1351,
  exercised_sites=8702,staged_motion_cases=432,all_staged_motion_cases_return=True,
  observer_events=sum(len(c['instructions']) for c in f['cases']),hardware_steps=h['single_stepped_original_instructions'],
  ordered_writes=sum(len(c['writes']) for c in f['cases']),retained_bytes=sum(len(bytes.fromhex(b['hex'])) for c in f['cases'] for b in c['final_banks']),
  binaries=binaries,reused_parent_archives=archives,logs=logs,parent_sources_changed=False,
  tu15_release_sanitizer_production_passed=True,original_code_used_in_production=False,
  required_owner_results_supplied=False,full_physics_parity=False,
  body_coverage=o['body_coverage'],new_cleanup_case_kinds=o['new_cleanup_case_kinds'],defined_x87_c1_preserved=True)
 (E/'validation.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(cases=15141,validation_sha256=sha(E/'validation.json'))))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--freeze',action='store_true');a=p.parse_args();freeze() if a.freeze else validate()
