#!/usr/bin/env python3
"""Authenticate the native platform/world join and its bounded original caller evidence."""
import json
import re
from freeze_physics_platform_call_20261011 import ROOT, BASE, OWNED, APPEND, PARENT_PIN, sha

E=ROOT/'analysis/multiplayer/sandbox-source/physics-platform-call-runtime-20261011'
PAIR_PIN='83b00042fa68b81a52d8ba524596294a205f8e5b7f85062a90b7f6a0a839dc48'
GENERATED_PIN='3f426e6a4d641c0dde55b5a60300e6e275ceb8059cd7daeadad68346f7202673'
FIXTURE_PIN='9c1603017a39b39583088b258ece712219323126be6fddc2cf9cc75a3f93884b'
BINDING_PIN='e275f75ed22289095ef7b67ff7dd9f09c20f1263d83d665c7ffbe6637e0954bd'


def read(p):return json.loads(p.read_text())


def main():
    parent_path=BASE/'source-manifest-v12.json';assert sha(parent_path)==PARENT_PIN
    parent=read(parent_path);manifest_path=BASE/'source-manifest-v13.json';manifest=read(manifest_path)
    assert manifest['parent_manifest_sha256']==PARENT_PIN and manifest['owned_overlays']==list(OWNED)
    for relative,pin in parent['files'].items():
        assert sha(BASE/'source-v12'/relative)==pin,relative
        if relative not in OWNED and relative!='CMakeLists.txt':assert manifest['files'][relative]==pin,relative
    for relative,pin in manifest['files'].items():assert sha(BASE/'source-v13'/relative)==pin,relative
    for relative in OWNED:assert sha(ROOT/relative)==manifest['files'][relative],relative
    assert (BASE/'source-v13/CMakeLists.txt').read_text()==(BASE/'source-v12/CMakeLists.txt').read_text()+APPEND
    pair_path=E/'correspondence-verified/complete-report.json'
    assert sha(pair_path)==PAIR_PIN==sha(E/'correspondence-repeat/complete-report.json')
    pair=read(pair_path)
    assert pair['extension_tool_sha256']==sha(ROOT/'tools/research_physics_world_platform_runtime_20261011.py')
    assert pair['complete_bodies']==371 and sum(pair['mnemonics'].values())==30717
    old_pair=read(ROOT/'analysis/multiplayer/sandbox-source/physics-world-bootstrap-runtime-20261011/correspondence-v19/complete-report.json')
    for key in ('entries','instruction_map','address_map'):
        assert all(pair[key][p]==q for p,q in old_pair[key].items()),key
    assert set(pair['entries'])-set(old_pair['entries'])=={f'{p:08x}' for p in (0x2db3c6,0x326baf,0x326970,0x1ecf60,0x1ecfe0,0x1f1600,0x1f2710,0x1c7480,0x2e5ce0)}
    for row in pair['routines']:
        for label in ('disc','later'):
            listing=row[label+'_listing'];assert sha(pair_path.parent/'instructions'/listing['file'])==listing['sha256']
    generated_path=E/'generated-verified/manifest.json'
    assert sha(generated_path)==GENERATED_PIN==sha(E/'generated-repeat/manifest.json')
    generated=read(generated_path)
    assert generated['generator_sha256']==sha(ROOT/'tools/generate_physics_world_platform_runtime_20261011.py')
    assert generated['correspondence_sha256']==PAIR_PIN
    assert generated['generated_cpp_sha256']==sha(ROOT/'src/physics_world_runtime_generated.cpp')==sha(E/'generated-repeat/runtime.cpp')
    assert len(generated['complete_native_entries'])==371 and len(generated['lowered_instructions'])==30717
    assert not generated['production_opcode_decoder'] and not generated['production_original_code_execution']
    binding_path=E/'call-bindings-verified/report.json';assert sha(binding_path)==BINDING_PIN
    bindings=read(binding_path)
    assert bindings['tool_sha256']==sha(ROOT/'tools/research_physics_platform_call_bindings_20261011.py')
    assert bindings['correspondence_sha256']==PAIR_PIN and bindings['all_supported_import_callers_bound']
    assert bindings['header_sha256']==sha(ROOT/'src/physics_platform_call_bindings.hpp')
    assert {b['instruction'] for b in bindings['bindings']}=={0x2dacad,0x2db25e,0x2db3d2}
    fixture_path=E/'original-verified/fixture.json'
    assert sha(fixture_path)==FIXTURE_PIN==sha(E/'original-repeat/fixture.json')
    fixture=read(fixture_path)
    assert fixture['tool_sha256']==sha(ROOT/'tools/oracle_physics_world_platform_bridge_20261011.py')
    assert fixture['correspondence_sha256']==PAIR_PIN and fixture['source_caller_behavior_equal']
    assert not any(fixture[k] for k in ('native_expected_data_used','original_kernel_abi_proven','original_kernel_executed','production_checkpoint_replay','live_gameplay_proof'))
    assert len(fixture['cases'])==1 and len(fixture['declared_volatile_variations'])==6
    c=fixture['cases'][0]
    assert len(c['instructions'])==67468 and len(set(c['instructions']))==1414
    assert len(c['writes'])==5004 and len(c['platform_calls'])==4
    assert sum(len(bytes.fromhex(b['hex'])) for b in c['final_banks'])==43027
    assert c['exit_entry']==0x306f40 and not c['complete'] and not c['reference_private_byte_diagnostics']
    reference=fixture['compiled_reference_case']
    assert c['instructions']==reference['instructions']
    def writes(x):return [(w['instruction'],w['address'],w['written']) for w in x['writes']]
    assert writes(c)==writes(reference)
    assert all(v['source_behavior_equal'] for v in fixture['declared_volatile_variations'])
    predecessor=ROOT/'analysis/multiplayer/sandbox-source/physics-platform-lock-runtime-20261011/validation.json'
    assert sha(predecessor)=='afc654eea7f7db5fa1f69936a0fe1269882dadfd31e51b3c62e92766fa19815e'
    logs,binaries={},{}
    for kind,directory in (('production','build-v13'),('asan','asan-v13')):
        for stage in ('configure','build','rebuild','test'):
            p=E/f'{kind}-{stage}.log';logs[p.name]=sha(p)
        output=(E/f'{kind}-test.log').read_text()
        assert '100% tests passed out of 22' in output and 'Skipped' not in output
        assert len(re.findall(r'\boriginal_physics_world_\w+\s+\.+\s+Passed',output))==20
        for name in ('native_physics_platform_locks','native_physics_platform_construction'):
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
        original_fixture_sha256=FIXTURE_PIN,complete_paired_bodies=371,lowered_instructions=30717,
        retained_initialization_instructions=67468,exercised_distinct_instructions=1414,
        ordered_source_writes=5004,observed_state_bytes=43027,native_platform_calls=4,
        declared_volatile_variations=6,native_checks_passed=22,sanitizer_checks_passed=22,logs=logs,binaries=binaries,
        independent_source_and_fixture_repeats_identical=True,joined_public_entry_point_verified=True,
        native_preserved_void_abi_declared=True,original_kernel_abi_proven=False,full_world_initialization_complete=False,
        required_next_owner=0x306f40,live_match_integrated=False,full_tu15_startup_proven=False,
        whole_physics_engine_complete=False,published_launcher_replaced=False,
        limits=['One retained1.1 construction input and six declared volatile variations',
            'TU1.5 source functions and isolated caller bindings authenticate; full new TU1.5 construction is not executed',
            'Native process startup/heap, remaining BSP/world/body/controller ownership and live mover integration remain',
            'Other shapes/motions, vehicles/ragdolls, Xbox estimates and continuous all-map split-screen/LAN remain',
            'Private validation trees inherit original test resources; not redistributable asset-free packages'])
    path=E/'validation.json';path.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(native_checks=22,sanitizer_checks=22,validation_sha256=sha(path))))


if __name__=='__main__':main()
