#!/usr/bin/env python3
"""Seal the original outer game-frame component and explicit remaining limits."""
import json
import re
from freeze_physics_game_frame_runtime_20261011 import ROOT, BASE, OWNED, APPEND, PARENT_PIN, sha
from freeze_physics_game_frame_audit_correction_20261011 import CORRECTIONS, PARENT_PIN as BUILD_MANIFEST_PIN

E = ROOT / 'analysis/multiplayer/sandbox-source/physics-game-frame-runtime-20261011'
PREVIOUS = ROOT / 'analysis/multiplayer/sandbox-source/physics-world-game-callback-runtime-20261011'
PAIR_PIN = '92379da9dc9d29baea9eaca194cebdab7d69f8be319aecf6cd0b759e26e744cc'
GENERATED_PIN = 'b599340b08c743d88ba832f5fd9b6fcde8cea18910981c0bb2793fa3057b5d7b'
ORIGINAL_PIN = '1960794b2ba2c03fe4da9911914cc28db881f5060b7788b94999f94bfd1a2219'
FRAME_PIN = '0a549dd8a9cd2fa1fc9f352388615c559c16eb9c5d2b780faf97f340a842b13f'
PREFIX_PIN = 'c420cabcbc27a33a77f3925372da75c2b40b7be63bc1cebb74ddd042cb8e5488'
FP_PIN = '9c75ab30c656ec5675abbf4bd9290ebaeb0f8e5b481cf693ee51914d9312aa68'
ADDED = {f'{p:08x}' for p in (0x1c7660,0x2ef630,0x148360,0x148740,0x22ecb0,
    0x327866,0x3276fc,0x32778e,0x327855,0x32781c,0x1c7b90,0x27fa50,0x1c8350,
    0xbc910,0xb8250,0xbd6a0,0xbbe70)}


def read(path):
    return json.loads(path.read_text())


def main():
    parent_path = BASE / 'source-manifest-v18.json'
    assert sha(parent_path) == PARENT_PIN
    parent = read(parent_path)
    build_manifest_path = BASE / 'source-manifest-v19.json'
    assert sha(build_manifest_path) == BUILD_MANIFEST_PIN
    build_manifest = read(build_manifest_path)
    assert build_manifest['parent_manifest_sha256'] == PARENT_PIN and build_manifest['owned_overlays'] == list(OWNED)
    manifest_path = BASE / 'source-manifest-v19a.json'
    manifest = read(manifest_path)
    assert manifest['parent_manifest_sha256'] == BUILD_MANIFEST_PIN and manifest['metadata_overlays'] == list(CORRECTIONS)
    assert manifest['compiled_inputs_unchanged'] and manifest['build_source_manifest_sha256'] == BUILD_MANIFEST_PIN
    for relative, pin in parent['files'].items():
        assert sha(BASE / 'source-v18' / relative) == pin, relative
        if relative not in OWNED and relative != 'CMakeLists.txt':
            assert build_manifest['files'][relative] == pin, relative
    for relative, pin in build_manifest['files'].items():
        assert sha(BASE / 'source-v19' / relative) == pin, relative
        if relative not in CORRECTIONS:
            assert manifest['files'][relative] == pin, relative
    for relative, pin in manifest['files'].items():
        assert sha(BASE / 'source-v19a' / relative) == pin, relative
    for relative in set(OWNED) | set(CORRECTIONS):
        assert sha(ROOT / relative) == manifest['files'][relative], relative
    assert (BASE / 'source-v19/CMakeLists.txt').read_text() == (BASE / 'source-v18/CMakeLists.txt').read_text() + APPEND
    pair_path = E / 'correspondence-v2/complete-report.json'
    assert sha(pair_path) == PAIR_PIN == sha(E / 'correspondence-repeat/complete-report.json')
    pair = read(pair_path)
    assert pair['extension_tool_sha256'] == sha(ROOT / 'tools/research_physics_game_frame_runtime_20261011.py')
    assert pair['complete_bodies'] == 477 and sum(pair['mnemonics'].values()) == 35694
    old_pair_path = PREVIOUS / 'correspondence-verified/complete-report.json'
    assert sha(old_pair_path) == 'e4b0a21a7859c727ba3374222465e9f425c63ba70639b13dff13eca441e2ad43'
    old_pair = read(old_pair_path)
    for key in ('entries', 'instruction_map', 'address_map'):
        assert all(pair[key][p] == q for p, q in old_pair[key].items()), key
    assert set(pair['entries']) - set(old_pair['entries']) == ADDED
    assert pair['entries']['001c7660'] == 0x1cc8d0
    for row in pair['routines']:
        assert row['complete_paired_body']
        for label in ('disc', 'later'):
            listing = row[label + '_listing']
            assert sha(pair_path.parent / 'instructions' / listing['file']) == listing['sha256']
    generated_path = E / 'generated-v2/manifest.json'
    assert sha(generated_path) == GENERATED_PIN == sha(E / 'generated-repeat/manifest.json')
    generated = read(generated_path)
    assert generated['generator_sha256'] == sha(ROOT / 'tools/generate_physics_game_frame_runtime_20261011.py')
    assert generated['correspondence_sha256'] == PAIR_PIN
    assert generated['generated_cpp_sha256'] == sha(ROOT / 'src/physics_world_runtime_generated.cpp') == sha(E / 'generated-repeat/runtime.cpp')
    assert len(generated['complete_native_entries']) == 477 and len(generated['lowered_instructions']) == 35694
    assert not generated['production_opcode_decoder'] and not generated['production_original_code_execution']
    assert sha(E / 'capture-v1/checkpoint.json') == '6a64d10d3ac19526287985c2495d9c7262e8a5a881392b529eeb2f9aa74f230e'
    checkpoint = read(E / 'capture-v1/checkpoint.json')
    assert checkpoint['tool_sha256'] == sha(ROOT / 'tools/capture_physics_game_frame_20261011.py')
    original_path = E / 'original-v3/fixture.json'
    assert sha(original_path) == ORIGINAL_PIN == sha(E / 'original-repeat/fixture.json')
    original = read(original_path)
    assert original['tool_sha256'] == sha(ROOT / 'tools/oracle_physics_game_frame_checkpoint_20261011.py')
    assert original['previous_original_prefix_identical'] and original['original_prefix_stopped_before_cpu_observation']
    assert not original['native_expected_data_used'] and not original['production_checkpoint_replay']
    assert not original['source_instruction_patches'] and not original['live_gameplay_proof']
    assert original['cases'][0]['complete'] and len(original['cases'][0]['instructions']) == 24940
    assert sha(E / 'fp-variations-input-v3.json') == '02c11a713dbf3fa586e2eecd31c6cd742974a13300b0b70f9df3418a00c6f96f' == sha(E / 'fp-variations-input-repeat.json')
    hardware = {}
    for name, pin, counts in (
        ('prefix', PREFIX_PIN, (1,9166,1738,4248,9165)),
        ('frame', FRAME_PIN, (1,24941,5080,7452,24936)),
        ('fp', FP_PIN, (25,623879,127024,288575,623754))):
        path = E / f'hardware-{name}-verified/fixture.json'
        assert sha(path) == pin == sha(E / f'hardware-{name}-repeat/fixture.json')
        j = read(path)
        assert j['oracle_backend'] == 'original_i386_hardware_single_step'
        assert j['hardware_tool_sha256'] == sha(ROOT / 'tools/oracle_physics_game_frame_hardware_20261011.py')
        assert j['trace_observer_sha256'] == sha(ROOT / 'tools/physics_game_frame_hardware_trace_20261011.cpp')
        assert j['bridge_input_tool_sha256'] == sha(ROOT / 'tools/original_i386_game_frame_bridge_20261011.py')
        assert not j['original_instruction_patches'] and not j['native_expected_data_used'] and not j['xbox_hardware_proof']
        assert j['unretained_data_read_policy'] == 'reject'
        cs = j['cases']
        measured = (len(cs), sum(len(c['instructions']) for c in cs), sum(len(c['writes']) for c in cs),
            sum(sum(len(b['hex'])//2 for b in c['final_banks']) for c in cs), j['single_stepped_original_instructions'])
        assert measured == counts, (name, measured)
        assert all(c['entry'] == 0x1c7660 and c['entry_abi']['fs_base'] == checkpoint['captured_entry']['entry_abi']['fs_base'] for c in cs)
        assert all(not c['timestamps'] for c in cs)
        if name == 'prefix':
            assert not cs[0]['complete'] and cs[0]['exit_entry'] == 0x30bb22 and not cs[0]['estimates']
        else:
            assert all(c['complete'] and c['exit_entry'] == c['return_address'] == 0x3000000 for c in cs)
            assert all(len(c['packets']) == 2 and len(c['estimates']) == 3 for c in cs)
        hardware[name] = j
    baseline = hardware['frame']['cases'][0]
    assert baseline['entry_abi'] == original['cases'][0]['entry_abi']
    assert baseline['initial_banks'] == original['cases'][0]['initial_banks']
    assert baseline['estimates'] != original['cases'][0]['estimates']
    # This source STMXCSR store is observable: do not hide emulator precision loss.
    def mxcsr_stores(c):
        return [w['written'] for w in c['writes'] if w['instruction'] == 0x1c76ed]
    assert mxcsr_stores(baseline) == ['a01f0000'] and mxcsr_stores(original['cases'][0]) == ['801f0000']
    reached = set(i for c in hardware['fp']['cases'] for i in c['instructions'])
    reached_entries = sorted(p for p in ADDED if int(p,16) in reached)
    assert set(ADDED) - set(reached_entries) == {'000bbe70'}
    for c in hardware['fp']['cases']:
        assert c['declared_additional_source_input']['checkpoint_sha256'] == sha(E / 'capture-v1/checkpoint.json')
        assert c['declared_additional_source_input']['address'] == 0x82d88000 and c['declared_additional_source_input']['size'] == 4096
        if c['id']:
            v = c['declared_abi_variation']
            assert not v['natural_capture']
            assert (c['entry_abi']['mxcsr'] & ~0x3f) == (baseline['entry_abi']['mxcsr'] & ~0x3f)
    predecessor = PREVIOUS / 'validation.json'
    assert sha(predecessor) == '1ef1854cb4c1b17e3269bfab495a199efdb17686862aca09034cdcfc4e792db4'
    logs, binaries = {}, {}
    for kind, directory in (('production', 'build-v19'), ('asan', 'asan-v19')):
        for stage in ('configure', 'build', 'rebuild', 'test'):
            path = E / f'{kind}-{stage}.log'
            logs[path.name] = sha(path)
        output = (E / f'{kind}-test.log').read_text()
        assert '100% tests passed out of 31' in output and 'Skipped' not in output and 'Not Run' not in output
        assert len(re.findall(r'\boriginal_physics_world_\w+\s+\.+\s+Passed', output)) == 21
        for name in ('native_physics_platform_locks', 'native_physics_platform_construction', 'native_physics_bsp_construction',
                'native_physics_world_construction', 'native_physics_world_setup_listener', 'native_physics_world_manager',
                'native_physics_world_initialization', 'native_physics_game_frame_prefix', 'native_physics_game_frame', 'native_physics_game_frame_fp_controls'):
            assert re.search(r'\b' + name + r'\s+\.+\s+Passed', output), name
        assert 'ninja: no work to do.' in (E / f'{kind}-rebuild.log').read_text()
        for name in ('physics_world_runtime_tests', 'physics_platform_locks_tests', 'physics_platform_call_tests', 'physics_world_mmx_tests', 'physics_game_frame_runtime_tests'):
            binaries[kind + '/' + name] = sha(BASE / directory / name)
        build = (BASE / directory / 'build.ninja').read_text()
        block = build.split('CMakeFiles/halo2_physics_world_runtime.dir/src/physics_world_runtime_generated.cpp.o:')[1].split('\n\n')[0]
        assert all(flag in block for flag in ('-fno-fast-math', '-ffp-contract=off', '-frounding-math'))
        if kind == 'asan':
            assert '-fsanitize=address,undefined' in block and '-O0' in block
    for name in ('hardware-frame-verified.log', 'hardware-prefix-verified.log', 'hardware-fp-verified.log',
                 'hardware-frame-repeat.log', 'hardware-prefix-repeat.log', 'hardware-fp-repeat.log'):
        logs[name] = sha(E / name)
    result = dict(schema=1, validator_sha256=sha(__file__), source_manifest_sha256=sha(manifest_path), parent_manifest_sha256=BUILD_MANIFEST_PIN,
        v18_parent_manifest_sha256=PARENT_PIN, compiled_build_source_manifest_sha256=sha(build_manifest_path), compiled_inputs_unchanged_by_audit_correction=True,
        unchanged_parent_validation_sha256=sha(predecessor), correspondence_sha256=PAIR_PIN, generated_manifest_sha256=GENERATED_PIN,
        original_fixture_sha256=ORIGINAL_PIN, hardware_frame_fixture_sha256=FRAME_PIN, hardware_prefix_fixture_sha256=PREFIX_PIN,
        hardware_fp_fixture_sha256=FP_PIN, complete_paired_bodies=477, lowered_instructions=35694, added_bodies=17,
        reached_added_entries=reached_entries, unexercised_added_entries=['000bbe70'],
        retained_frame_instruction_observations=24941, retained_frame_original_hardware_steps=24936, retained_frame_ordered_writes=5080,
        retained_frame_state_bytes=7452, fp_cases=25, fp_instruction_observations=623879, fp_ordered_writes=127024,
        fp_retained_state_bytes=288575, native_checks_passed=31, sanitizer_checks_passed=31, logs=logs, binaries=binaries,
        independent_source_generation_and_original_repeats_identical=True, public_outer_frame_entry_verified=True,
        original_instruction_patches=[], native_expected_data_used=False, production_original_code_execution=False,
        emulator_observable_fp_difference_preserved=True, source_fp_controls_and_exception_state_compared=True,
        host_fp_scope_restoration_compared=True, retained_outer_frame_complete=True, fresh_native_world_proven=False,
        live_match_integrated=False, full_tu15_startup_frame_proven=False, xbox_cpu_estimates_proven=False,
        whole_physics_engine_complete=False, published_launcher_replaced=False,
        limits=['Actual retained1.1 input plus24 declared FP ABI variations; full1.5 frame/startup is not executed',
            'Additional original entry page is authenticated input, never expected or production replay state',
            '477 complete paired bodies; only reached paths and16 of17 added entries execute in these cases',
            'Host x87/SSE estimates and Linux FS routing do not prove original Xbox CPU/kernel behavior',
            'Fresh startup/context/world, remaining body/controller/agent owners and live mover integration remain',
            'Vehicles/ragdolls, continuous all-map split-screen/LAN and external asset-only release remain',
            'Private original fixtures and inherited resources are not redistributable code-only artifacts'])
    path = E / 'validation.json'
    path.write_text(json.dumps(result, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(native_checks=31, sanitizer_checks=31, validation_sha256=sha(path))))


if __name__ == '__main__':
    main()
