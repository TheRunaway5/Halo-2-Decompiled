#!/usr/bin/env python3
"""Seal the retained world initialization callback/iterator cohort and its limits."""
import json
import re
from freeze_physics_world_game_callback_20261011 import ROOT, BASE, OWNED, APPEND, PARENT_PIN, sha

E=ROOT/'analysis/multiplayer/sandbox-source/physics-world-game-callback-runtime-20261011'
PREVIOUS=ROOT/'analysis/multiplayer/sandbox-source/physics-world-manager-runtime-20261011'
PAIR_PIN='e4b0a21a7859c727ba3374222465e9f425c63ba70639b13dff13eca441e2ad43'
GENERATED_PIN='39b27f834d8bb5c17777cb7cd751d2937440767f73714cf783d85b85bfd5a268'
FIXTURE_PIN='ee644a994c596febaf5982a3e9ca21834381d200ab5db6202467c0060f4ec18e'
ADDED={f'{p:08x}' for p in (0x27f190,0xbbda0,0x1c89f0)}


def read(path):return json.loads(path.read_text())


def main():
    parent_path=BASE/'source-manifest-v17.json';assert sha(parent_path)==PARENT_PIN
    parent=read(parent_path);manifest_path=BASE/'source-manifest-v18.json';manifest=read(manifest_path)
    assert manifest['parent_manifest_sha256']==PARENT_PIN and manifest['owned_overlays']==list(OWNED)
    for relative,pin in parent['files'].items():
        assert sha(BASE/'source-v17'/relative)==pin,relative
        if relative not in OWNED and relative!='CMakeLists.txt':assert manifest['files'][relative]==pin,relative
    for relative,pin in manifest['files'].items():assert sha(BASE/'source-v18'/relative)==pin,relative
    for relative in OWNED:assert sha(ROOT/relative)==manifest['files'][relative],relative
    assert (BASE/'source-v18/CMakeLists.txt').read_text()==(BASE/'source-v17/CMakeLists.txt').read_text()+APPEND
    pair_path=E/'correspondence-verified/complete-report.json'
    assert sha(pair_path)==PAIR_PIN==sha(E/'correspondence-repeat/complete-report.json')
    pair=read(pair_path)
    assert pair['extension_tool_sha256']==sha(ROOT/'tools/research_physics_world_game_callback_runtime_20261011.py')
    assert pair['complete_bodies']==460 and sum(pair['mnemonics'].values())==34769
    old_pair_path=PREVIOUS/'correspondence-verified/complete-report.json'
    assert sha(old_pair_path)=='716fdc8c7f819a65b04aab04cdc1610035b9cda808b0a1429a78a284c093f2f9'
    old_pair=read(old_pair_path)
    for key in ('entries','instruction_map','address_map'):assert all(pair[key][p]==q for p,q in old_pair[key].items()),key
    assert set(pair['entries'])-set(old_pair['entries'])==ADDED
    for row in pair['routines']:
        assert row['complete_paired_body']
        for label in ('disc','later'):
            listing=row[label+'_listing'];assert sha(pair_path.parent/'instructions'/listing['file'])==listing['sha256']
    generated_path=E/'generated-verified/manifest.json'
    assert sha(generated_path)==GENERATED_PIN==sha(E/'generated-repeat/manifest.json')
    generated=read(generated_path)
    assert generated['generator_sha256']==sha(ROOT/'tools/generate_physics_world_game_callback_runtime_20261011.py')
    assert generated['correspondence_sha256']==PAIR_PIN
    assert generated['generated_cpp_sha256']==sha(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp')==sha(E/'generated-repeat/runtime.cpp')
    assert len(generated['complete_native_entries'])==460 and len(generated['lowered_instructions'])==34769
    assert not generated['production_opcode_decoder'] and not generated['production_original_code_execution']
    fixture_path=E/'original-verified/fixture.json'
    assert sha(fixture_path)==FIXTURE_PIN==sha(E/'original-repeat/fixture.json')
    fixture=read(fixture_path)
    assert fixture['tool_sha256']==sha(ROOT/'tools/oracle_physics_world_game_callback_runtime_20261011.py')
    assert fixture['correspondence_sha256']==PAIR_PIN and fixture['source_caller_behavior_equal']
    assert len(fixture['cases'])==1 and len(fixture['declared_volatile_variations'])==6
    assert not any(fixture[k] for k in ('native_expected_data_used','original_kernel_abi_proven','original_kernel_executed','production_checkpoint_replay','live_gameplay_proof'))
    c=fixture['cases'][0]
    assert len(c['instructions'])==282297 and len(set(c['instructions']))==6655
    assert {int(p,16) for p in ADDED}<=set(c['instructions']) and 0x1c6c2c in c['instructions']
    assert c['instructions'][-1]==0x1c60fd and c['exit_entry']==c['return_address']==0x3000000 and c['complete']
    assert len(c['writes'])==46066 and len(c['platform_calls'])==6
    assert sum(len(b['hex'])//2 for b in c['final_banks'])==70346
    assert not c['reference_private_byte_diagnostics'] and not c['estimates'] and not c['timestamps']
    assert any(w['instruction']==0x1c6867 and w['address']==0x526234 and w['written']=='70540006' for w in c['writes'])
    reference=fixture['compiled_reference_case'];assert c['instructions']==reference['instructions']
    assert [(w['instruction'],w['address'],w['written']) for w in c['writes']]==[(w['instruction'],w['address'],w['written']) for w in reference['writes']]
    assert all(v['source_behavior_equal'] for v in fixture['declared_volatile_variations'])
    predecessor=PREVIOUS/'validation.json'
    assert sha(predecessor)=='2c98793c19da6e8c86539f58ee610d3693039b19b5e94a497d9d1f728844e503'
    logs,binaries={},{}
    for name in ('native-v3-build.log','native-v3-link.log','native-v3-test.log'):
        logs[name]=sha(E/name)
    assert '282297 instructions, 46066 ordered writes, 70346 full-state bytes' in (E/'native-v3-test.log').read_text()
    binaries['preflight']=sha(E/'native-v3-tests')
    for kind,directory in (('production','build-v18'),('asan','asan-v18')):
        for stage in ('configure','build','rebuild','test'):
            path=E/f'{kind}-{stage}.log';logs[path.name]=sha(path)
        output=(E/f'{kind}-test.log').read_text()
        assert '100% tests passed out of 28' in output and 'Skipped' not in output and 'Not Run' not in output
        assert len(re.findall(r'\boriginal_physics_world_\w+\s+\.+\s+Passed',output))==21
        for name in ('native_physics_platform_locks','native_physics_platform_construction','native_physics_bsp_construction','native_physics_world_construction','native_physics_world_setup_listener','native_physics_world_manager','native_physics_world_initialization'):
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
        complete_paired_bodies=460,lowered_instructions=34769,added_bodies=3,all_added_entries_executed=True,
        retained_initialization_instructions=282297,exercised_distinct_instructions=6655,ordered_source_writes=46066,
        observed_state_bytes=70346,native_platform_calls=6,declared_volatile_variations=6,native_checks_passed=28,sanitizer_checks_passed=28,logs=logs,binaries=binaries,
        independent_source_and_fixture_repeats_identical=True,joined_public_entry_point_verified=True,core_world_constructor_returned=True,agent_registration_returned=True,world_setup_returned=True,
        native_preserved_void_abi_declared=True,original_kernel_abi_proven=False,retained_world_initialization_complete=True,fresh_native_world_initialization_proven=False,
        required_next_owner=None,live_match_integrated=False,full_tu15_startup_proven=False,whole_physics_engine_complete=False,published_launcher_replaced=False,
        limits=['One retained1.1 construction input and six declared volatile variations',
            'Complete paired TU1.5 bodies and relocations; full TU1.5 construction is not executed',
            'Agent registration does not establish every installed agent callback behavior',
            'Retained game callback and iterator inputs do not establish populated dynamic-object paths or alternative game-unit callees',
            'Fresh startup/heap, remaining constructors/body/controller owners and live mover integration remain',
            'Other shapes/motions, vehicles/ragdolls, Xbox CPU estimates and continuous all-map split-screen/LAN remain',
            'Private validation trees inherit original resources; not redistributable asset-free packages'])
    path=E/'validation.json';path.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(native_checks=28,sanitizer_checks=28,validation_sha256=sha(path))))


if __name__=='__main__':main()
