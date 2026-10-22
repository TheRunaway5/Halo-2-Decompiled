#!/usr/bin/env python3
"""Authenticate native full-call evidence without claiming complete live physics."""
import json
import re
import zlib
from pathlib import Path
from freeze_physics_world_event_runtime_20261011 import ROOT, BASE, OWNED, PARENT_PIN, sha

E = ROOT / 'analysis/multiplayer/sandbox-source/physics-world-event-runtime-20261011'
PAIR_PIN = '789f91f0e53c22d5b0d8926a89ce87242d613c1cdbdd2855e8da8253eb1a37d7'
GENERATED_PIN = '9052e796674a37d349d55b076ecccba21e2a7a51840a3cf5b5df9d7f1015db76'
CHECKPOINT_PIN = '30fc15530e53f157fbc54afde8f009559db3f44698b0ef9a865a7b99dbfbacb4'
BASELINE_PIN = '203e132fa7ccc83744682ba113f7d575366f6164cd38bc7e1327b28a8d4d033c'


def read(path):
    return json.loads(path.read_text())


def main():
    manifest = BASE / 'source-manifest-v10.json'
    frozen = read(manifest)
    parent = BASE / 'source-manifest-v9.json'
    assert sha(parent) == PARENT_PIN == frozen['parent_manifest_sha256']
    previous = read(parent)
    assert frozen['owned_overlays'] == list(OWNED)
    for relative, pin in previous['files'].items():
        assert sha(BASE / 'source-v9' / relative) == pin, relative
        if relative not in OWNED:
            assert frozen['files'][relative] == pin, relative
    for relative, pin in frozen['files'].items():
        assert sha(BASE / 'source-v10' / relative) == pin, relative
        assert not relative.startswith(('assets/', 'analysis/'))
    for relative in OWNED:
        assert sha(ROOT / relative) == frozen['files'][relative], relative
    assert (BASE / 'source-v10/CMakeLists.txt').read_bytes() == (BASE / 'source-v9/CMakeLists.txt').read_bytes()
    pair = E / 'correspondence-v27/complete-report.json'
    assert sha(pair) == PAIR_PIN == sha(E / 'correspondence-v27-repeat/complete-report.json')
    paired = read(pair)
    assert paired['extension_tool_sha256'] == sha(ROOT / 'tools/research_physics_world_event_runtime_20261011.py')
    assert paired['complete_bodies'] == 296 and sum(paired['mnemonics'].values()) == 27033
    assert len(set(paired['instruction_map'].values())) == len(paired['instruction_map'])
    assert not paired['outside_tail_jumps']
    for row in paired['routines']:
        for label in ('disc', 'later'):
            listing = row[label + '_listing']
            assert sha(pair.parent / 'instructions' / listing['file']) == listing['sha256']
    generated = E / 'draft-v27/generated-manifest.json'
    assert sha(generated) == GENERATED_PIN == sha(E / 'generated-v27-repeat/manifest.json')
    g = read(generated)
    assert g['correspondence_sha256'] == PAIR_PIN
    assert g['generator_sha256'] == sha(ROOT / 'tools/generate_physics_world_event_runtime_20261011.py')
    assert g['generated_cpp_sha256'] == sha(ROOT / 'src/physics/runtime/physics_world_runtime_generated.cpp') == sha(E / 'generated-v27-repeat/runtime.cpp')
    assert len(g['complete_native_entries']) == 296 and len(g['lowered_instructions']) == 27033
    assert not g['production_opcode_decoder'] and not g['production_original_code_execution']
    baseline = read(E / 'disc-original-v5/fixture.json')
    assert sha(E / 'disc-original-v5/fixture.json') == BASELINE_PIN
    for folder in ('checkpoint-v5', 'checkpoint-v5-repeat'):
        path = E / folder / 'checkpoint.json'
        assert sha(path) == CHECKPOINT_PIN
        checkpoint = read(path)
        assert checkpoint['tool_sha256'] == sha(ROOT / 'tools/capture_physics_world_checkpoint_20261011.py')
        assert checkpoint['original_capture_tool_sha256'] == sha(ROOT / 'tools/oracle_physics_world_event_runtime_20261011.py')
        assert checkpoint['parent_fixture_sha256'] == checkpoint['unchanged_original_prefix_sha256'] == BASELINE_PIN
        assert sha(path.parent / 'measured/fixture.json') == BASELINE_PIN
        assert checkpoint['captured_entry']['entry_abi'] == baseline['cases'][0]['entry_abi']
        assert checkpoint['original_text_unpatched'] and not checkpoint['production_checkpoint_replay'] and not checkpoint['native_expected_data_used']
        for region in checkpoint['captured_entry']['regions']:
            resource = path.parent / 'memory' / region['file']
            assert sha(resource) == region['compressed_sha256']
            raw = zlib.decompress(resource.read_bytes())
            assert len(raw) == region['size']
            import hashlib
            assert hashlib.sha256(raw).hexdigest() == region['sha256']
    reexecution = read(E / 'checkpoint-reexecution-v5/fixture.json')
    assert reexecution['cases'] == baseline['cases'] and reexecution['previous_original_prefix_identical']
    original = read(E / 'disc-original-verified/fixture.json')
    fast = read(E / 'disc-checkpoint-v27/fixture.json')
    assert fast['cases'] == original['cases'] and fast['previous_original_prefix_identical']
    assert fast['checkpoint_sha256'] == CHECKPOINT_PIN
    assert fast['tool_sha256'] == reexecution['tool_sha256'] == sha(ROOT / 'tools/oracle_physics_world_checkpoint_20261011.py')
    assert sha(E / 'disc-checkpoint-v27/fixture.json') == sha(E / 'disc-checkpoint-v27-repeat/fixture.json')
    bindings = ROOT / 'analysis/multiplayer/sandbox-source/physics-world-runtime-20261011/input-bindings-extra-v4/report.json'
    assert sha(bindings) == 'e2311bd9cb96bb734646ff47bd919900d0602a3cc43e066ceb97b0dac1329277'
    fixtures, seen, totals = {}, set(), [0, 0, 0, 0, 0]
    for name, folder, repeat, count, latest, stress in (
        ('disc', 'disc-original-verified', 'disc-original-repeat', 1, False, False),
        ('u5', 'u5-rebound-verified', 'u5-rebound-repeat', 1, True, False),
        ('disc_fp', 'disc-x87-verified', 'disc-x87-repeat', 24, False, True),
        ('u5_fp', 'u5-x87-verified', 'u5-x87-repeat', 24, True, True),
    ):
        path = E / folder / 'fixture.json'
        assert sha(path) == sha(E / repeat / 'fixture.json')
        f = read(path)
        assert f['correspondence_sha256'] == PAIR_PIN and not f['native_expected_data_used']
        tool = 'oracle_physics_world_event_full_20261011.py' if name == 'disc' else 'oracle_physics_world_event_rebound_20261011.py'
        assert f['tool_sha256'] == sha(ROOT / 'tools' / tool)
        assert len(f['cases']) == count
        if name == 'disc':
            assert not f['instruction_patches'] and f['prior_complete_measurement_identical']
        else:
            assert not f['source_instruction_patches'] and not f['live_gameplay_proof']
            assert f['input_fixture_sha256'] == fixtures['disc']['sha256']
            assert f['input_bindings_sha256'] == sha(bindings) and f['declared_x87_input_perturbations'] == stress
        stats = [count, sum(len(c['instructions']) for c in f['cases']), sum(len(c['writes']) for c in f['cases']),
            sum(len(bytes.fromhex(b['hex'])) for c in f['cases'] for b in c['final_banks']), sum(len(c['packets']) for c in f['cases'])]
        assert stats == [count, 23851 * count, 4922 * count, 7162 * count, 2 * count], (name, stats)
        for c in f['cases']:
            assert c['complete'] and c['exit_entry'] == c['return_address']
            assert len(c['estimates']) == 3 and not c['timestamps']
            seen.update(c['instructions'])
        if stress:
            assert {(c['input_perturbation']['depth'], c['input_perturbation']['top']) for c in f['cases']} == {(d, t) for d in (0, 1, 3) for t in range(8)}
        fixtures[name] = dict(sha256=sha(path), counts=stats, complete=True, declared_fp_stress=stress)
        totals = [a + b for a, b in zip(totals, stats)]
    assert totals == [50, 1192550, 246100, 358100, 100] and len(seen) == 9326
    rebound = read(E / 'disc-rebound-verified/fixture.json')
    for a, b in zip(original['cases'], rebound['cases']):
        for key in ('entry_abi', 'exit', 'exit_entry', 'complete', 'initial_banks', 'final_banks', 'writes', 'instructions', 'packets', 'estimates', 'timestamps'):
            assert a[key] == b[key], key
    names = ['world_runtime', 'world_runtime_tu15', 'world_runtime_x87', 'world_runtime_x87_tu15',
        'world_runtime_retained_contacts', 'world_runtime_retained_contacts_tu15',
        'world_listener', 'world_listener_tu15', 'world_listener_x87', 'world_listener_x87_tu15',
        'world_listener_manager_boundary', 'world_listener_manager_boundary_tu15',
        'world_event', 'world_event_tu15', 'world_event_x87', 'world_event_x87_tu15']
    logs, binaries = {}, {}
    for kind, directory in (('production', 'build-v10'), ('asan', 'asan-v10')):
        for stage in ('configure', 'build', 'rebuild', 'test'):
            path = E / f'{kind}-{stage}.log'
            logs[path.name] = sha(path)
        output = (E / f'{kind}-test.log').read_text()
        assert '100% tests passed out of 16' in output and 'Skipped' not in output
        for name in names:
            assert re.search(r'\boriginal_physics_' + name + r'\s+\.+\s+Passed', output), name
        binaries[kind] = sha(BASE / directory / 'physics_world_runtime_tests')
        block = (BASE / directory / 'build.ninja').read_text().split('CMakeFiles/halo2_physics_world_runtime.dir/src/physics_world_runtime_generated.cpp.o:')[1].split('\n\n')[0]
        for flag in ('-fno-fast-math', '-ffp-contract=off', '-frounding-math'):
            assert flag in block
        if kind == 'asan':
            assert '-fsanitize=address,undefined' in block and '-O0' in block
    prior_validation = ROOT / 'analysis/multiplayer/sandbox-source/physics-world-listener-runtime-20261011/validation.json'
    assert sha(prior_validation) == '287b934b41d632b2d1ac0a4d4fa751a058c241268b9bf4d598b5a673a4d60080'
    result = dict(schema=1, validator_sha256=sha(__file__), source_manifest_sha256=sha(manifest),
        parent_source_manifest_sha256=PARENT_PIN, complete_paired_native_bodies=296, lowered_instructions=27033,
        exercised_instructions=len(seen), correspondence_sha256=PAIR_PIN, generated_manifest_sha256=GENERATED_PIN,
        checkpoint_sha256=CHECKPOINT_PIN, fixtures=fixtures, totals=totals, native_checks_passed=16,
        sanitizer_checks_passed=16, logs=logs, binaries=binaries, original_setup_and_checkpoint_identical=True,
        source_and_fixture_repeats_identical=True, unchanged_previous_validation_sha256=sha(prior_validation),
        production_opcode_decoder=False, production_original_execution=False, production_checkpoint_replay=False,
        live_match_mover_replaced=False, whole_physics_engine_complete=False, published_launcher_replaced=False,
        private_validation_tree_inherits_original_test_resources=True,
        limits=['One retained complete world call is not continuous or live gameplay',
            'TU1.5 inputs explicitly rebind the 1.1 state, not complete TU1.5 world construction',
            'Declared FP variations remain one gameplay scenario',
            'Live memory/controller/allocator/world construction and other required lower owners remain',
            'Other motions/shapes, vehicle/ragdoll constraints, Xbox CPU estimates and all-map split-screen/LAN remain',
            'Unused x87 exception bits, sparse stacks and stack faults remain unproven',
            'Physical external asset relocation remains open'])
    path = E / 'validation.json'
    path.write_text(json.dumps(result, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(cases=totals[0], instructions=totals[1], writes=totals[2], full_state_bytes=totals[3], validation_sha256=sha(path))))


if __name__ == '__main__':
    main()
