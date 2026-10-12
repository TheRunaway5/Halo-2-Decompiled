#!/usr/bin/env python3
"""Seal original contact ABI/packet evidence and its exact native build."""
import hashlib,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/physics-contact-runtime-20261011'
B=ROOT/'analysis/multiplayer/integration/physics-engine-20261010'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 parent=B/'source-manifest-v5.json';assert sha(parent)=='5f5bfc7cdf01cbf6a12d81de9e6dc1f3eaddcc9ae902392cf16ad926ec57f8b5'
 old=json.loads(parent.read_text());manifest=B/'source-manifest-v6.json';source=json.loads(manifest.read_text())
 assert source['parent_manifest_sha256']==sha(parent)
 for rel,pin in old['files'].items():
  assert sha(B/'source-v5'/rel)==pin,rel
  if rel not in source['owned_overlays']:assert source['files'][rel]==pin,rel
 for rel,pin in source['files'].items():assert sha(B/'source-v6'/rel)==pin,rel
 for rel in source['owned_overlays']:
  if rel!='CMakeLists.txt':assert sha(ROOT/rel)==source['files'][rel],rel
 extra='include(cmake/PhysicsContactRuntime.cmake)\n'
 assert (B/'source-v6/CMakeLists.txt').read_text()==(B/'source-v5/CMakeLists.txt').read_text().replace('include(cmake/PhysicsSolver.cmake)\n','include(cmake/PhysicsSolver.cmake)\n'+extra)
 assert extra in (ROOT/'CMakeLists.txt').read_text()
 paired=E/'correspondence-v3/complete-report.json';assert sha(paired)=='2e1ff8be89e05fbca990255bee2523e975769dcdc988ca8f200665d43b6a1dde'
 p=json.loads(paired.read_text());assert p['complete_bodies']==70
 assert p['extension_tool_sha256']==sha(ROOT/'tools/research_physics_contact_runtime_20261011.py')
 assert sha(E/'correspondence-repeat/complete-report.json')==sha(paired)
 generated=E/'generated-manifest-v2.json';g=json.loads(generated.read_text())
 assert g['correspondence_sha256']==sha(paired) and g['generator_sha256']==sha(ROOT/'tools/generate_physics_contact_runtime_20261011.py')
 assert g['generated_cpp_sha256']==sha(ROOT/'src/physics_contact_runtime_generated.cpp')==sha(E/'generated-repeat/runtime.cpp')
 assert sha(generated)==sha(E/'generated-repeat/manifest.json')
 assert len(g['complete_native_entries'])==70 and len(g['lowered_instructions'])==8386
 assert not g['production_opcode_decoder'] and not g['production_original_code_execution']
 binding=E/'input-bindings-final-v2/report.json';assert sha(binding)=='3066f2456593dc29620a8f7a68ed4372a3ee95954435e57665e2ebae2b66987b'
 assert sha(binding)==sha(E/'input-bindings-repeat/report.json')
 q=json.loads(binding.read_text());assert q['contact_correspondence_sha256']==sha(paired)
 assert q['tool_sha256']==sha(ROOT/'tools/research_physics_contact_bindings_20261011.py')
 assert len(q['routines'])==7 and sum(row['instructions'] for row in q['routines'])==878 and len(q['verified_virtual_slots'])==6
 fixtures={};seen=set();totals=[0,0,0,0];estimates=0
 for name,location,repeat,pin in (
  ('disc','disc-original-verified','disc-original-repeat','04bcca152cac843ec159afd715894f9b467040c8f777ae18a0f8e580b28e71ea'),
  ('u5','u5-rebound-verified','u5-rebound-repeat','f97a859027276f5d4573bd2ae4d3334d987209f010b1baf47c315b2b270f9a3e')):
  file=E/location/'fixture.json';assert sha(file)==pin==sha(E/repeat/'fixture.json')
  f=json.loads(file.read_text());assert f['correspondence_sha256']==sha(paired)
  tool='oracle_physics_contact_runtime_20261011.py' if name=='disc' else 'oracle_physics_contact_rebound_20261011.py'
  assert f['tool_sha256']==sha(ROOT/'tools'/tool)
  assert not f['native_expected_data_used']
  if name=='disc':
   assert not f['instruction_patches'] and f['prior_complete_measurement_identical']
  else:
   assert not f['source_instruction_patches'] and not f['live_gameplay_proof']
   assert f['input_bindings_sha256']==sha(binding) and f['input_fixture_sha256']==fixtures['disc']['sha256']
  counts=[len(f['cases']),sum(len(c['instructions']) for c in f['cases']),sum(len(c['writes']) for c in f['cases']),sum(len(bytes.fromhex(b['hex'])) for c in f['cases'] for b in c['final_banks'])]
  assert counts==[2,14944,2686,5577],(name,counts)
  assert sum(c['complete'] for c in f['cases'])==1
  assert sum(len(c['packets']) for c in f['cases'])==2
  assert [c['exit_entry'] for c in f['cases'] if not c['complete']]==[0x313800 if name=='disc' else 0x317350]
  for c in f['cases']:seen.update(c['instructions']);estimates+=len(c['estimates'])
  fixtures[name]=dict(sha256=pin,cases=2,instructions=14944,ordered_writes=2686,full_state_bytes=5577,packets=2,required_stops=1)
  totals=[x+y for x,y in zip(totals,counts)]
 assert len(seen)==3768 and estimates==4
 rebound=E/'disc-rebound-verified/fixture.json';r=json.loads(rebound.read_text())
 assert r['tool_sha256']==sha(ROOT/'tools/oracle_physics_contact_rebound_20261011.py') and r['input_bindings_sha256']==sha(binding)
 original=json.loads((E/'disc-original-verified/fixture.json').read_text())
 for a,b in zip(original['cases'],r['cases']):
  for key in ('entry_abi','exit','exit_entry','complete','initial_banks','final_banks','writes','instructions','packets','estimates'):assert a[key]==b[key],key
 tests=['world_step','motion_tu15','connected_world','solver_motion','solver_motion_tu15','constraint_step','full_solver','full_solver_tu15','constraint_completed','jacobians','jacobians_tu15','constraint_produced','contact_runtime','contact_runtime_tu15']
 logs={};binaries={}
 for kind,directory in (('production','build-v6'),('asan','asan-v6')):
  for stage in ('configure','build','rebuild','test'):
   path=E/f'{kind}-{stage}.log';logs[path.name]=sha(path)
  result=(E/f'{kind}-test.log').read_text();assert '100% tests passed out of 14' in result and 'Skipped' not in result
  for name in tests:assert re.search(r'\boriginal_physics_'+name+r'\s+\.+\s+Passed',result),name
  for name in ('physics_solver_tests','physics_world_step_tests','physics_jacobian_tests','physics_contact_runtime_tests'):binaries[f'{kind}/{name}']=sha(B/directory/name)
 modules=['physics_world_step','physics_motion_step','physics_constraint_step','physics_solver_generated','physics_current_broadphase','physics_pair_manager','physics_agent_scheduler','physics_bsp_children','physics_child_agents','physics_narrowphase_owners','physics_gjk','physics_contact_dispatch','physics_contact_batches','physics_contact_runtime_generated']
 ninja=(B/'asan-v6/build.ninja').read_text()
 for name in modules:assert '-fsanitize=address,undefined' in ninja.split(f'CMakeFiles/halo2_systems.dir/src/{name}.cpp.o:')[1].split('\n\n')[0],name
 for directory in ('build-v6','asan-v6'):
  block=(B/directory/'build.ninja').read_text().split('CMakeFiles/halo2_systems.dir/src/physics_contact_runtime_generated.cpp.o:')[1].split('\n\n')[0]
  for flag in ('-fno-fast-math','-ffp-contract=off','-frounding-math'):assert flag in block
 previous=ROOT/'analysis/multiplayer/sandbox-source/physics-constraint-runtime-20261011/validation.json'
 assert sha(previous)=='73ee7ac5272c1fd8361eee43fb2eb78d018134c96195200f180fb46c3592e27d'
 result=dict(schema=1,validator_sha256=sha(Path(__file__)),frozen_source_manifest_sha256=sha(manifest),frozen_files_verified=len(source['files']),parent_manifest_sha256=sha(parent),previous_validation_sha256=sha(previous),
  correspondence_sha256=sha(paired),complete_paired_native_bodies=70,lowered_instructions=8386,exercised_instructions=3768,extra_input_binding_bodies=7,extra_binding_instructions=878,input_binding_sha256=sha(binding),generated_manifest_sha256=sha(generated),
  fixtures=fixtures,new_cases=totals[0],new_instruction_observations=totals[1],new_ordered_writes=totals[2],new_full_state_bytes=totals[3],packet_copies=4,emulator_only_cpu_estimates=4,
  original_capture_repeats_identical=True,tu15_repeats_identical=True,original_capture_reexecution_identical=True,generated_repeats_identical=True,
  release_checks_passed=14,sanitizer_checks_passed=14,sanitizer_modules=modules,logs=logs,binaries=binaries,
  full_guest_packet_generation_validated=True,production_instruction_decoder=False,production_original_execution=False,original_instruction_patches=[],
  whole_physics_engine_complete=False,live_simulation_mover_replaced=False,live_lockout_lift_fixed=False,published_launcher_replaced=False,
  remaining=['World-manager313800 and remaining constraint membership/deferred mutation owners',
   'Full source world scheduler with real caller ABI and live allocator/controller ownership',
   'Alternate body/motion and vehicle/ragdoll constraints','Xbox-verified CPU estimates','All-map split-screen and LAN gameplay validation'],
  limits=['Original retained/emulator schedule is not live Xbox gameplay',
   'TU1.5 raw inputs are explicitly rebound 1.1 records; not a complete TU1.5 world execution',
   'Unused x87 exception status bits are not hardware parity proof',
   'Existing semantic dispatcher still requires missing original packet call state',
   'Current live host CollisionWorld mover and published launcher remain'])
 path=E/'validation.json';path.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(cases=4,instructions=totals[1],ordered_writes=totals[2],full_state_bytes=totals[3],validation_sha256=sha(path))))
if __name__=='__main__':main()
