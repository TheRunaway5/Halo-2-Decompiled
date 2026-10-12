#!/usr/bin/env python3
"""Seal the expanded contact-to-world native cohort without a live-game claim."""
import hashlib,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/physics-world-contact-runtime-20261011'
B=ROOT/'analysis/multiplayer/integration/physics-engine-20261010'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 parent=B/'source-manifest-v6.json';assert sha(parent)=='09426059166d2591ef58f04d64cba31ee1f7eba00fd5d1494f8f4dc0b24d384d'
 old=json.loads(parent.read_text());manifest=B/'source-manifest-v7.json';source=json.loads(manifest.read_text())
 assert source['parent_manifest_sha256']==sha(parent)
 for rel,pin in old['files'].items():
  assert sha(B/'source-v6'/rel)==pin,rel
  if rel not in source['owned_overlays']:assert source['files'][rel]==pin,rel
 for rel,pin in source['files'].items():assert sha(B/'source-v7'/rel)==pin,rel
 for rel in source['owned_overlays']:assert sha(ROOT/rel)==source['files'][rel],rel
 assert (B/'source-v7/CMakeLists.txt').read_bytes()==(B/'source-v6/CMakeLists.txt').read_bytes()
 paired=E/'correspondence-v2/complete-report.json';assert sha(paired)=='257cf7d69e56957067898e7bf2df455f9bc54e93b8851dafc2e8fac74edbb17b'
 p=json.loads(paired.read_text());assert p['complete_bodies']==82 and sum(p['mnemonics'].values())==8926
 assert p['extension_tool_sha256']==sha(ROOT/'tools/research_physics_world_contact_runtime_20261011.py')
 assert sha(E/'correspondence-repeat/complete-report.json')==sha(paired)
 generated=E/'generated-manifest-v2.json';g=json.loads(generated.read_text())
 assert g['correspondence_sha256']==sha(paired) and g['generator_sha256']==sha(ROOT/'tools/generate_physics_world_contact_runtime_20261011.py')
 assert g['generated_cpp_sha256']==sha(ROOT/'src/physics_contact_runtime_generated.cpp')==sha(E/'generated-repeat/runtime.cpp')
 assert sha(generated)==sha(E/'generated-repeat/manifest.json')
 assert len(g['complete_native_entries'])==82 and len(g['lowered_instructions'])==8926
 assert not g['production_opcode_decoder'] and not g['production_original_code_execution']
 binding=E/'input-bindings-v3/report.json';assert sha(binding)=='e9b90473f101c65bd6d303579bf77526bcb0ee8ee8ca81fb3151f74d1bb73bec'
 assert sha(binding)==sha(E/'input-bindings-v3-repeat/report.json')
 q=json.loads(binding.read_text());assert q['contact_correspondence_sha256']==sha(paired)
 assert q['tool_sha256']==sha(ROOT/'tools/research_physics_world_contact_bindings_20261011.py')
 assert len(q['routines'])==12 and sum(row['instructions'] for row in q['routines'])==1043 and len(q['verified_virtual_slots'])==11
 for report,directory in ((p,paired.parent/'instructions'),(q,binding.parent)):
  for row in report['routines']:
   for label in ('disc','later'):
    listing=row[label+'_listing'];assert sha(directory/listing['file'])==listing['sha256']
 fixtures={};seen=set();totals=[0,0,0,0];estimates=0
 for name,location,repeat,pin,count,latest,stress in (
  ('disc','disc-original-v3','disc-original-repeat','a16eebd92140324ee938abf65f8567a5cf553e15ed939b6fd72e39638bee338f',2,False,False),
  ('u5','u5-rebound-v3','u5-rebound-repeat','0eb706b5fbeba24e0ba144b18f09b2b0672f1075b51b648905581ee0e6b56356',2,True,False),
  ('disc_fp','disc-x87-v3','disc-x87-repeat','f18dc42a042a458d691ba1d77f322ec35d45319b321d16cf1d2e9ef3835b5258',48,False,True),
  ('u5_fp','u5-x87-v3','u5-x87-repeat','206677517b48204e6a20530ef6ce20612793fffe503c79be71f82cdd65b2c454',48,True,True)):
  file=E/location/'fixture.json';assert sha(file)==pin==sha(E/repeat/'fixture.json')
  f=json.loads(file.read_text());assert f['correspondence_sha256']==sha(paired)
  tool='oracle_physics_world_contact_complete_20261011.py' if name=='disc' else 'oracle_physics_world_contact_rebound_20261011.py'
  assert f['tool_sha256']==sha(ROOT/'tools'/tool) and not f['native_expected_data_used']
  if name=='disc':assert not f['instruction_patches'] and f['prior_complete_measurement_identical']
  else:
   assert not f['source_instruction_patches'] and not f['live_gameplay_proof']
   assert f['input_bindings_sha256']==sha(binding) and f['input_fixture_sha256']==fixtures['disc']['sha256']
   assert f['declared_x87_input_perturbations']==stress
  counts=[len(f['cases']),sum(len(c['instructions']) for c in f['cases']),sum(len(c['writes']) for c in f['cases']),sum(len(bytes.fromhex(b['hex'])) for c in f['cases'] for b in c['final_banks'])]
  assert counts==[count,15443*(count//2),2828*(count//2),5940*(count//2)],(name,counts)
  assert sum(c['complete'] for c in f['cases'])==count//2
  assert sum(len(c['packets']) for c in f['cases'])==count
  assert set(c['exit_entry'] for c in f['cases'] if not c['complete'])=={0x322440 if latest else 0x31e8b0}
  if stress:
   assert {(c['input_perturbation']['original_case'],c['input_perturbation']['depth'],c['input_perturbation']['top']) for c in f['cases']}=={(i,d,t) for i in (0,1) for d in (0,1,3) for t in range(8)}
  for c in f['cases']:seen.update(c['instructions']);estimates+=len(c['estimates'])
  fixtures[name]=dict(sha256=pin,cases=count,instructions=counts[1],ordered_writes=counts[2],full_state_bytes=counts[3],packets=count,required_stops=count//2,declared_fp_stress=stress)
  totals=[x+y for x,y in zip(totals,counts)]
 assert totals==[100,772150,141400,297000] and len(seen)==4175 and estimates==100
 rebound=E/'disc-rebound-v3/fixture.json';r=json.loads(rebound.read_text())
 assert r['tool_sha256']==sha(ROOT/'tools/oracle_physics_world_contact_rebound_20261011.py') and r['input_bindings_sha256']==sha(binding)
 original=json.loads((E/'disc-original-v3/fixture.json').read_text())
 for a,b in zip(original['cases'],r['cases']):
  for key in ('entry_abi','exit','exit_entry','complete','initial_banks','final_banks','writes','instructions','packets','estimates'):assert a[key]==b[key],key
 tests=['world_step','motion_tu15','connected_world','solver_motion','solver_motion_tu15','constraint_step','full_solver','full_solver_tu15','constraint_completed','jacobians','jacobians_tu15','constraint_produced','contact_runtime','contact_runtime_tu15','world_contact_runtime','world_contact_runtime_tu15','world_contact_x87','world_contact_x87_tu15']
 logs={};binaries={}
 for kind,directory in (('production','build-v7'),('asan','asan-v7')):
  for stage in ('configure','build','rebuild','test'):
   path=E/f'{kind}-{stage}.log';logs[path.name]=sha(path)
  result=(E/f'{kind}-test.log').read_text();assert '100% tests passed out of 18' in result and 'Skipped' not in result
  for name in tests:assert re.search(r'\boriginal_physics_'+name+r'\s+\.+\s+Passed',result),name
  for name in ('physics_solver_tests','physics_world_step_tests','physics_jacobian_tests','physics_contact_runtime_tests'):binaries[f'{kind}/{name}']=sha(B/directory/name)
 modules=['physics_world_step','physics_motion_step','physics_constraint_step','physics_solver_generated','physics_current_broadphase','physics_pair_manager','physics_agent_scheduler','physics_bsp_children','physics_child_agents','physics_narrowphase_owners','physics_gjk','physics_contact_dispatch','physics_contact_batches','physics_contact_runtime_generated']
 ninja=(B/'asan-v7/build.ninja').read_text()
 for name in modules:assert '-fsanitize=address,undefined' in ninja.split(f'CMakeFiles/halo2_systems.dir/src/{name}.cpp.o:')[1].split('\n\n')[0],name
 for directory in ('build-v7','asan-v7'):
  block=(B/directory/'build.ninja').read_text().split('CMakeFiles/halo2_systems.dir/src/physics_contact_runtime_generated.cpp.o:')[1].split('\n\n')[0]
  for flag in ('-fno-fast-math','-ffp-contract=off','-frounding-math'):assert flag in block
 previous=ROOT/'analysis/multiplayer/sandbox-source/physics-contact-runtime-20261011/validation.json'
 assert sha(previous)=='06d2be23004d0f437de64a7649550e692614b765b8c912bc12bb25d0996b8cc1'
 result=dict(schema=1,validator_sha256=sha(Path(__file__)),frozen_source_manifest_sha256=sha(manifest),frozen_files_verified=len(source['files']),parent_manifest_sha256=sha(parent),previous_validation_sha256=sha(previous),
  correspondence_sha256=sha(paired),complete_paired_native_bodies=82,new_complete_bodies=12,lowered_instructions=8926,exercised_instructions=4175,extra_input_binding_bodies=12,extra_binding_instructions=1043,input_binding_sha256=sha(binding),generated_manifest_sha256=sha(generated),
  fixtures=fixtures,retained_original_cases_per_image=2,declared_fp_stress_cases_per_image=48,total_cases=100,new_instruction_observations=totals[1],new_ordered_writes=totals[2],new_full_state_bytes=totals[3],packet_copies=100,emulator_only_cpu_estimates=100,
  original_capture_repeats_identical=True,tu15_repeats_identical=True,fp_stress_repeats_identical=True,original_capture_reexecution_identical=True,generated_repeats_identical=True,
  release_checks_passed=18,sanitizer_checks_passed=18,sanitizer_modules=modules,logs=logs,binaries=binaries,
  full_guest_world_contact_insertion_validated=True,x87_physical_top_preserved=True,production_instruction_decoder=False,production_original_execution=False,original_instruction_patches=[],
  next_owner=dict(disc='0031e8b0',tu15='00322440'),whole_physics_engine_complete=False,live_simulation_mover_replaced=False,live_lockout_lift_fixed=False,published_launcher_replaced=False,
  remaining=['Listener/game callback chain31E8B0','Full source world scheduler with real caller ABI and live allocator/controller ownership','Alternate body/motion and vehicle/ragdoll constraints','Xbox-verified CPU estimates','All-map split-screen and LAN gameplay validation','Physical external asset relocation and import seam'],
  limits=['Original retained/emulator schedule is not live Xbox gameplay','TU1.5 raw inputs are explicitly rebound 1.1 records; not a complete TU1.5 world execution','FP stress fixtures perturb call state, not additional gameplay scenarios','Unused x87 exception bits, sparse tag stacks and stack-fault handling remain unproven','Existing semantic dispatcher still requires missing original packet call state','Current live host CollisionWorld mover and published launcher remain'])
 path=E/'validation.json';path.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(cases=100,instructions=totals[1],ordered_writes=totals[2],full_state_bytes=totals[3],validation_sha256=sha(path))))
if __name__=='__main__':main()
