#!/usr/bin/env python3
"""Seal native world ABI component evidence; do not claim live match parity."""
import hashlib,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/physics-world-runtime-20261011'
B=ROOT/'analysis/multiplayer/integration/physics-engine-20261010'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 parent=B/'source-manifest-v7.json';assert sha(parent)=='7af339c537258f521bb1d022c6ed88c1d788e24a91a0b3f3113ae5e0eaf10fdb'
 old=json.loads(parent.read_text());manifest=B/'source-manifest-v8.json';source=json.loads(manifest.read_text());assert source['parent_manifest_sha256']==sha(parent)
 for rel,pin in old['files'].items():
  assert sha(B/'source-v7'/rel)==pin,rel
  if rel not in source['owned_overlays']:assert source['files'][rel]==pin,rel
 for rel,pin in source['files'].items():assert sha(B/'source-v8'/rel)==pin,rel
 for rel in source['owned_overlays']:assert sha(ROOT/rel)==source['files'][rel],rel
 assert (B/'source-v8/CMakeLists.txt').read_bytes()==(B/'source-v7/CMakeLists.txt').read_bytes()
 assert (B/'source-v8/cmake/PhysicsContactRuntime.cmake').read_bytes()==(B/'source-v7/cmake/PhysicsContactRuntime.cmake').read_bytes()+b'\ninclude(${CMAKE_CURRENT_LIST_DIR}/PhysicsWorldRuntime.cmake)\n'
 paired=E/'correspondence-v4/complete-report.json';assert sha(paired)=='f40e922cf11726f4a3fb2e96739bd537c482c74bcff7cd30b3e33577655203ad'
 p=json.loads(paired.read_text());assert p['complete_bodies']==202 and sum(p['mnemonics'].values())==20747
 assert p['extension_tool_sha256']==sha(ROOT/'tools/research_physics_world_runtime_20261011.py')
 assert sha(paired)==sha(E/'correspondence-v4-repeat/complete-report.json')
 for row in p['routines']:
  for label in ('disc','later'):
   q=row[label+'_listing'];assert sha(paired.parent/'instructions'/q['file'])==q['sha256']
 generated=E/'generated-manifest-v4.json';g=json.loads(generated.read_text())
 assert g['correspondence_sha256']==sha(paired) and g['generator_sha256']==sha(ROOT/'tools/generate_physics_world_runtime_20261011.py')
 assert g['generated_cpp_sha256']==sha(ROOT/'src/physics_world_runtime_generated.cpp')==sha(E/'generated-repeat/runtime.cpp')
 assert sha(generated)==sha(E/'generated-repeat/manifest.json') and len(g['lowered_instructions'])==20747 and len(g['complete_native_entries'])==202
 assert not g['production_opcode_decoder'] and not g['production_original_code_execution']
 binding=E/'input-bindings-extra-v4/report.json';q=json.loads(binding.read_text());assert q['tool_sha256']==sha(ROOT/'tools/research_physics_world_runtime_extra_bindings_20261011.py')
 assert sha(binding)==sha(E/'input-bindings-extra-repeat/report.json') and q['correspondence_sha256']==sha(paired)
 regular=E/'input-bindings-v4/report.json';assert q['parent_input_bindings_sha256']==sha(regular)==sha(E/'input-bindings-repeat/report.json')
 assert len(q['verified_virtual_slots'])==25
 for row in q['additional_routines']:
  for label in ('disc','later'):
   listing=row[label+'_listing'];assert sha(binding.parent/listing['file'])==listing['sha256']
 fixtures={};totals=[0,0,0,0];seen=set();estimates=0
 for name,location,repeat,count,latest,stress in (
  ('disc','disc-original-verified','disc-original-repeat',1,False,False),
  ('u5','u5-rebound-verified','u5-rebound-repeat',1,True,False),
  ('disc_fp','disc-x87-verified','disc-x87-repeat',24,False,True),
  ('u5_fp','u5-x87-verified','u5-x87-repeat',24,True,True)):
  file=E/location/'fixture.json';assert sha(file)==sha(E/repeat/'fixture.json');f=json.loads(file.read_text())
  assert f['correspondence_sha256']==sha(paired) and not f['native_expected_data_used']
  tool='oracle_physics_world_runtime_20261011.py' if name=='disc' else 'oracle_physics_world_runtime_rebound_20261011.py';assert f['tool_sha256']==sha(ROOT/'tools'/tool)
  if name=='disc':assert not f['instruction_patches'] and f['prior_complete_measurement_identical']
  else:
   assert not f['source_instruction_patches'] and not f['live_gameplay_proof']
   assert f['input_fixture_sha256']==fixtures['disc']['sha256'] and f['input_bindings_sha256']==sha(binding)
   assert f['declared_x87_input_perturbations']==stress
  counts=[len(f['cases']),sum(len(c['instructions']) for c in f['cases']),sum(len(c['writes']) for c in f['cases']),sum(len(bytes.fromhex(b['hex'])) for c in f['cases'] for b in c['final_banks'])]
  assert counts==[count,14033*count,2718*count,5188*count],(name,counts)
  assert not any(c['complete'] for c in f['cases'])
  stop=p['address_map']['0031c2e0'] if latest else 0x31c2e0;assert {c['exit_entry'] for c in f['cases']}=={stop}
  assert sum(len(c['packets']) for c in f['cases'])==2*count and not any(c['timestamps'] for c in f['cases'])
  if stress:assert {(c['input_perturbation']['original_case'],c['input_perturbation']['depth'],c['input_perturbation']['top']) for c in f['cases']}=={(0,d,t) for d in (0,1,3) for t in range(8)}
  for c in f['cases']:seen.update(c['instructions']);estimates+=len(c['estimates'])
  fixtures[name]=dict(sha256=sha(file),cases=count,instructions=counts[1],ordered_writes=counts[2],full_state_bytes=counts[3],packets=count*2,required_stops=count,declared_fp_stress=stress)
  totals=[a+b for a,b in zip(totals,counts)]
 assert totals==[50,701650,135900,259400] and estimates==100
 original=json.loads((E/'disc-original-verified/fixture.json').read_text());replay=json.loads((E/'disc-rebound-v4/fixture.json').read_text())
 for a,b in zip(original['cases'],replay['cases']):
  for key in ('entry_abi','exit','exit_entry','complete','initial_banks','final_banks','writes','instructions','packets','estimates','timestamps'):assert a[key]==b[key],key
 tests=['world_runtime','world_runtime_tu15','world_runtime_x87','world_runtime_x87_tu15','world_runtime_retained_contacts','world_runtime_retained_contacts_tu15'];logs={};binaries={}
 for kind,directory in (('production','build-v8'),('asan','asan-v8')):
  for stage in ('configure','build','rebuild','test'):
   file=E/f'{kind}-{stage}.log';logs[file.name]=sha(file)
  output=(E/f'{kind}-test.log').read_text();assert '100% tests passed out of 6' in output and 'Skipped' not in output
  for test in tests:assert re.search(r'\boriginal_physics_'+test+r'\s+\.+\s+Passed',output),test
  binaries[kind]=sha(B/directory/'physics_world_runtime_tests')
  block=(B/directory/'build.ninja').read_text().split('CMakeFiles/halo2_physics_world_runtime.dir/src/physics_world_runtime_generated.cpp.o:')[1].split('\n\n')[0]
  for flag in ('-fno-fast-math','-ffp-contract=off','-frounding-math'):assert flag in block
  if kind=='asan':assert '-fsanitize=address,undefined' in block and '-O1' in block
 previous=ROOT/'analysis/multiplayer/sandbox-source/physics-world-contact-runtime-20261011/validation.json';assert sha(previous)=='1d5d06a593dc4e8a8ef0b1ae574671afd203673f2186f87bb0dc93e30b362fcf'
 result=dict(schema=1,validator_sha256=sha(__file__),frozen_source_manifest_sha256=sha(manifest),parent_source_manifest_sha256=sha(parent),frozen_files_verified=len(source['files']),
  correspondence_sha256=sha(paired),generated_manifest_sha256=sha(generated),complete_paired_native_bodies=202,lowered_instructions=20747,exercised_instructions=len(seen),input_bindings_sha256=sha(binding),verified_virtual_slots=25,
  fixtures=fixtures,total_world_cases=50,retained_original_world_cases_per_image=1,declared_fp_cases_per_image=24,new_world_instruction_observations=totals[1],new_world_ordered_writes=totals[2],new_world_full_state_bytes=totals[3],packet_copies=100,emulator_only_cpu_estimates=100,
  new_native_checks_passed=6,new_sanitizer_checks_passed=6,previous_unchanged_cohort_validation_sha256=sha(previous),previous_cohort_checks_per_build=18,logs=logs,binaries=binaries,
  original_capture_repeats_identical=True,original_minimal_input_reexecution_identical=True,tu15_and_fp_repeats_identical=True,correspondence_and_generation_repeats_identical=True,
  production_opcode_decoder=False,production_original_execution=False,source_instruction_patches=[],
  next_owner=dict(disc='0031c2e0',tu15=f'{p["address_map"]["0031c2e0"]:08x}'),whole_physics_engine_complete=False,live_simulation_mover_replaced=False,published_launcher_replaced=False,
  remaining=['Contact listener dispatcher31C2E0 and further required world/game callbacks','Actual live world/allocator/controller construction and match integration','Other motion, shape, vehicle, ragdoll and constraint kinds','Xbox CPU estimate and full TU1.5 world validation','All-map split-screen/LAN gameplay and physical external asset relocation'],
  limits=['One retained world prefix is not a complete world step or live gameplay','TU1.5 inputs are explicitly rebound1.1 call state, not TU1.5 full world construction','FP perturbations are not additional gameplay scenarios','Unused x87 exception bits, sparse stacks and stack faults remain unproven','Prior18-check cohort is inherited immutable v7 evidence, not a fresh v8 build','The current host terrain mover remains active'])
 file=E/'validation.json';file.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(cases=50,instructions=totals[1],ordered_writes=totals[2],full_state_bytes=totals[3],validation_sha256=sha(file))))
if __name__=='__main__':main()
