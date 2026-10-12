#!/usr/bin/env python3
"""Authenticate the BSP/body construction extension and bounded original/native comparisons."""
import json
import re
from freeze_physics_bsp_construction_20261011 import ROOT, BASE, OWNED, APPEND, PARENT_PIN, sha

E=ROOT/'analysis/multiplayer/sandbox-source/physics-bsp-construction-runtime-20261011'
PAIR_PIN='567dfc57c559dc19cb510359c35e7bcccfc5f70e43e6ddeba1cd128d80544cc7'
GENERATED_PIN='4b352cc5a8c4329e4764f7a3b5f0b60e6c40f9ae4e008fdae209d8d89f1795e3'
FIXTURE_PIN='4621900b91c8e2fc779b1854236be813cd1d319afdd7fcd8457cf53f4c69c93a'
BINDING_PIN='559a08004b0f60aed29e1bbfcccc5d5175a9037f0785cdc5450776bbce3bb616'


def read(p):return json.loads(p.read_text())


def main():
    parent_path=BASE/'source-manifest-v13.json';assert sha(parent_path)==PARENT_PIN
    parent=read(parent_path);manifest_path=BASE/'source-manifest-v14.json';manifest=read(manifest_path)
    assert manifest['parent_manifest_sha256']==PARENT_PIN and manifest['owned_overlays']==list(OWNED)
    for relative,pin in parent['files'].items():
        assert sha(BASE/'source-v13'/relative)==pin,relative
        if relative not in OWNED and relative!='CMakeLists.txt':assert manifest['files'][relative]==pin,relative
    for relative,pin in manifest['files'].items():assert sha(BASE/'source-v14'/relative)==pin,relative
    for relative in OWNED:assert sha(ROOT/relative)==manifest['files'][relative],relative
    assert (BASE/'source-v14/CMakeLists.txt').read_text()==(BASE/'source-v13/CMakeLists.txt').read_text()+APPEND
    pair_path=E/'correspondence-verified/complete-report.json'
    assert sha(pair_path)==PAIR_PIN==sha(E/'correspondence-repeat/complete-report.json')
    pair=read(pair_path)
    assert pair['extension_tool_sha256']==sha(ROOT/'tools/research_physics_bsp_construction_runtime_20261011.py')
    assert pair['complete_bodies']==398 and sum(pair['mnemonics'].values())==31930
    old_pair_path=ROOT/'analysis/multiplayer/sandbox-source/physics-platform-call-runtime-20261011/correspondence-verified/complete-report.json'
    assert sha(old_pair_path)=='83b00042fa68b81a52d8ba524596294a205f8e5b7f85062a90b7f6a0a839dc48'
    old_pair=read(old_pair_path)
    for key in ('entries','instruction_map','address_map'):
        assert all(pair[key][p]==q for p,q in old_pair[key].items()),key
    assert set(pair['entries'])-set(old_pair['entries'])=={f'{p:08x}' for p in (0x2e7d80,0x2e8260,0x2eb4c0,0x306f40,0x3121a0,0x3121c0,0x312410,0x312a90,0x317890,0x317a30,0x317d10,0x317df0,0x317e80,0x317ea0,0x318100,0x318a20,0x318af0,0x31c110,0x31c1a0,0x31f620,0x31f970,0x31fd40,0x320cc0,0x329233,0x3292f1,0x32933b,0x379030)}
    assert pair['parents']['actual_bsp_array_element_initializer']==dict(
        constructor=0x2e8260,old_push=0x2e82ad,later_push=0x2ebd5d,
        old_entry=0x2e7d80,later_entry=0x2eb830,array_constructor=0x3292f1,
        actual_indirect_consumer=0x329315)
    for row in pair['routines']:
        for label in ('disc','later'):
            listing=row[label+'_listing'];assert sha(pair_path.parent/'instructions'/listing['file'])==listing['sha256']
    generated_path=E/'generated-verified/manifest.json'
    assert sha(generated_path)==GENERATED_PIN==sha(E/'generated-repeat/manifest.json')
    generated=read(generated_path)
    assert generated['generator_sha256']==sha(ROOT/'tools/generate_physics_bsp_construction_runtime_20261011.py')
    assert generated['correspondence_sha256']==PAIR_PIN
    assert generated['generated_cpp_sha256']==sha(ROOT/'src/physics_world_runtime_generated.cpp')==sha(E/'generated-repeat/runtime.cpp')
    assert len(generated['complete_native_entries'])==398 and len(generated['lowered_instructions'])==31930
    assert not generated['production_opcode_decoder'] and not generated['production_original_code_execution']
    binding_path=E/'call-bindings-verified/report.json';assert sha(binding_path)==BINDING_PIN
    assert sha(binding_path)==sha(E/'call-bindings-repeat/report.json')
    bindings=read(binding_path)
    assert bindings['tool_sha256']==sha(ROOT/'tools/research_physics_bsp_platform_bindings_20261011.py')
    assert bindings['correspondence_sha256']==PAIR_PIN and bindings['all_supported_import_callers_bound']
    assert bindings['header_sha256']==sha(ROOT/'src/physics_platform_call_bindings.hpp')
    assert {b['instruction'] for b in bindings['bindings']}=={0x2dacad,0x2db25e,0x2db3d2}
    fixture_path=E/'original-verified/fixture.json'
    assert sha(fixture_path)==FIXTURE_PIN==sha(E/'original-repeat/fixture.json')
    fixture=read(fixture_path)
    assert fixture['tool_sha256']==sha(ROOT/'tools/oracle_physics_bsp_construction_runtime_20261011.py')
    assert fixture['correspondence_sha256']==PAIR_PIN and fixture['source_caller_behavior_equal']
    assert not any(fixture[k] for k in ('native_expected_data_used','original_kernel_abi_proven','original_kernel_executed','production_checkpoint_replay','live_gameplay_proof'))
    assert len(fixture['cases'])==1 and len(fixture['declared_volatile_variations'])==6
    c=fixture['cases'][0]
    assert len(c['instructions'])==76992 and len(set(c['instructions']))==2706
    assert len(c['writes'])==7016 and len(c['platform_calls'])==4
    assert sum(len(bytes.fromhex(b['hex'])) for b in c['final_banks'])==44962
    assert c['exit_entry']==0x2ec520 and not c['complete'] and not c['reference_private_byte_diagnostics']
    reference=fixture['compiled_reference_case']
    assert c['instructions']==reference['instructions']
    def writes(x):return [(w['instruction'],w['address'],w['written']) for w in x['writes']]
    assert writes(c)==writes(reference)
    assert all(v['source_behavior_equal'] for v in fixture['declared_volatile_variations'])
    predecessor=ROOT/'analysis/multiplayer/sandbox-source/physics-platform-call-runtime-20261011/validation.json'
    assert sha(predecessor)=='7f00d305bc141122600ddb492327908da562acfec1d76b9027207ebd05123b02'
    logs,binaries={},{}
    for name in ('native-preflight-build.log','native-preflight-test.log','native-preflight-predecessor-test.log'):
        logs[name]=sha(E/name)
    assert '76992 instructions, 7016 ordered writes, 44962 full-state bytes' in (E/'native-preflight-test.log').read_text()
    assert '67468 instructions, 5004 ordered writes, 43027 full-state bytes, retained earlier prefix boundary' in (E/'native-preflight-predecessor-test.log').read_text()
    binaries['preflight']=sha(E/'native-preflight-tests')
    for kind,directory in (('production','build-v14'),('asan','asan-v14')):
        for stage in ('configure','build','rebuild','test'):
            p=E/f'{kind}-{stage}.log';logs[p.name]=sha(p)
        output=(E/f'{kind}-test.log').read_text()
        assert '100% tests passed out of 23' in output and 'Skipped' not in output
        assert len(re.findall(r'\boriginal_physics_world_\w+\s+\.+\s+Passed',output))==20
        for name in ('native_physics_platform_locks','native_physics_platform_construction','native_physics_bsp_construction'):
            assert re.search(r'\b'+name+r'\s+\.+\s+Passed',output)
        for name in ('physics_world_runtime_tests','physics_platform_locks_tests','physics_platform_call_tests'):
            binaries[kind+'/'+name]=sha(BASE/directory/name)
        build=(BASE/directory/'build.ninja').read_text()
        block=build.split('CMakeFiles/halo2_physics_world_runtime.dir/src/physics_world_runtime_generated.cpp.o:')[1].split('\n\n')[0]
        assert all(flag in block for flag in ('-fno-fast-math','-ffp-contract=off','-frounding-math'))
        if kind=='asan':assert '-fsanitize=address,undefined' in block and '-O0' in block
    result=dict(schema=1,validator_sha256=sha(__file__),source_manifest_sha256=sha(manifest_path),
        parent_manifest_sha256=PARENT_PIN,unchanged_parent_validation_sha256=sha(predecessor),
        correspondence_sha256=PAIR_PIN,generated_manifest_sha256=GENERATED_PIN,call_bindings_sha256=BINDING_PIN,
        original_fixture_sha256=FIXTURE_PIN,complete_paired_bodies=398,lowered_instructions=31930,
        retained_initialization_instructions=76992,exercised_distinct_instructions=2706,
        ordered_source_writes=7016,observed_state_bytes=44962,native_platform_calls=4,
        declared_volatile_variations=6,native_checks_passed=23,sanitizer_checks_passed=23,logs=logs,binaries=binaries,
        independent_source_and_fixture_repeats_identical=True,joined_public_entry_point_verified=True,
        native_preserved_void_abi_declared=True,original_kernel_abi_proven=False,full_world_initialization_complete=False,
        required_next_owner=0x2ec520,live_match_integrated=False,full_tu15_startup_proven=False,
        whole_physics_engine_complete=False,published_launcher_replaced=False,
        limits=['One retained1.1 construction input and six declared volatile variations',
            'TU1.5 source functions and isolated caller bindings authenticate; full new TU1.5 construction is not executed',
            'Native process startup/heap, remaining BSP/world/body/controller ownership and live mover integration remain',
            'Other shapes/motions, vehicles/ragdolls, Xbox estimates and continuous all-map split-screen/LAN remain',
            'Private validation trees inherit original test resources; not redistributable asset-free packages'])
    path=E/'validation.json';path.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(native_checks=23,sanitizer_checks=23,validation_sha256=sha(path))))


if __name__=='__main__':main()
