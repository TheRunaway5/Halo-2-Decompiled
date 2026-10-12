#!/usr/bin/env python3
"""Seal and audit the TU1.5-only vehicle contact/reference composition."""
import argparse,ast,hashlib,json,re,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/vehicle-contact-state-tu15-20261011'
P=ROOT/'analysis/multiplayer/sandbox-source/vehicle-motion-math-20261011'
PARENT_PIN='d5c0fdf607ca869e865e8975258a656a30b46f1937bb070f971f4e5ce3617e29'
VALIDATION_PIN='57b1b0db72d685185cbefab2ba9ab269acdf4e1f59eddcd68fa22984105441bb'
OWNED=('include/halo2/physics_vehicle_contact_state.hpp','src/physics_vehicle_contact_state_tu15_generated.cpp',
 'cmake/PhysicsVehicleContactState.cmake','tests/physics_vehicle_contact_state_tests.cpp',
 'tools/research_vehicle_contact_state_tu15_20261011.py','tools/generate_vehicle_contact_state_tu15_20261011.py',
 'tools/oracle_vehicle_contact_state_tu15_20261011.py','tools/validate_vehicle_contact_state_tu15_20261011.py',
 'docs/PHYSICS_VEHICLE_CONTACT_STATE_TU15_20261011.md')
REGISTRATION='include(cmake/PhysicsVehicleContactState.cmake)'
PARTS=('runtime','update','motion','motion_math')
TOP='''cmake_minimum_required(VERSION 3.20)
project(halo2_tu15_vehicle_contact_state LANGUAGES CXX)
include(CTest)
option(HALO2_SANITIZERS "Instrument the complete component" OFF)
set(HALO2_REUSE_PARENT_BUILD "" CACHE PATH "Unchanged validated parent archives, or empty to compile all sources")
find_package(nlohmann_json CONFIG REQUIRED)
if(HALO2_REUSE_PARENT_BUILD)
  foreach(part IN ITEMS runtime update motion motion_math)
    add_library(halo2_physics_vehicle_${part} STATIC IMPORTED GLOBAL)
    set_target_properties(halo2_physics_vehicle_${part} PROPERTIES
      IMPORTED_LOCATION "${HALO2_REUSE_PARENT_BUILD}/libhalo2_physics_vehicle_${part}.a"
      INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include")
  endforeach()
  set_target_properties(halo2_physics_vehicle_update PROPERTIES INTERFACE_LINK_LIBRARIES halo2_physics_vehicle_runtime)
  set_target_properties(halo2_physics_vehicle_motion PROPERTIES INTERFACE_LINK_LIBRARIES halo2_physics_vehicle_update)
  set_target_properties(halo2_physics_vehicle_motion_math PROPERTIES INTERFACE_LINK_LIBRARIES halo2_physics_vehicle_motion)
else()
  include(cmake/PhysicsVehicleRuntime.cmake)
  include(cmake/PhysicsVehicleUpdate.cmake)
  include(cmake/PhysicsVehicleMotion.cmake)
  include(cmake/PhysicsVehicleMotionMath.cmake)
endif()
include(cmake/PhysicsVehicleContactState.cmake)
'''
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read(p):return json.loads(Path(p).read_text())
def parent():
 assert sha(P/'source-manifest-v1.json')==PARENT_PIN and sha(P/'validation.json')==VALIDATION_PIN
 m=read(P/'source-manifest-v1.json')
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
 assert r['extension_tool_sha256']==sha(ROOT/'tools/research_vehicle_contact_state_tu15_20261011.py')
 assert r['complete_bodies']==99 and len(r['instruction_map'])==11216 and r['additional_native_entries']==[0x20b3b0,0x20b470,0x22e1b0]
 assert [v['instructions'] for v in r['tu15_new_owners']]==[61,65,34]
 from oracle_smg_firing_1_1 import Xbe,_Code
 from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
 assert sha(NEW_DEFAULT)==NEW_PIN==r['tu15_owned_xbe_sha256'];image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
 for row in r['tu15_new_owners']:
  for mode in ('verified','repeat'):
   listing=E/('research-'+mode)/row['listing_file'];assert sha(listing)==row['listing_sha256']
   for line in listing.read_text().splitlines():
    address,raw,mnemonic,operand=line.split('\t');i=code.at(int(address,16))
    assert i.bytes.hex()==raw and i.mnemonic==mnemonic and i.op_str==operand
 assert g['generator_sha256']==sha(ROOT/'tools/generate_vehicle_contact_state_tu15_20261011.py') and g['research_sha256']==sha(E/'research-verified/report.json')
 assert g['generated_cpp_sha256']==sha(ROOT/'src/physics_vehicle_contact_state_tu15_generated.cpp')==sha(E/'generated-repeat/physics_vehicle_contact_state_tu15_generated.cpp')
 assert len(g['lowered_instructions'])==160 and not g['production_opcode_decoder'] and not g['production_original_code_execution']
 assert o['tool_sha256']==sha(ROOT/'tools/oracle_vehicle_contact_state_tu15_20261011.py') and o['cases']==12518 and o['new_owner_covered_instructions']==151
 for name in ('u5.json',):assert sha(E/'original-verified'/name)==sha(E/'original-repeat'/name)==o['fixture_sha256']
 for name in ('u5.json','u5-native.json'):assert sha(E/'hardware-verified'/name)==sha(E/'hardware-repeat'/name),name
 h=read(E/'hardware-verified/u5.json');f=read(E/'hardware-verified/u5-native.json');inputs=read(E/'original-verified/u5.json')
 assert h['tool_sha256']==sha(ROOT/'tools/oracle_vehicle_motion_math_hardware_20261011.py') and h['trace_observer_sha256']==sha(ROOT/'tools/vehicle_motion_math_hardware_trace_20261011.cpp')
 assert h['original_sha256']==NEW_PIN and h['input_fixture_sha256']==sha(E/'original-verified/u5.json')
 assert not h['paired_instruction_bytes_changed'] and not h['original_code_used_in_production']
 assert f['image']=='1.5' and f['oracle_backend']=='original_i386_hardware_single_step'
 assert len(f['cases'])==12518 and sum(c['complete'] for c in f['cases'])==10086
 seen=set();lookup={c['id']:c for c in h['cases']};staged=[]
 assert len(inputs['cases'])==len(f['cases'])==len(h['cases'])
 for c,initial in zip(f['cases'],inputs['cases']):
  hardware=lookup[c['id']];assert c['id']==initial['id']
  for name in ('entry_abi','initial_banks','subsequent_call'):assert c[name]==initial[name]
  for name in ('exit','final_banks','instructions'):assert c[name]==hardware[name]
  assert [{k:v for k,v in w.items() if k!='kind'} for w in c['writes']]==hardware['writes'];seen.update(c['instructions'])
  if c['subsequent_call']:staged.append(c)
 assert len(seen)==7342 and len(staged)==432 and all(c['complete'] for c in staged)
 new_sites=set(g['lowered_instructions']);assert len(seen & {int(p,16) for p in new_sites})==151
 for definition in f['definitions']:
  path=ROOT/definition['map'];assert sha(path)==definition['map_sha256']
  for record,key in ((definition,'prefix_hex'),*[(t,'raw_hex') for t in definition['dependent_tags']]):
   raw=bytes.fromhex(record[key])
   with path.open('rb') as stream:stream.seek(record['file_offset']);assert stream.read(len(raw))==raw
 logs={};binaries={};archives={};parent_validation=read(P/'validation.json')
 expected=f"PASS original TU1.5 vehicle contact state: 12518 cases, {sum(len(c['instructions']) for c in f['cases'])} instructions, {sum(len(c['writes']) for c in f['cases'])} ordered writes, {sum(len(bytes.fromhex(b['hex'])) for c in f['cases'] for b in c['final_banks'])} full-state bytes"
 for mode in ('release','sanitizer','production'):
  for stage in ('configure','build','test'):logs[mode+'-'+stage]=sha(E/(mode+'-'+stage+'.log'))
  output=(E/(mode+'-test.log')).read_text();assert output.count(expected)==1 and '100% tests passed' in output and 'Skipped' not in output
  assert re.search(r'original_physics_vehicle_contact_state_tu15\s+\.+\s+Passed',output)
  directory=ROOT/'build-vehicle-control-20261011' if mode=='production' else E/('build-'+mode)
  binaries[mode]=sha(directory/'physics_vehicle_contact_state_tests');ninja=(directory/'build.ninja').read_text()
  block=ninja.split('CMakeFiles/halo2_physics_vehicle_contact_state.dir/src/physics_vehicle_contact_state_tu15_generated.cpp.o:')[1].split('\n\n')[0]
  for flag in ('-fno-fast-math','-ffp-contract=off','-frounding-math'):assert flag in block
  if mode=='sanitizer':assert '-fsanitize=address,undefined' in block
  if mode!='production':
   parent_build=P/('build-'+mode);cache=(directory/'CMakeCache.txt').read_text()
   assert f'HALO2_REUSE_PARENT_BUILD:PATH={parent_build}' in cache
   assert sha(parent_build/'physics_vehicle_motion_math_tests')==parent_validation['binaries'][mode]
   archives[mode]={part:sha(parent_build/f'libhalo2_physics_vehicle_{part}.a') for part in PARTS}
  else:
   for part in PARTS:assert re.search('original_physics_vehicle_'+part+r'_u5\s+\.+\s+Passed',output)
 result=dict(schema=1,parity_target='English TU1.5 only',source_manifest_sha256=sha(E/'source-manifest-v1.json'),
  parent_validation_sha256=VALIDATION_PIN,validator_sha256=sha(__file__),research_sha256=sha(E/'research-verified/report.json'),
  generator_manifest_sha256=sha(E/'generated-verified/manifest.json'),hardware_fixture_sha256=sha(E/'hardware-verified/u5.json'),
  native_fixture_sha256=sha(E/'hardware-verified/u5-native.json'),native_cases=12518,complete_cases=10086,
  required_owner_prefixes=2432,compiled_bodies=99,compiled_sites=11216,new_bodies=3,new_sites=160,exercised_new_sites=151,
  exercised_sites=7342,staged_motion_cases=432,all_staged_motion_cases_return=True,
  observer_events=sum(len(c['instructions']) for c in f['cases']),hardware_steps=h['single_stepped_original_instructions'],
  ordered_writes=sum(len(c['writes']) for c in f['cases']),retained_bytes=sum(len(bytes.fromhex(b['hex'])) for c in f['cases'] for b in c['final_banks']),
  binaries=binaries,reused_parent_archives=archives,logs=logs,parent_sources_changed=False,
  tu15_release_sanitizer_production_passed=True,original_code_used_in_production=False,
  required_owner_results_supplied=False,full_physics_parity=False)
 (E/'validation.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(cases=12518,validation_sha256=sha(E/'validation.json'))))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--freeze',action='store_true');a=p.parse_args();freeze() if a.freeze else validate()
