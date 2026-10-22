#!/usr/bin/env python3
"""Authenticate native initialization evidence without claiming live integration."""
import hashlib
import json
import re
import zlib
from freeze_physics_world_bootstrap_runtime_20261011 import ROOT, BASE, OWNED, PARENT_PIN, sha

E = ROOT / 'analysis/multiplayer/sandbox-source/physics-world-bootstrap-runtime-20261011'
PAIR_PIN = '7b2500b397173b0a250aed96be86b3a11848b3872fa9382d2a0e11daca6002ec'
GENERATED_PIN = '8b0d282efa9b91d5488336b88cc594515597cb259850ab28802ecf1883510775'
BINDINGS_PIN = '9acb16f6ec0196380e6ff2a678e9ff0f233fad7185a31dd8bf0b57e37307b7ae'
BASE_CHECKPOINT_PIN = 'e088390dbaa9b164ad8427dda9ebf0533883ff89ee7ea600dad85f965fd8307d'


def read(path):
    return json.loads(path.read_text())


def main():
    manifest = BASE / 'source-manifest-v11.json'
    frozen = read(manifest)
    parent_path = BASE / 'source-manifest-v10.json'
    assert sha(parent_path) == PARENT_PIN == frozen['parent_manifest_sha256']
    parent = read(parent_path)
    assert frozen['owned_overlays'] == list(OWNED)
    for relative, pin in parent['files'].items():
        assert sha(BASE / 'source-v10' / relative) == pin, relative
        if relative not in OWNED:
            assert frozen['files'][relative] == pin, relative
    for relative, pin in frozen['files'].items():
        assert sha(BASE / 'source-v11' / relative) == pin, relative
        assert not relative.startswith(('assets/', 'analysis/'))
    for relative in OWNED:
        assert sha(ROOT / relative) == frozen['files'][relative], relative
    pair_path = E / 'correspondence-v19/complete-report.json'
    assert sha(pair_path) == PAIR_PIN == sha(E / 'correspondence-v19-repeat/complete-report.json')
    pair = read(pair_path)
    assert pair['extension_tool_sha256'] == sha(ROOT / 'tools/research_physics_world_bootstrap_runtime_20261011.py')
    assert pair['complete_bodies'] == 362 and sum(pair['mnemonics'].values()) == 30221
    assert len(set(pair['instruction_map'].values())) == len(pair['instruction_map'])
    assert set(pair['outside_tail_jumps']) == {'001ecfe0'}
    for row in pair['routines']:
        for label in ('disc', 'later'):
            listing = row[label + '_listing']
            assert sha(pair_path.parent / 'instructions' / listing['file']) == listing['sha256']
    bindings_path = E / 'static-bindings-v4/report.json'
    assert sha(bindings_path) == BINDINGS_PIN
    bindings = read(bindings_path)
    assert bindings['tool_sha256'] == sha(ROOT / 'tools/research_physics_world_bootstrap_bindings_20261011.py')
    assert bindings['all_eight_registrations_bound'] and not bindings['guessed_bindings']
    assert len(bindings['registrations']) == 8 and len(bindings['routines']) == 21
    for row in bindings['routines']:
        assert row['complete_body_equal']
        for label in ('disc', 'later'):
            listing = row[label + '_listing']
            assert sha(bindings_path.parent / 'instructions' / listing['file']) == listing['sha256']
        assert pair['entries'][f"{row['old_entry']:08x}"] == row['later_entry']
    generated_path = E / 'draft-v19/manifest.json'
    assert sha(generated_path) == GENERATED_PIN == sha(E / 'generated-v19-repeat/manifest.json')
    generated = read(generated_path)
    assert generated['generator_sha256'] == sha(ROOT / 'tools/generate_physics_world_bootstrap_runtime_20261011.py')
    assert generated['correspondence_sha256'] == PAIR_PIN
    assert generated['generated_cpp_sha256'] == sha(ROOT / 'src/physics/runtime/physics_world_runtime_generated.cpp') == sha(E / 'generated-v19-repeat/runtime.cpp')
    assert len(generated['complete_native_entries']) == 362 and len(generated['lowered_instructions']) == 30221
    assert not generated['production_opcode_decoder'] and not generated['production_original_code_execution']
    base_checkpoint = read(E / 'disc-original-v1/checkpoint.json')
    assert sha(E / 'disc-original-v1/checkpoint.json') == BASE_CHECKPOINT_PIN
    assert sha(E / 'disc-original-v1/checkpoint.json') == sha(E / 'disc-original-v1-repeat/checkpoint.json')
    checkpoint = read(E / 'disc-original-verified/checkpoint.json')
    assert sha(E / 'disc-original-verified/checkpoint.json') == sha(E / 'disc-original-repeat/checkpoint.json')
    assert checkpoint['captured_entry'] == base_checkpoint['captured_entry']
    assert checkpoint['tool_sha256'] == sha(ROOT / 'tools/capture_physics_world_bootstrap_20261011.py')
    assert checkpoint['parent_fixture_sha256'] == checkpoint['unchanged_original_prefix_sha256'] == sha(E / 'disc-original-verified/fixture.json')
    assert checkpoint['original_prepare_completed'] and checkpoint['original_text_unpatched']
    assert not checkpoint['native_expected_data_used'] and not checkpoint['production_checkpoint_replay']
    for region in checkpoint['captured_entry']['regions']:
        resource = E / 'disc-original-verified/memory' / region['file']
        repeat = E / 'disc-original-repeat/memory' / region['file']
        assert sha(resource) == region['compressed_sha256'] == sha(repeat)
        raw = zlib.decompress(resource.read_bytes())
        assert len(raw) == region['size'] and hashlib.sha256(raw).hexdigest() == region['sha256']
    original = read(E / 'disc-original-verified/fixture.json')
    fast = read(E / 'disc-checkpoint-v19/fixture.json')
    assert fast['cases'] == original['cases'] and fast['previous_original_prefix_identical']
    assert fast['checkpoint_sha256'] == BASE_CHECKPOINT_PIN
    assert fast['tool_sha256'] == sha(ROOT / 'tools/oracle_physics_world_bootstrap_checkpoint_20261011.py')
    assert read(E / 'disc-checkpoint-v1/fixture.json')['cases'] == read(E / 'disc-original-v1/fixture.json')['cases']
    fixtures, totals, seen = {}, [0, 0, 0, 0], set()
    for name, folder, repeat, count, stress in (
        ('disc', 'disc-original-verified', 'disc-original-repeat', 1, False),
        ('u5', 'u5-rebound-verified', 'u5-rebound-repeat', 1, False),
        ('disc_fp', 'disc-x87-verified', 'disc-x87-repeat', 24, True),
        ('u5_fp', 'u5-x87-verified', 'u5-x87-repeat', 24, True),
    ):
        path = E / folder / 'fixture.json'
        assert sha(path) == sha(E / repeat / 'fixture.json')
        fixture = read(path)
        assert fixture['correspondence_sha256'] == PAIR_PIN and not fixture['native_expected_data_used']
        tool = 'capture_physics_world_bootstrap_20261011.py' if name == 'disc' else 'oracle_physics_world_bootstrap_rebound_20261011.py'
        assert fixture['tool_sha256'] == sha(ROOT / 'tools' / tool)
        assert len(fixture['cases']) == count
        if name != 'disc':
            assert fixture['input_fixture_sha256'] == sha(E / 'disc-checkpoint-v19/fixture.json')
            assert fixture['bootstrap_bindings_sha256'] == BINDINGS_PIN
            assert fixture['declared_x87_input_perturbations'] == stress
            assert fixture['kernel_import_trap_routing_only'] and not fixture['source_instruction_patches'] and not fixture['live_gameplay_proof']
        stats = [count, sum(len(c['instructions']) for c in fixture['cases']), sum(len(c['writes']) for c in fixture['cases']),
            sum(len(bytes.fromhex(bank['hex'])) for c in fixture['cases'] for bank in c['final_banks'])]
        assert stats == [count, 17504 * count, 1953 * count, 1890 * count], (name, stats)
        for case in fixture['cases']:
            assert not case['complete'] and case['exit_entry'] == 0x80000115
            assert not case['estimates'] and not case['timestamps'] and not case['packets']
            seen.update(case['instructions'])
        if stress:
            assert {(c['input_perturbation']['depth'], c['input_perturbation']['top']) for c in fixture['cases']} == {(d, t) for d in (0, 1, 3) for t in range(8)}
        totals = [a + b for a, b in zip(totals, stats)]
        fixtures[name] = dict(sha256=sha(path), counts=stats, complete=False, kernel_boundary=0x80000115, declared_fp_stress=stress)
    assert totals == [50, 875200, 97650, 94500] and len(seen) == 779
    rebound = read(E / 'disc-rebound-verified/fixture.json')
    for a, b in zip(original['cases'], rebound['cases']):
        for key in ('entry_abi', 'exit', 'exit_entry', 'complete', 'initial_banks', 'final_banks', 'writes', 'instructions', 'packets', 'estimates', 'timestamps'):
            assert a[key] == b[key], key
    names = ['world_runtime', 'world_runtime_tu15', 'world_runtime_x87', 'world_runtime_x87_tu15',
        'world_runtime_retained_contacts', 'world_runtime_retained_contacts_tu15',
        'world_listener', 'world_listener_tu15', 'world_listener_x87', 'world_listener_x87_tu15',
        'world_listener_manager_boundary', 'world_listener_manager_boundary_tu15',
        'world_event', 'world_event_tu15', 'world_event_x87', 'world_event_x87_tu15',
        'world_bootstrap', 'world_bootstrap_tu15', 'world_bootstrap_x87', 'world_bootstrap_x87_tu15']
    logs, binaries = {}, {}
    for kind, directory in (('production', 'build-v11'), ('asan', 'asan-v11')):
        for stage in ('configure', 'build', 'rebuild', 'test'):
            path = E / f'{kind}-{stage}.log'
            logs[path.name] = sha(path)
        output = (E / f'{kind}-test.log').read_text()
        assert '100% tests passed out of 20' in output and 'Skipped' not in output
        for name in names:
            assert re.search(r'\boriginal_physics_' + name + r'\s+\.+\s+Passed', output), name
        binaries[kind] = sha(BASE / directory / 'physics_world_runtime_tests')
        block = (BASE / directory / 'build.ninja').read_text().split('CMakeFiles/halo2_physics_world_runtime.dir/src/physics_world_runtime_generated.cpp.o:')[1].split('\n\n')[0]
        assert all(flag in block for flag in ('-fno-fast-math', '-ffp-contract=off', '-frounding-math'))
        if kind == 'asan':
            assert '-fsanitize=address,undefined' in block and '-O0' in block
    previous_validation = ROOT / 'analysis/multiplayer/sandbox-source/physics-world-event-runtime-20261011/validation.json'
    assert sha(previous_validation) == '8490312facaa098283349098d3e0d9a8c1d71009a77888ffa1731279dba74c0b'
    result = dict(schema=1, validator_sha256=sha(__file__), source_manifest_sha256=sha(manifest),
        parent_source_manifest_sha256=PARENT_PIN, complete_paired_native_bodies=362, lowered_instructions=30221,
        exercised_initialization_instructions=len(seen), correspondence_sha256=PAIR_PIN,
        generated_manifest_sha256=GENERATED_PIN, startup_bindings_sha256=BINDINGS_PIN,
        checkpoint_sha256=sha(E / 'disc-original-verified/checkpoint.json'), fixtures=fixtures, totals=totals,
        native_checks_passed=20, sanitizer_checks_passed=20, logs=logs, binaries=binaries,
        full_schedule_and_checkpoint_prefix_identical=True, source_and_fixture_repeats_identical=True,
        unchanged_previous_validation_sha256=sha(previous_validation), production_opcode_decoder=False,
        production_original_execution=False, production_checkpoint_replay=False,
        original_kernel_lock_owner_supplied=False, full_world_initialization_complete=False,
        live_match_mover_replaced=False, whole_physics_engine_complete=False, published_launcher_replaced=False,
        private_validation_tree_inherits_original_test_resources=True,
        limits=['One initialization prefix stops at actual RtlEnterCriticalSection import277',
            'TU1.5 explicitly rebinds1.1 inputs; full TU1.5 startup is unproven',
            'Declared FP variations remain one initialization scenario; only779 distinct instructions are exercised',
            'Real native critical-section/process-heap, world/BSP/body/controller ownership and live integration remain',
            'Other motions/shapes, vehicles/ragdolls, Xbox CPU estimates and continuous all-map split-screen/LAN remain',
            'Physical external relocation of inherited original test resources remains open'])
    path = E / 'validation.json'
    path.write_text(json.dumps(result, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(cases=totals[0], instructions=totals[1], writes=totals[2], full_state_bytes=totals[3], validation_sha256=sha(path))))


if __name__ == '__main__':
    main()
