#!/usr/bin/env python3
"""Pin complete solver comparisons and isolated full-library build provenance."""
import hashlib,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/physics-solver-20261011'
B=ROOT/'analysis/multiplayer/integration/physics-engine-20261010'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    manifest=B/'source-manifest-v4.json';source=json.loads(manifest.read_text())
    assert source['parent_manifest_sha256']=='8f3dcd04c9357573a32d79d3fab0df3e148a5baf6c625b5c8780aa89af65743a'
    for rel,pin in source['files'].items():assert sha(B/'source-v4'/rel)==pin,rel
    for rel in source['owned_overlays']:
        if rel!='CMakeLists.txt':assert sha(ROOT/rel)==source['files'][rel],rel
    correspondence=E/'correspondence-verified/complete-report.json'
    assert sha(correspondence)=='b6793a21f1780bee254eabafd229d39810f26e715369ba22eba16aeb2c98809d'
    paired=json.loads(correspondence.read_text());assert paired['complete_bodies']==25
    assert paired['extension_tool_sha256']==sha(ROOT/'tools/research_physics_solver_20261011.py')
    generated=E/'generated-manifest-v3.json';g=json.loads(generated.read_text())
    assert g['correspondence_sha256']==sha(correspondence)
    assert g['generator_sha256']==sha(ROOT/'tools/generate_physics_solver_20261011.py')
    assert g['generated_cpp_sha256']==sha(ROOT/'src/physics/runtime/physics_solver_generated.cpp')
    assert len(g['lowered_instructions'])==2494
    assert not g['production_opcode_decoder'] and not g['production_original_code_execution']
    fixtures={};totals=[0,0,0,0];instruction_count=0;seen=set()
    for name,pin,expected in (
        ('disc','deca4fd9285c71bc8d46d7d41570ff1ea3ebd79edcaab2ee404057ac937585af',[422,81053,55324702,20]),
        ('u5','2ce65f6b918bde3b1de5616e6cacddf6c10313aa4ed1f5ccc45c02a336a7b7ad',[422,81053,55324702,20]),
        ('completed','c88685d40e92fc9a92e04a695910fbabfd9ede06eed9a7e4894962fd5e4011f1',[16,2640,1052784,0])):
        path=E/f'{name}-verified/fixture.json';assert sha(path)==pin
        assert sha(E/f'{name}-repeat/fixture.json')==pin
        f=json.loads(path.read_text())
        assert not f['source_instruction_patches'] and not f['native_expected_data_used'] and not f['live_gameplay_proof']
        tool='oracle_physics_constraint_completed_20261011.py' if name=='completed' else 'oracle_physics_solver_20261011.py'
        assert f['tool_sha256']==sha(ROOT/'tools'/tool)
        actual=[len(f['cases']),sum(len(c['writes']) for c in f['cases']),
            sum(sum(len(data)//2 for _,data in c['after_banks']) for c in f['cases']),
            sum(c['boundary'] is not None for c in f['cases'])]
        assert actual==expected,(name,actual)
        if name!='completed':
            assert f['correspondence_sha256']==sha(correspondence)
            instruction_count+=sum(len(c['trace']) for c in f['cases'])
            seen.update(p for c in f['cases'] for p in c['trace'])
            assert {p for c in f['cases'] for p in c['declared_input_controls']['schema_selectors']}==set(range(1,17))
        fixtures[name]=dict(sha256=pin,cases=actual[0],ordered_writes=actual[1],full_state_bytes=actual[2],required_stops=actual[3])
        totals=[a+b for a,b in zip(totals,actual)]
    assert len(seen)==2366
    bank_dependency=E/'disc-full-v1/fixture.json'
    assert sha(bank_dependency)=='325988532f637b8a853acc0b339a3e6705b266b4402ca1322740649db15deb48'
    tests=['original_physics_world_step','original_physics_motion_tu15','original_physics_connected_world',
        'original_physics_solver_motion','original_physics_solver_motion_tu15','original_physics_constraint_step',
        'original_physics_full_solver','original_physics_full_solver_tu15','original_physics_constraint_completed']
    logs={};binaries={}
    for kind,directory in (('production','build-v4'),('asan','asan-v4')):
        for stage in ('configure','build','test'):
            path=E/f'{kind}-{stage}.log';logs[path.name]=sha(path)
        result=(E/f'{kind}-test.log').read_text()
        assert '100% tests passed out of 9' in result and 'Skipped' not in result
        for name in tests:assert re.search(r'\b'+name+r'\s+\.+\s+Passed',result),name
        for name in ('physics_solver_tests','physics_world_step_tests'):binaries[f'{kind}/{name}']=sha(B/directory/name)
    modules=['physics_world_step','physics_motion_step','physics_constraint_step','physics_solver_generated',
        'physics_current_broadphase','physics_pair_manager','physics_agent_scheduler','physics_bsp_children',
        'physics_child_agents','physics_narrowphase_owners','physics_gjk','physics_contact_dispatch','physics_contact_batches']
    ninja=(B/'asan-v4/build.ninja').read_text()
    for name in modules:
        block=ninja.split(f'CMakeFiles/halo2_systems.dir/src/{name}.cpp.o:')[1].split('\n\n')[0]
        assert '-fsanitize=address,undefined' in block,name
    for kind in ('build-v4','asan-v4'):
        block=(B/kind/'build.ninja').read_text().split('CMakeFiles/halo2_systems.dir/src/physics_solver_generated.cpp.o:')[1].split('\n\n')[0]
        for flag in ('-fno-fast-math','-ffp-contract=off','-frounding-math'):assert flag in block
    result=dict(schema=1,validator_sha256=sha(Path(__file__)),frozen_source_manifest_sha256=sha(manifest),
        frozen_files_verified=len(source['files']),correspondence_sha256=sha(correspondence),complete_paired_bodies=25,
        generated_manifest_sha256=sha(generated),lowered_solver_instructions=2494,exercised_solver_instructions=len(seen),
        original_instruction_observations=instruction_count,fixtures=fixtures,new_cases=totals[0],new_ordered_writes=totals[1],
        new_full_state_bytes=totals[2],new_required_stops=totals[3],logs=logs,binaries=binaries,
        release_checks_passed=9,sanitizer_checks_passed=9,sanitizer_modules=modules,
        compiled_solver_connected_to_island_wrapper=True,source_produced_jacobians_validated=False,
        completed_wrapper_retained_bank_range_dependency_sha256=sha(bank_dependency),
        completed_wrapper_diagnostic_cases=16,whole_physics_engine_complete=False,live_simulation_mover_replaced=False,
        published_launcher_replaced=False,live_lockout_lift_fixed=False,
        limits=['Unicorn comparisons do not prove Xbox hardware rounding',
            'Raw schema/row controls do not prove actual game constraint production',
            'Initialization alternate clock/error and schema14 callback remain required owners',
            'Readback RSQRTSS comparison inputs are emulator-only, not a production estimate provider',
            'Existing live host CollisionWorld mover remains'],
        remaining=['Real Jacobian and retained contact-packet production','Deferred mutation/contact delivery',
            'Live allocator/world/controller and TU1.5 mutable bindings','Vehicle/ragdoll body classes',
            'Production source-compatible RSQRTSS estimates','All-map split-screen/LAN gameplay validation'])
    path=E/'validation.json';path.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(new_cases=totals[0],writes=totals[1],bytes=totals[2],required_stops=totals[3],validation_sha256=sha(path))))
if __name__=='__main__':main()
