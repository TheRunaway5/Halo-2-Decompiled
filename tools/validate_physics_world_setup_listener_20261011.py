#!/usr/bin/env python3
"""Seal the native world setup listener/registration cohort and its limits."""
import json
import re
from freeze_physics_world_setup_listener_20261011 import ROOT, BASE, OWNED, APPEND, PARENT_PIN, sha

E=ROOT/'analysis/multiplayer/sandbox-source/physics-world-setup-listener-runtime-20261011'
PREVIOUS=ROOT/'analysis/multiplayer/sandbox-source/physics-world-construction-runtime-20261011'
PAIR_PIN='9f1c944ba868d5940e4d6ba2ed68cc3e77a42930362804e911b3bcf968394ec0'
GENERATED_PIN='5873701add5aafbe441089b385c1efdd5e03c5d79c8f6eb3054efc7f836c5c9e'
FIXTURE_PIN='0a5d68e5c083df8ca8701ffcc80a3c8d9a130082b89ab4fac4a6068e000b19e7'
ADDED={f'{p:08x}' for p in (
    0x27eca0,0x2e1470,0x2ebf60,0x2ebfe0,0x2ec230,0x2ec2a0,0x2ec3c0,
    0x2ec6e0,0x2ed260,0x2ee2a0,0x2ef5c0,0x2f1ad0,0x2f2160,0x2f31a0,
    0x2f3cc0,0x2f3f70,0x2f4800,0x2f5450,0x2f5ec0,0x2f63e0,0x2f73c0,
    0x2f8070,0x2f8dc0,0x2fa870,0x2fe000,0x2fee90,0x2ffc60,0x301f40,
    0x3129e0,0x313390,0x3137a0,0x3137d0)}


def read(path):return json.loads(path.read_text())


def main():
    parent_path=BASE/'source-manifest-v15a.json';assert sha(parent_path)==PARENT_PIN
    parent=read(parent_path);manifest_path=BASE/'source-manifest-v16.json';manifest=read(manifest_path)
    assert manifest['parent_manifest_sha256']==PARENT_PIN and manifest['owned_overlays']==list(OWNED)
    for relative,pin in parent['files'].items():
        assert sha(BASE/'source-v15a'/relative)==pin,relative
        if relative not in OWNED and relative!='CMakeLists.txt':assert manifest['files'][relative]==pin,relative
    for relative,pin in manifest['files'].items():assert sha(BASE/'source-v16'/relative)==pin,relative
    for relative in OWNED:assert sha(ROOT/relative)==manifest['files'][relative],relative
    assert (BASE/'source-v16/CMakeLists.txt').read_text()==(BASE/'source-v15a/CMakeLists.txt').read_text()+APPEND
    pair_path=E/'correspondence-verified/complete-report.json'
    assert sha(pair_path)==PAIR_PIN==sha(E/'correspondence-repeat/complete-report.json')
    pair=read(pair_path)
    assert pair['extension_tool_sha256']==sha(ROOT/'tools/research_physics_world_setup_listener_runtime_20261011.py')
    assert pair['complete_bodies']==453 and sum(pair['mnemonics'].values())==34282
    old_pair_path=PREVIOUS/'correspondence-verified/complete-report.json'
    assert sha(old_pair_path)=='82701d874fd937eeaecc15d1b3ad8d86fbbb2c783e649057f89f0589f86520f9'
    old_pair=read(old_pair_path)
    for key in ('entries','instruction_map','address_map'):assert all(pair[key][p]==q for p,q in old_pair[key].items()),key
    assert set(pair['entries'])-set(old_pair['entries'])==ADDED
    for name,constructor,store,slot,entry,consumer in (
        ('actual_game_world_contact_slot0',0x27e770,0x27e7aa,0x461bd8,0x27eca0,0x3114d1),
        ('actual_world_temporary_retirement_slot0',0x316710,0x316e1a,0x419f00,0x313390,0x312743),
        ('actual_world_pair_manager_destructor_slot0',0x316710,0x316b05,0x419f1c,0x3137a0,0x316ead)):
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
    assert generated['generator_sha256']==sha(ROOT/'tools/generate_physics_world_setup_listener_runtime_20261011.py')
    assert generated['correspondence_sha256']==PAIR_PIN
    assert generated['generated_cpp_sha256']==sha(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp')==sha(E/'generated-repeat/runtime.cpp')
    assert len(generated['complete_native_entries'])==453 and len(generated['lowered_instructions'])==34282
    assert not generated['production_opcode_decoder'] and not generated['production_original_code_execution']
    fixture_path=E/'original-verified/fixture.json'
    assert sha(fixture_path)==FIXTURE_PIN==sha(E/'original-repeat/fixture.json')
    fixture=read(fixture_path)
    assert fixture['tool_sha256']==sha(ROOT/'tools/oracle_physics_world_setup_listener_runtime_20261011.py')
    assert fixture['correspondence_sha256']==PAIR_PIN and fixture['source_caller_behavior_equal']
    assert len(fixture['cases'])==1 and len(fixture['declared_volatile_variations'])==6
    assert not any(fixture[k] for k in ('native_expected_data_used','original_kernel_abi_proven','original_kernel_executed','production_checkpoint_replay','live_gameplay_proof'))
    c=fixture['cases'][0]
    assert len(c['instructions'])==273422 and len(set(c['instructions']))==6220
    assert {int(p,16) for p in ADDED}<=set(c['instructions']) and 0x1c7475 in c['instructions']
    assert c['instructions'][-1]==0x2e137d and c['exit_entry']==0x2e1280 and not c['complete']
    assert len(c['writes'])==44309 and len(c['platform_calls'])==4
    assert sum(len(b['hex'])//2 for b in c['final_banks'])==70039
    assert not c['reference_private_byte_diagnostics'] and not c['estimates'] and not c['timestamps']
    assert any(w['instruction']==0x1c6867 and w['address']==0x526234 and w['written']=='70540006' for w in c['writes'])
    reference=fixture['compiled_reference_case'];assert c['instructions']==reference['instructions']
    assert [(w['instruction'],w['address'],w['written']) for w in c['writes']]==[(w['instruction'],w['address'],w['written']) for w in reference['writes']]
    assert all(v['source_behavior_equal'] for v in fixture['declared_volatile_variations'])
    predecessor=PREVIOUS/'validation.json'
    assert sha(predecessor)=='5706ef6d4cb98c89c62a0b91e201dd1b9aa5674ff952f0822914dd7267904659'
    logs,binaries={},{}
    for name in ('native-v7-build.log','native-v7-link.log','native-v7-test.log'):
        logs[name]=sha(E/name)
    assert '273422 instructions, 44309 ordered writes, 70039 full-state bytes' in (E/'native-v7-test.log').read_text()
    binaries['preflight']=sha(E/'native-v7-tests')
    for kind,directory in (('production','build-v16'),('asan','asan-v16')):
        for stage in ('configure','build','rebuild','test'):
            path=E/f'{kind}-{stage}.log';logs[path.name]=sha(path)
        output=(E/f'{kind}-test.log').read_text()
        assert '100% tests passed out of 26' in output and 'Skipped' not in output and 'Not Run' not in output
        assert len(re.findall(r'\boriginal_physics_world_\w+\s+\.+\s+Passed',output))==21
        for name in ('native_physics_platform_locks','native_physics_platform_construction','native_physics_bsp_construction','native_physics_world_construction','native_physics_world_setup_listener'):
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
        complete_paired_bodies=453,lowered_instructions=34282,added_bodies=32,all_added_entries_executed=True,
        retained_initialization_instructions=273422,exercised_distinct_instructions=6220,ordered_source_writes=44309,
        observed_state_bytes=70039,native_platform_calls=4,declared_volatile_variations=6,native_checks_passed=26,sanitizer_checks_passed=26,logs=logs,binaries=binaries,
        independent_source_and_fixture_repeats_identical=True,joined_public_entry_point_verified=True,core_world_constructor_returned=True,agent_registration_returned=True,
        native_preserved_void_abi_declared=True,original_kernel_abi_proven=False,full_world_initialization_complete=False,
        required_next_owner=0x2e1280,live_match_integrated=False,full_tu15_startup_proven=False,whole_physics_engine_complete=False,published_launcher_replaced=False,
        limits=['One retained1.1 construction input and six declared volatile variations',
            'Complete paired TU1.5 bodies and relocations; full TU1.5 construction is not executed',
            'Agent registration does not establish every installed agent callback behavior',
            'Retained27ECA0 non-unit branch does not exercise its alternative game-unit callees',
            'Fresh startup/heap, remaining constructors/body/controller owners and live mover integration remain',
            'Other shapes/motions, vehicles/ragdolls, Xbox CPU estimates and continuous all-map split-screen/LAN remain',
            'Private validation trees inherit original resources; not redistributable asset-free packages'])
    path=E/'validation.json';path.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(native_checks=26,sanitizer_checks=26,validation_sha256=sha(path))))


if __name__=='__main__':main()
