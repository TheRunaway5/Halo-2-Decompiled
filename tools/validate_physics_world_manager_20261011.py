#!/usr/bin/env python3
"""Seal the native world manager construction/shape cohort and its limits."""
import json
import re
from freeze_physics_world_manager_20261011 import ROOT, BASE, OWNED, APPEND, PARENT_PIN, sha

E=ROOT/'analysis/multiplayer/sandbox-source/physics-world-manager-runtime-20261011'
PREVIOUS=ROOT/'analysis/multiplayer/sandbox-source/physics-world-setup-listener-runtime-20261011'
PAIR_PIN='716fdc8c7f819a65b04aab04cdc1610035b9cda808b0a1429a78a284c093f2f9'
GENERATED_PIN='68d3a62c0f032d60bc47341dfaa7083d88fa1386ff08925569e2b8fb0d40d149'
FIXTURE_PIN='069017e910186317459e42cb3a774eba985b4ce323f97ee912c1e156938f0265'
ADDED={f'{p:08x}' for p in (0x2e1280,0x312fe0,0x302070,0x1f2a10)}


def read(path):return json.loads(path.read_text())


def main():
    parent_path=BASE/'source-manifest-v16.json';assert sha(parent_path)==PARENT_PIN
    parent=read(parent_path);manifest_path=BASE/'source-manifest-v17.json';manifest=read(manifest_path)
    assert manifest['parent_manifest_sha256']==PARENT_PIN and manifest['owned_overlays']==list(OWNED)
    for relative,pin in parent['files'].items():
        assert sha(BASE/'source-v16'/relative)==pin,relative
        if relative not in OWNED and relative!='CMakeLists.txt':assert manifest['files'][relative]==pin,relative
    for relative,pin in manifest['files'].items():assert sha(BASE/'source-v17'/relative)==pin,relative
    for relative in OWNED:assert sha(ROOT/relative)==manifest['files'][relative],relative
    assert (BASE/'source-v17/CMakeLists.txt').read_text()==(BASE/'source-v16/CMakeLists.txt').read_text()+APPEND
    pair_path=E/'correspondence-verified/complete-report.json'
    assert sha(pair_path)==PAIR_PIN==sha(E/'correspondence-repeat/complete-report.json')
    pair=read(pair_path)
    assert pair['extension_tool_sha256']==sha(ROOT/'tools/research_physics_world_manager_runtime_20261011.py')
    assert pair['complete_bodies']==457 and sum(pair['mnemonics'].values())==34354
    old_pair_path=PREVIOUS/'correspondence-verified/complete-report.json'
    assert sha(old_pair_path)=='9f1c944ba868d5940e4d6ba2ed68cc3e77a42930362804e911b3bcf968394ec0'
    old_pair=read(old_pair_path)
    for key in ('entries','instruction_map','address_map'):assert all(pair[key][p]==q for p,q in old_pair[key].items()),key
    assert set(pair['entries'])-set(old_pair['entries'])==ADDED
    for name,constructor,store,slot,entry,consumer in (
        ('actual_world_shape_wrapper_slot18',0x1c74d0,0x1c7537,0x460fc8,0x302070,0x314542),
        ('actual_world_bsp_shape_slot18',0x1f2710,0x1f2720,0x45d290,0x1f2a10,0x302075)):
        binding=pair['parents'][name]
        assert all(binding[k]==v for k,v in dict(constructor=constructor,old_store=store,old_slot=slot,old_entry=entry,actual_consumer=consumer).items())
        assert binding['later_entry']==pair['entries'][f'{entry:08x}'] and binding['later_store']==pair['instruction_map'][f'{store:08x}']
    for row in pair['routines']:
        assert row['complete_paired_body']
        for label in ('disc','later'):
            listing=row[label+'_listing'];assert sha(pair_path.parent/'instructions'/listing['file'])==listing['sha256']
    generated_path=E/'generated-verified/manifest.json'
    assert sha(generated_path)==GENERATED_PIN==sha(E/'generated-repeat/manifest.json')
    generated=read(generated_path)
    assert generated['generator_sha256']==sha(ROOT/'tools/generate_physics_world_manager_runtime_20261011.py')
    assert generated['correspondence_sha256']==PAIR_PIN
    assert generated['generated_cpp_sha256']==sha(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp')==sha(E/'generated-repeat/runtime.cpp')
    assert len(generated['complete_native_entries'])==457 and len(generated['lowered_instructions'])==34354
    assert not generated['production_opcode_decoder'] and not generated['production_original_code_execution']
    fixture_path=E/'original-verified/fixture.json'
    assert sha(fixture_path)==FIXTURE_PIN==sha(E/'original-repeat/fixture.json')
    fixture=read(fixture_path)
    assert fixture['tool_sha256']==sha(ROOT/'tools/oracle_physics_world_manager_runtime_20261011.py')
    assert fixture['correspondence_sha256']==PAIR_PIN and fixture['source_caller_behavior_equal']
    assert len(fixture['cases'])==1 and len(fixture['declared_volatile_variations'])==6
    assert not any(fixture[k] for k in ('native_expected_data_used','original_kernel_abi_proven','original_kernel_executed','production_checkpoint_replay','live_gameplay_proof'))
    c=fixture['cases'][0]
    assert len(c['instructions'])==282169 and len(set(c['instructions']))==6549
    assert {int(p,16) for p in ADDED}<=set(c['instructions']) and 0x1c6c2c in c['instructions']
    assert c['instructions'][-1]==0x1c738d and c['exit_entry']==0x27f190 and not c['complete']
    assert len(c['writes'])==46023 and len(c['platform_calls'])==6
    assert sum(len(b['hex'])//2 for b in c['final_banks'])==70309
    assert not c['reference_private_byte_diagnostics'] and not c['estimates'] and not c['timestamps']
    assert any(w['instruction']==0x1c6867 and w['address']==0x526234 and w['written']=='70540006' for w in c['writes'])
    reference=fixture['compiled_reference_case'];assert c['instructions']==reference['instructions']
    assert [(w['instruction'],w['address'],w['written']) for w in c['writes']]==[(w['instruction'],w['address'],w['written']) for w in reference['writes']]
    assert all(v['source_behavior_equal'] for v in fixture['declared_volatile_variations'])
    predecessor=PREVIOUS/'validation.json'
    assert sha(predecessor)=='0b92be2bd14a061a278319b8185eb675815ab95a71a1bbf246fa4b03d6163dfc'
    logs,binaries={},{}
    for name in ('native-v3-build.log','native-v3-link.log','native-v3-test.log'):
        logs[name]=sha(E/name)
    assert '282169 instructions, 46023 ordered writes, 70309 full-state bytes' in (E/'native-v3-test.log').read_text()
    binaries['preflight']=sha(E/'native-v3-tests')
    for kind,directory in (('production','build-v17'),('asan','asan-v17')):
        for stage in ('configure','build','rebuild','test'):
            path=E/f'{kind}-{stage}.log';logs[path.name]=sha(path)
        output=(E/f'{kind}-test.log').read_text()
        assert '100% tests passed out of 27' in output and 'Skipped' not in output and 'Not Run' not in output
        assert len(re.findall(r'\boriginal_physics_world_\w+\s+\.+\s+Passed',output))==21
        for name in ('native_physics_platform_locks','native_physics_platform_construction','native_physics_bsp_construction','native_physics_world_construction','native_physics_world_setup_listener','native_physics_world_manager'):
            assert re.search(r'\b'+name+r'\s+\.+\s+Passed',output)
        assert 'ninja: no work to do.' in (E/f'{kind}-rebuild.log').read_text()
        for name in ('physics_world_runtime_tests','physics_platform_locks_tests','physics_platform_call_tests','physics_world_mmx_tests'):
            binaries[kind+'/'+name]=sha(BASE/directory/name)
        build=(BASE/directory/'build.ninja').read_text()
        block=build.split('CMakeFiles/halo2_physics_world_runtime.dir/src/physics_world_runtime_generated.cpp.o:')[1].split('\n\n')[0]
        assert all(flag in block for flag in ('-fno-fast-math','-ffp-contract=off','-frounding-math'))
        if kind=='asan':assert '-fsanitize=address,undefined' in block and '-O0' in block
    result=dict(schema=1,validator_sha256=sha(__file__),source_manifest_sha256=sha(manifest_path),parent_manifest_sha256=PARENT_PIN,
        unchanged_parent_validation_sha256=sha(predecessor),correspondence_sha256=PAIR_PIN,generated_manifest_sha256=GENERATED_PIN,original_fixture_sha256=FIXTURE_PIN,
        complete_paired_bodies=457,lowered_instructions=34354,added_bodies=4,all_added_entries_executed=True,
        retained_initialization_instructions=282169,exercised_distinct_instructions=6549,ordered_source_writes=46023,
        observed_state_bytes=70309,native_platform_calls=6,declared_volatile_variations=6,native_checks_passed=27,sanitizer_checks_passed=27,logs=logs,binaries=binaries,
        independent_source_and_fixture_repeats_identical=True,joined_public_entry_point_verified=True,core_world_constructor_returned=True,agent_registration_returned=True,world_setup_returned=True,
        native_preserved_void_abi_declared=True,original_kernel_abi_proven=False,full_world_initialization_complete=False,
        required_next_owner=0x27f190,live_match_integrated=False,full_tu15_startup_proven=False,whole_physics_engine_complete=False,published_launcher_replaced=False,
        limits=['One retained1.1 construction input and six declared volatile variations',
            'Complete paired TU1.5 bodies and relocations; full TU1.5 construction is not executed',
            'Agent registration does not establish every installed agent callback behavior',
            'Retained27ECA0 non-unit branch does not exercise its alternative game-unit callees',
            'Fresh startup/heap, remaining constructors/body/controller owners and live mover integration remain',
            'Other shapes/motions, vehicles/ragdolls, Xbox CPU estimates and continuous all-map split-screen/LAN remain',
            'Private validation trees inherit original resources; not redistributable asset-free packages'])
    path=E/'validation.json';path.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(native_checks=27,sanitizer_checks=27,validation_sha256=sha(path))))


if __name__=='__main__':main()
