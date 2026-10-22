#!/usr/bin/env python3
"""Authenticate the native world construction/MMX cohort and its measured limits."""
import json
import re
from freeze_physics_world_construction_correction_20261011 import ROOT, BASE, OWNED, APPEND, PARENT_PIN, sha

E=ROOT/'analysis/multiplayer/sandbox-source/physics-world-construction-runtime-20261011'
PAIR_PIN='82701d874fd937eeaecc15d1b3ad8d86fbbb2c783e649057f89f0589f86520f9'
GENERATED_PIN='e99f546ae4ea84c5811b327df527fc6f3b9bad66729ea2e33ade5cef5ac12835'
FIXTURE_PIN='3ba075f3f7814e1fb48ceb9cc215c8e617c31f0594f1a07b0208c908e18bcdf0'
MMX_PIN='2698359825e1c3c404ee7bdd03dddf72502c128bd2bad56f3cfa9a6fda13ddbb'
ARCHITECTURE_PIN='d7587d46f29dc822e97eebe88d51c9f5303c88f1b4ae986ae575b976d6979f2e'


def read(p):return json.loads(p.read_text())


def main():
    parent_path=BASE/'source-manifest-v15.json';assert sha(parent_path)==PARENT_PIN
    parent=read(parent_path);manifest_path=BASE/'source-manifest-v15a.json';manifest=read(manifest_path)
    assert manifest['parent_manifest_sha256']==PARENT_PIN and manifest['owned_overlays']==list(OWNED)
    for relative,pin in parent['files'].items():
        assert sha(BASE/'source-v15'/relative)==pin,relative
        if relative not in OWNED and relative!='CMakeLists.txt':assert manifest['files'][relative]==pin,relative
    for relative,pin in manifest['files'].items():assert sha(BASE/'source-v15a'/relative)==pin,relative
    for relative in OWNED:assert sha(ROOT/relative)==manifest['files'][relative],relative
    assert (BASE/'source-v15a/CMakeLists.txt').read_text()==(BASE/'source-v15/CMakeLists.txt').read_text()+APPEND
    pair_path=E/'correspondence-verified/complete-report.json'
    assert sha(pair_path)==PAIR_PIN==sha(E/'correspondence-repeat/complete-report.json')
    pair=read(pair_path);assert pair['extension_tool_sha256']==sha(ROOT/'tools/research_physics_world_construction_runtime_20261011.py')
    assert pair['complete_bodies']==421 and sum(pair['mnemonics'].values())==33237
    old_pair_path=ROOT/'analysis/multiplayer/sandbox-source/physics-bsp-construction-runtime-20261011/correspondence-verified/complete-report.json'
    assert sha(old_pair_path)=='567dfc57c559dc19cb510359c35e7bcccfc5f70e43e6ddeba1cd128d80544cc7'
    old_pair=read(old_pair_path)
    for key in ('entries','instruction_map','address_map'):assert all(pair[key][p]==q for p,q in old_pair[key].items()),key
    added={f'{p:08x}' for p in (0x22e380,0x22e420,0x2e6240,0x2e7500,0x2e7da0,0x2e8090,0x2e8a00,0x2ebee0,0x2ebf70,0x2ec520,0x311680,0x311810,0x3119f0,0x312c60,0x313020,0x313750,0x314a00,0x318870,0x31bed0,0x31c140,0x31e6f0,0x31f130,0x379070)}
    assert set(pair['entries'])-set(old_pair['entries'])==added
    for name,constructor,store,slot,entry,consumer in (
        ('actual_constraint_setup_manager_slot4',0x2e8260,0x2e8292,0x418c44,0x2e8a00,0x314a6e),
        ('actual_world_pair_manager_slot0',0x316710,0x316b0b,0x419f18,0x313750,0x378fcf),
        ('actual_world_setup_virtual_slot18',0x3119f0,0x311a43,0x419e24,0x311680,0x314a4a),
        ('actual_world_setup_virtual_slot1c',0x3119f0,0x311a43,0x419e28,0x311810,0x31c462)):
        binding=pair['parents'][name]
        assert all(binding[k]==v for k,v in dict(constructor=constructor,old_store=store,old_slot=slot,old_entry=entry,actual_consumer=consumer).items())
        assert binding['later_entry']==pair['entries'][f'{entry:08x}'] and binding['later_store']==pair['instruction_map'][f'{store:08x}']
    for row in pair['routines']:
        assert row['complete_paired_body']
        for label in ('disc','later'):
            listing=row[label+'_listing'];assert sha(pair_path.parent/'instructions'/listing['file'])==listing['sha256']
    generated_path=E/'generated-verified/manifest.json';assert sha(generated_path)==GENERATED_PIN==sha(E/'generated-repeat/manifest.json')
    generated=read(generated_path)
    assert generated['generator_sha256']==sha(ROOT/'tools/generate_physics_world_construction_runtime_20261011.py') and generated['correspondence_sha256']==PAIR_PIN
    assert generated['generated_cpp_sha256']==sha(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp')==sha(E/'generated-repeat/runtime.cpp')
    assert len(generated['complete_native_entries'])==421 and len(generated['lowered_instructions'])==33237
    assert not generated['production_opcode_decoder'] and not generated['production_original_code_execution']
    fixture_path=E/'original-verified/fixture.json';assert sha(fixture_path)==FIXTURE_PIN==sha(E/'original-repeat/fixture.json')
    fixture=read(fixture_path)
    assert fixture['tool_sha256']==sha(ROOT/'tools/oracle_physics_world_construction_runtime_20261011.py') and fixture['correspondence_sha256']==PAIR_PIN
    assert fixture['source_caller_behavior_equal'] and len(fixture['cases'])==1 and len(fixture['declared_volatile_variations'])==6
    assert not any(fixture[k] for k in ('native_expected_data_used','original_kernel_abi_proven','original_kernel_executed','production_checkpoint_replay','live_gameplay_proof'))
    c=fixture['cases'][0]
    assert len(c['instructions'])==116677 and len(set(c['instructions']))==4597 and c['instructions'][-1]==0x3114d1
    assert len(c['writes'])==17744 and len(c['platform_calls'])==4 and sum(len(b['hex'])//2 for b in c['final_banks'])==64689
    assert c['exit_entry']==0x27eca0 and not c['complete'] and not c['reference_private_byte_diagnostics'] and not c['estimates'] and not c['timestamps']
    assert any(w['instruction']==0x1c6867 and w['address']==0x526234 and w['written']=='70540006' for w in c['writes'])
    assert 0x2e62c0 in c['instructions'] and 0x2e6331 in c['instructions']
    reference=fixture['compiled_reference_case'];assert c['instructions']==reference['instructions']
    assert [(w['instruction'],w['address'],w['written']) for w in c['writes']]==[(w['instruction'],w['address'],w['written']) for w in reference['writes']]
    assert all(v['source_behavior_equal'] for v in fixture['declared_volatile_variations'])
    mmx_path=E/'mmx-original-verified/fixture.json';assert sha(mmx_path)==MMX_PIN==sha(E/'mmx-original-repeat/fixture.json')
    mmx=read(mmx_path);assert mmx['tool_sha256']==sha(ROOT/'tools/oracle_physics_world_mmx_20261011.py') and mmx['correspondence_sha256']==PAIR_PIN
    assert len(mmx['cases'])==240 and sum(not q['complete'] for q in mmx['cases'])==48
    assert sum(len(q['instructions']) for q in mmx['cases'])==25248 and sum(len(q['writes']) for q in mmx['cases'])==5232
    assert {q['image'] for q in mmx['cases']}=={'1.1','1.5'} and mmx['paired_image_results_equal']
    assert mmx['active_alias_upper_words_and_classified_tags_are_emulator_diagnostics'] and mmx['active_mmx_payload_comparison_bits']==64
    assert not mmx['live_gameplay_proof'] and not mmx['original_xbox_cpu_executed']
    architecture_path=E/'mmx-architecture/sources.json';assert sha(architecture_path)==ARCHITECTURE_PIN
    for name,record in read(architecture_path).items():assert sha(architecture_path.parent/name)==record['sha256']
    predecessor=ROOT/'analysis/multiplayer/sandbox-source/physics-bsp-construction-runtime-20261011/validation.json'
    assert sha(predecessor)=='e4a6e97a88b2c435d763fe0dec189e03feb47de50b8bf2182bb9afa0eaf15c28'
    logs,binaries={},{}
    for name in ('native-preflight-build.log','native-preflight-link.log','native-preflight-test.log','native-preflight-predecessor-test.log','mmx-preflight-final-link.log','mmx-preflight-final-test.log'):logs[name]=sha(E/name)
    assert '116677 instructions, 17744 ordered writes, 64689 full-state bytes' in (E/'native-preflight-test.log').read_text()
    assert '76992 instructions, 7016 ordered writes, 44962 full-state bytes, retained earlier prefix boundary' in (E/'native-preflight-predecessor-test.log').read_text()
    assert '240 cases, 48 active physical-alias prefixes, 25248 instructions, 5232 ordered writes' in (E/'mmx-preflight-final-test.log').read_text()
    binaries['preflight']=sha(E/'native-preflight-tests');binaries['mmx_preflight']=sha(E/'mmx-preflight-final-tests')
    for kind,directory in (('production','build-v15'),('asan','asan-v15')):
        for stage in ('configure','build','rebuild','test'):
            p=E/f'{kind}-{stage}.log';logs[p.name]=sha(p)
        output=(E/f'{kind}-test.log').read_text();assert '100% tests passed out of 25' in output and 'Skipped' not in output
        assert len(re.findall(r'\boriginal_physics_world_\w+\s+\.+\s+Passed',output))==21
        for name in ('native_physics_platform_locks','native_physics_platform_construction','native_physics_bsp_construction','native_physics_world_construction'):
            assert re.search(r'\b'+name+r'\s+\.+\s+Passed',output)
        for name in ('physics_world_runtime_tests','physics_platform_locks_tests','physics_platform_call_tests','physics_world_mmx_tests'):binaries[kind+'/'+name]=sha(BASE/directory/name)
        build=(BASE/directory/'build.ninja').read_text();block=build.split('CMakeFiles/halo2_physics_world_runtime.dir/src/physics_world_runtime_generated.cpp.o:')[1].split('\n\n')[0]
        assert all(flag in block for flag in ('-fno-fast-math','-ffp-contract=off','-frounding-math'))
        if kind=='asan':assert '-fsanitize=address,undefined' in block and '-O0' in block
    assert manifest['code_tests_headers_cmake_unchanged'] and manifest['compiled_source_manifest_sha256']==PARENT_PIN
    result=dict(schema=1,compiled_source_manifest_sha256=PARENT_PIN,metadata_only_revision=True,core_pointer_store_instruction=0x1c6867,validator_sha256=sha(__file__),source_manifest_sha256=sha(manifest_path),parent_manifest_sha256=PARENT_PIN,
        unchanged_parent_validation_sha256=sha(predecessor),correspondence_sha256=PAIR_PIN,generated_manifest_sha256=GENERATED_PIN,original_fixture_sha256=FIXTURE_PIN,
        mmx_fixture_sha256=MMX_PIN,mmx_architecture_sources_sha256=ARCHITECTURE_PIN,complete_paired_bodies=421,lowered_instructions=33237,
        retained_initialization_instructions=116677,exercised_distinct_instructions=4597,ordered_source_writes=17744,observed_state_bytes=64689,native_platform_calls=4,
        mmx_component_cases=240,mmx_active_alias_prefixes=48,mmx_component_instructions=25248,mmx_component_ordered_writes=5232,
        declared_volatile_variations=6,native_checks_passed=25,sanitizer_checks_passed=25,logs=logs,binaries=binaries,
        independent_source_and_fixture_repeats_identical=True,joined_public_entry_point_verified=True,core_world_constructor_returned=True,
        native_preserved_void_abi_declared=True,original_kernel_abi_proven=False,full_world_initialization_complete=False,
        required_next_owner=0x27eca0,live_match_integrated=False,full_tu15_startup_proven=False,whole_physics_engine_complete=False,published_launcher_replaced=False,
        limits=['One retained1.1 construction input and six declared volatile variations',
            'TU1.5 complete source bodies and isolated MMX inputs authenticate; full TU1.5 construction is not executed',
            'Unicorn physical upper words and classified tags are diagnostics; Intel SDM and a separate host probe establish those native semantics',
            'Fresh startup/heap ownership, remaining game callbacks/body/controller owners and live mover integration remain',
            'Other shapes/motions, vehicles/ragdolls, Xbox CPU estimates and continuous all-map split-screen/LAN remain',
            'Private validation trees inherit original resources; not redistributable asset-free packages'])
    path=E/'validation.json';path.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(native_checks=25,sanitizer_checks=25,validation_sha256=sha(path))))


if __name__=='__main__':main()
