#!/usr/bin/env python3
"""Pin actual producer/solver integration evidence without claiming live parity."""
import hashlib,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/physics-constraint-runtime-20261011'
B=ROOT/'analysis/multiplayer/integration/physics-engine-20261010'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    parent=B/'source-manifest-v4.json';assert sha(parent)=='f841db7ca3361eb9e1203950e4f1a3fa5750c293f20d0a116549480e66995b6e'
    old=json.loads(parent.read_text())
    for rel,pin in old['files'].items():assert sha(B/'source-v4'/rel)==pin,rel
    manifest=B/'source-manifest-v5.json';source=json.loads(manifest.read_text())
    assert source['parent_manifest_sha256']==sha(parent)
    for rel,pin in source['files'].items():assert sha(B/'source-v5'/rel)==pin,rel
    for rel in source['owned_overlays']:assert sha(ROOT/rel)==source['files'][rel],rel
    for rel,pin in old['files'].items():
        if rel not in source['owned_overlays']:assert source['files'][rel]==pin,rel
    paired=E/'correspondence-verified/complete-report.json'
    assert sha(paired)=='27291554f1e41b5b58aa4948274a28848943261aff95c5b57998373173d9283b'
    p=json.loads(paired.read_text());assert p['complete_bodies']==36
    assert p['extension_tool_sha256']==sha(ROOT/'tools/research_physics_constraint_runtime_20261011.py')
    generated=E/'generated-manifest-v1.json';g=json.loads(generated.read_text())
    assert g['correspondence_sha256']==sha(paired)
    assert g['generator_sha256']==sha(ROOT/'tools/generate_physics_constraint_runtime_20261011.py')
    assert g['generated_cpp_sha256']==sha(ROOT/'src/physics_solver_generated.cpp')
    assert len(g['lowered_instructions'])==3808 and len(g['complete_native_entries'])==16
    assert not g['production_opcode_decoder'] and not g['production_original_code_execution']
    fixtures={};totals=[0,0,0];observations=0;seen=set();estimates=0;composed_observations=0
    for name,location,repeat,pin,expected in (
        ('disc','disc-full-v1','disc-repeat-v1','e1d3f5d1479be3077cf06d9a92ee2de43802161229f47eb44bbf5fced03e222b',[156,16235,20451296]),
        ('u5','u5-full-v1','u5-repeat-v1','fcebb262d6a5ba292fc50292e029804ce4d547b0eb4376ed9f060c1eda83f1b0',[156,16235,20451296]),
        ('produced','produced-verified','produced-repeat','68d2b3c891aa93807fc1423ecf954b6948d8a6072cf0a9e6bb87fecd379d3f88',[144,56527,9456682])):
        path=E/location/'fixture.json';assert sha(path)==pin and sha(E/repeat/'fixture.json')==pin
        f=json.loads(path.read_text());assert f['correspondence_sha256']==sha(paired)
        assert not f['source_instruction_patches'] and not f['native_expected_data_used'] and not f['live_gameplay_proof']
        tool='oracle_physics_constraint_produced_20261011.py' if name=='produced' else 'oracle_physics_constraint_runtime_20261011.py'
        assert f['tool_sha256']==sha(ROOT/'tools'/tool)
        actual=[len(f['cases']),sum(len(c['writes']) for c in f['cases']),
            sum(len(data)//2 for c in f['cases'] for _,data in c['after_banks'])]
        assert actual==expected,(name,actual)
        assert all(c['boundary'] is None for c in f['cases'])
        for c in f['cases']:
            if name=='produced':
                control=c['declared_input_controls']
                assert not control['existing_schema_supplied'] and not control['solver_result_supplied']
                seen.update(c['constraint_runtime_trace']);observations+=len(c['constraint_runtime_trace'])
                composed_observations+=len(c['trace'])
            else:
                seen.update(c['trace']);observations+=len(c['trace'])
            estimates+=len(c['estimates'])
        if name=='produced':
            assert {c['declared_input_controls']['producer'] for c in f['cases']}=={0x322380,0x31bd00,0x31bbe0}
        else:assert {c['entry'] for c in f['cases']}=={0x31bbe0,0x31bd00,0x322380,0x325bb0,0x324d00,0x325b70,0x326820,0x31b810,0x31b880,0x31b9d0,0x1d2d70}
        fixtures[name]=dict(sha256=pin,cases=actual[0],ordered_writes=actual[1],full_state_bytes=actual[2],required_stops=0)
        totals=[a+b for a,b in zip(totals,actual)]
    old_validation=ROOT/'analysis/multiplayer/sandbox-source/physics-solver-20261011/validation.json'
    assert sha(old_validation)=='fe5c9288b8ce5e331f001629e0791443aa4547d6eec6298765ddc514167a7ba2'
    tests=['world_step','motion_tu15','connected_world','solver_motion','solver_motion_tu15','constraint_step',
        'full_solver','full_solver_tu15','constraint_completed','jacobians','jacobians_tu15','constraint_produced']
    logs={};binaries={}
    for kind,directory in (('production','build-v5'),('asan','asan-v5')):
        for stage in ('configure','build','rebuild','test'):
            path=E/f'{kind}-{stage}.log';logs[path.name]=sha(path)
        result=(E/f'{kind}-test.log').read_text()
        assert '100% tests passed out of 12' in result and 'Skipped' not in result
        for name in tests:assert re.search(r'\boriginal_physics_'+name+r'\s+\.+\s+Passed',result),name
        for name in ('physics_solver_tests','physics_world_step_tests','physics_jacobian_tests'):
            binaries[f'{kind}/{name}']=sha(B/directory/name)
    modules=['physics_world_step','physics_motion_step','physics_constraint_step','physics_solver_generated',
        'physics_current_broadphase','physics_pair_manager','physics_agent_scheduler','physics_bsp_children',
        'physics_child_agents','physics_narrowphase_owners','physics_gjk','physics_contact_dispatch','physics_contact_batches']
    ninja=(B/'asan-v5/build.ninja').read_text()
    for name in modules:
        block=ninja.split(f'CMakeFiles/halo2_systems.dir/src/{name}.cpp.o:')[1].split('\n\n')[0]
        assert '-fsanitize=address,undefined' in block,name
    for kind in ('build-v5','asan-v5'):
        for name in ('physics_constraint_step','physics_solver_generated'):
            block=(B/kind/'build.ninja').read_text().split(f'CMakeFiles/halo2_systems.dir/src/{name}.cpp.o:')[1].split('\n\n')[0]
            for flag in ('-fno-fast-math','-ffp-contract=off','-frounding-math'):assert flag in block
    result=dict(schema=1,validator_sha256=sha(Path(__file__)),frozen_source_manifest_sha256=sha(manifest),
        frozen_files_verified=len(source['files']),unchanged_parent_manifest_sha256=sha(parent),
        previous_solver_validation_sha256=sha(old_validation),correspondence_sha256=sha(paired),complete_paired_bodies=36,
        generated_manifest_sha256=sha(generated),lowered_runtime_instructions=3808,exercised_runtime_instructions=len(seen),
        runtime_instruction_observations=observations,original_composed_instruction_observations=composed_observations,
        fixtures=fixtures,new_cases=totals[0],new_ordered_writes=totals[1],new_full_state_bytes=totals[2],
        emulator_only_estimates_consumed=estimates,logs=logs,binaries=binaries,
        release_checks_passed=12,sanitizer_checks_passed=12,sanitizer_modules=modules,
        source_produced_jacobians_validated=True,compiled_producers_connected_to_island_wrapper=True,
        known_solver_callbacks_compiled=True,whole_physics_engine_complete=False,live_simulation_mover_replaced=False,
        published_launcher_replaced=False,live_lockout_lift_fixed=False,
        limits=['Diagnostic raw contacts are not captured gameplay contacts',
            'Unicorn CPU estimates do not establish Xbox hardware estimates',
            'Discarded producer EAX exception bits differ between Unicorn and host FPU; not compared',
            'Composed check excludes the handwritten outer wrapper ABI stack and full instruction trace',
            'Connected island wrapper retains1.1 mutable bindings; TU1.5 world binding incomplete',
            'Existing live host CollisionWorld mover remains'],
        remaining=['Real live contact/manifold and300400 retained packet-frame production',
            'Deferred world mutation/contact delivery and live allocator/controller ownership',
            'Alternate motion/body classes and vehicle/ragdoll constraints',
            'Production source-compatible RSQRTSS estimates','All-map split-screen/LAN gameplay validation'])
    path=E/'validation.json';path.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(new_cases=totals[0],writes=totals[1],bytes=totals[2],exercised=len(seen),validation_sha256=sha(path))))
if __name__=='__main__':main()
