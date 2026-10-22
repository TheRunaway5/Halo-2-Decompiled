#!/usr/bin/env python3
"""Seal the TU1.5 vehicle CRT component and audit original hardware evidence."""
import argparse,hashlib,json,re,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/vehicle-motion-math-20261011'
PARENT=ROOT/'analysis/multiplayer/sandbox-source/vehicle-motion-20261011'
PARENT_PIN='279a35483a321b66ce80be0026075e4cd55fef418a261d2919481e06e6ddfd38'
OVERLAYS=('include/halo2/physics_contact_runtime.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp')
OWNED=('include/halo2/physics_vehicle_motion_math.hpp','src/physics/vehicles/physics_vehicle_motion_math_internal.hpp',
 'src/physics/vehicles/physics_vehicle_motion_math_generated.cpp','tests/physics_vehicle_motion_math_tests.cpp',
 'cmake/PhysicsVehicleMotionMath.cmake','tools/research_vehicle_motion_math_20261011.py',
 'tools/generate_vehicle_motion_math_20261011.py','tools/oracle_vehicle_motion_math_20261011.py',
 'tools/oracle_vehicle_motion_math_hardware_20261011.py','tools/vehicle_motion_math_hardware_trace_20261011.cpp',
 'tools/validate_vehicle_motion_math_20261011.py','docs/PHYSICS_VEHICLE_MOTION_MATH_TU15_20261011.md')
TOP='''cmake_minimum_required(VERSION 3.20)
project(halo2_tu15_vehicle_motion_math LANGUAGES CXX)
include(CTest)
option(HALO2_SANITIZERS "Instrument the complete component" OFF)
find_package(nlohmann_json CONFIG REQUIRED)
include(cmake/PhysicsVehicleRuntime.cmake)
include(cmake/PhysicsVehicleUpdate.cmake)
include(cmake/PhysicsVehicleMotion.cmake)
include(cmake/PhysicsVehicleMotionMath.cmake)
'''
REGISTRATION='include(cmake/PhysicsVehicleMotionMath.cmake)'
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def read(path):return json.loads(Path(path).read_text())
def parent():
 assert sha(PARENT/'source-manifest-v1.json')==PARENT_PIN
 m=read(PARENT/'source-manifest-v1.json');changes={}
 for relative,pin in m['files'].items():
  assert sha(PARENT/'source-v1'/relative)==pin,relative
  current=sha(ROOT/relative)
  if relative in OVERLAYS:changes[relative]=dict(before=pin,after=current)
  else:assert current==pin,relative
 return m,changes
def dependencies():
 m,_=parent();files=set(m['files'])|set(OWNED);pending=[ROOT/p for p in files if Path(p).suffix in ('.cpp','.hpp')];seen=set(pending)
 while pending:
  p=pending.pop()
  for inc in re.findall(r'^#include "([^"]+)"',p.read_text(),re.M):
   matches=[q.resolve() for q in (p.parent/inc,ROOT/'include'/inc,ROOT/'src'/inc) if q.is_file()];assert matches,(p,inc)
   q=matches[0];files.add(str(q.relative_to(ROOT)))
   if q not in seen:seen.add(q);pending.append(q)
 return sorted(files)
def freeze():
 assert REGISTRATION in (ROOT/'CMakeLists.txt').read_text();_,changes=parent();destination=E/'source-v1';destination.mkdir()
 files={p:sha(ROOT/p) for p in dependencies()}
 for p,pin in files.items():
  q=destination/p;q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/p,q);assert sha(q)==pin==sha(ROOT/p)
 (destination/'CMakeLists.txt').write_text(TOP);(destination/'analysis').symlink_to(ROOT/'analysis',target_is_directory=True)
 manifest=dict(schema=1,parity_target='English TU1.5 only',files=files,owned=OWNED,overlays=changes,parent_manifest_sha256=PARENT_PIN,standalone_top_sha256=sha(destination/'CMakeLists.txt'),registration=REGISTRATION)
 (E/'source-manifest-v1.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(files=len(files),sha256=sha(E/'source-manifest-v1.json'))))
def validate():
 manifest=read(E/'source-manifest-v1.json');_,changes=parent();assert manifest['overlays']==changes and manifest['parent_manifest_sha256']==PARENT_PIN
 assert sorted(manifest['files'])==dependencies() and manifest['owned']==list(OWNED)
 for p,pin in manifest['files'].items():assert sha(ROOT/p)==pin==sha(E/'source-v1'/p),p
 assert (E/'source-v1/CMakeLists.txt').read_text()==TOP and sha(E/'source-v1/CMakeLists.txt')==manifest['standalone_top_sha256']
 assert REGISTRATION in (ROOT/'CMakeLists.txt').read_text()
 paired=read(E/'correspondence-verified/report.json');assert sha(E/'correspondence-verified/report.json')==sha(E/'correspondence-repeat/report.json')
 assert paired['extension_tool_sha256']==sha(ROOT/'tools/research_vehicle_motion_math_20261011.py')
 assert paired['complete_bodies']==96 and len(paired['instruction_map'])==11056 and len(paired['additional_native_entries'])==16
 for row in paired['routines']:
  for label in ('disc','later'):
   listing=row[label+'_listing']
   for directory in ('correspondence-verified','correspondence-repeat'):assert sha(E/directory/'instructions'/listing['file'])==listing['sha256']
 generated=read(E/'generated-verified/manifest.json');assert sha(E/'generated-verified/manifest.json')==sha(E/'generated-repeat/manifest.json')
 assert generated['generator_sha256']==sha(ROOT/'tools/generate_vehicle_motion_math_20261011.py')
 assert generated['correspondence_sha256']==sha(E/'correspondence-verified/report.json')
 assert generated['generated_cpp_sha256']==sha(ROOT/'src/physics/vehicles/physics_vehicle_motion_math_generated.cpp')==sha(E/'generated-repeat/physics_vehicle_motion_math_generated.cpp')
 assert len(generated['lowered_instructions'])==443 and not generated['production_opcode_decoder'] and not generated['production_original_code_execution']
 original=read(E/'original-verified/report.json');assert sha(E/'original-verified/report.json')==sha(E/'original-repeat/report.json')
 assert set(original['fixtures'])=={'u5'} and original['tool_sha256']==sha(ROOT/'tools/oracle_vehicle_motion_math_20261011.py')
 for directory in ('original-verified','original-repeat'):assert sha(E/directory/'u5.json')==original['fixtures']['u5']['fixture_sha256']
 hardware=read(E/'hardware-verified/u5.json');fixture=read(E/'hardware-verified/u5-native.json')
 for name in ('u5.json','u5-native.json','report.json'):assert sha(E/'hardware-verified'/name)==sha(E/'hardware-repeat'/name),name
 assert hardware['tool_sha256']==sha(ROOT/'tools/oracle_vehicle_motion_math_hardware_20261011.py')
 assert hardware['trace_observer_sha256']==sha(ROOT/'tools/vehicle_motion_math_hardware_trace_20261011.cpp')
 assert not hardware['paired_instruction_bytes_changed'] and not hardware['original_code_used_in_production']
 assert hardware['original_sha256']==paired['later_owned_xbe_sha256'] and hardware['input_fixture_sha256']==sha(E/'original-verified/u5.json')
 assert hardware['single_stepped_original_instructions']==3036319 and fixture['oracle_backend']=='original_i386_hardware_single_step'
 assert len(fixture['cases'])==8598 and sum(c['complete'] for c in fixture['cases'])==6118
 assert sum(len(c['instructions']) for c in fixture['cases'])==3039202
 assert sum(len(c['writes']) for c in fixture['cases'])==506987
 assert sum(len(bytes.fromhex(b['hex'])) for c in fixture['cases'] for b in c['final_banks'])==1602967
 inputs=read(E/'original-verified/u5.json');by_id={c['id']:c for c in hardware['cases']};seen=set()
 for c,initial in zip(fixture['cases'],inputs['cases']):
  h=by_id[c['id']];assert c['entry_abi']==initial['entry_abi'] and c['initial_banks']==initial['initial_banks'] and c['subsequent_call']==initial['subsequent_call']
  for k in ('exit','final_banks','instructions'):assert c[k]==h[k]
  assert [{k:v for k,v in e.items() if k!='kind'} for e in c['writes']]==h['writes'];seen.update(c['instructions'])
 assert len(seen)==6896 and seen<=set(int(p,16) for p in paired['instruction_map'])
 for definition in fixture['definitions']:
  path=ROOT/definition['map'];assert sha(path)==definition['map_sha256']
  for record,key in ((definition,'prefix_hex'),*[(t,'raw_hex') for t in definition['dependent_tags']]):
   raw=bytes.fromhex(record[key]);f=path.open('rb');f.seek(record['file_offset']);assert f.read(len(raw))==raw;f.close()
 logs={};binaries={}
 for mode in ('release','sanitizer','production'):
  for stage in ('configure','build','test'):logs[mode+'-'+stage]=sha(E/(mode+'-'+stage+'.log'))
  output=(E/(mode+'-test.log')).read_text();assert '100% tests passed' in output and 'Skipped' not in output
  assert output.count('PASS original physics vehicle motion math: 8598 cases, 3039202 instructions, 506987 ordered writes, 1602967 full-state bytes')==1
  for part in ('runtime','update','motion','motion_math'):assert re.search('original_physics_vehicle_'+part+r'_u5\s+\.+\s+Passed',output)
  directory=ROOT/'build-vehicle-control-20261011' if mode=='production' else E/('build-'+mode)
  binaries[mode]=sha(directory/'physics_vehicle_motion_math_tests');ninja=(directory/'build.ninja').read_text()
  block=ninja.split('CMakeFiles/halo2_physics_vehicle_motion_math.dir/src/physics_vehicle_motion_math_generated.cpp.o:')[1].split('\n\n')[0]
  for flag in ('-fno-fast-math','-ffp-contract=off','-frounding-math'):assert flag in block
  if mode=='sanitizer':assert '-fsanitize=address,undefined' in block
  if mode=='production':assert len(re.findall(r'Test\s+#\d+: original_physics_world_\S*tu15\s+\.+\s+Passed',output))==10
 result=dict(schema=1,parity_target='English TU1.5 only',validator_sha256=sha(__file__),source_manifest_sha256=sha(E/'source-manifest-v1.json'),
  correspondence_sha256=sha(E/'correspondence-verified/report.json'),generated_manifest_sha256=sha(E/'generated-verified/manifest.json'),
  hardware_fixture_sha256=sha(E/'hardware-verified/u5.json'),native_fixture_sha256=sha(E/'hardware-verified/u5-native.json'),
  native_cases=8598,complete_cases=6118,required_owner_prefixes=2480,native_observer_events=3039202,hardware_steps=3036319,
  ordered_writes=506987,retained_bytes=1602967,compiled_bodies=96,compiled_sites=11056,exercised_sites=6896,
  new_bodies=16,new_sites=443,overlays=changes,logs=logs,binaries=binaries,
  current_world_source_sha256=sha(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp'),current_world_binary_sha256=sha(ROOT/'build-vehicle-control-20261011/physics_world_runtime_tests'),
  production_uses_original_executable=False,unported_owner_results_supplied=False,tu15_release_sanitizer_production_passed=True,full_physics_parity=False)
 (E/'validation.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(cases=8598,validation_sha256=sha(E/'validation.json'))))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--freeze',action='store_true');a=p.parse_args();freeze() if a.freeze else validate()
