#!/usr/bin/env python3
"""Verify frozen native contact-listener evidence without claiming live parity."""
import json
import re
from pathlib import Path
from freeze_physics_world_listener_runtime_20261011 import ROOT, BASE, OWNED, PARENT_PIN, sha

E = ROOT / 'analysis/multiplayer/sandbox-source/physics-world-listener-runtime-20261011'
PAIR_PIN = 'fa14b9224e90167338b19bc1193398e195d78c022dc03124dd6825a9210e5d16'
GENERATED_PIN = '2730709615dd7693df6f4978236dfea72d54f0c8d3871ad33cd4247fd4d82921'


def main():
    parent = BASE / 'source-manifest-v8.json'
    assert sha(parent) == PARENT_PIN
    previous = json.loads(parent.read_text())
    manifest = BASE / 'source-manifest-v9.json'
    frozen = json.loads(manifest.read_text())
    assert frozen['parent_manifest_sha256'] == PARENT_PIN
    assert frozen['owned_overlays'] == list(OWNED)
    for relative, pin in previous['files'].items():
        assert sha(BASE / 'source-v8' / relative) == pin, relative
        if relative not in OWNED:
            assert frozen['files'][relative] == pin, relative
    for relative, pin in frozen['files'].items():
        assert sha(BASE / 'source-v9' / relative) == pin, relative
    for relative in OWNED:
        assert sha(ROOT / relative) == frozen['files'][relative], relative
    assert (BASE / 'source-v9/CMakeLists.txt').read_bytes() == (BASE / 'source-v8/CMakeLists.txt').read_bytes()
    pair = E / 'correspondence-v8/complete-report.json'
    assert sha(pair) == PAIR_PIN == sha(E / 'correspondence-v8-repeat/complete-report.json')
    paired = json.loads(pair.read_text())
    assert paired['complete_bodies'] == 226 and sum(paired['mnemonics'].values()) == 22716
    assert paired['extension_tool_sha256'] == sha(ROOT / 'tools/research_physics_world_listener_runtime_20261011.py')
    for row in paired['routines']:
        for label in ('disc', 'later'):
            listing = row[label + '_listing']
            assert sha(pair.parent / 'instructions' / listing['file']) == listing['sha256']
    generated = E / 'draft-v9/generated-manifest.json'
    assert sha(generated) == GENERATED_PIN == sha(E / 'generated-v9-repeat/manifest.json')
    g = json.loads(generated.read_text())
    assert g['correspondence_sha256'] == PAIR_PIN
    assert g['generator_sha256'] == sha(ROOT / 'tools/generate_physics_world_listener_runtime_20261011.py')
    assert g['generated_cpp_sha256'] == sha(ROOT / 'src/physics/runtime/physics_world_runtime_generated.cpp') == sha(E / 'generated-v9-repeat/runtime.cpp')
    assert len(g['complete_native_entries']) == 226 and len(g['lowered_instructions']) == 22716
    assert not g['production_opcode_decoder'] and not g['production_original_code_execution']
    binding = ROOT / 'analysis/multiplayer/sandbox-source/physics-world-runtime-20261011/input-bindings-extra-v4/report.json'
    assert sha(binding) == 'e2311bd9cb96bb734646ff47bd919900d0602a3cc43e066ceb97b0dac1329277'
    fixtures = {}
    totals = [0, 0, 0, 0, 0]
    seen = set()
    for name, folder, repeat, count, latest, stress in (
        ('disc', 'disc-original-verified', 'disc-original-repeat', 1, False, False),
        ('u5', 'u5-rebound-verified', 'u5-rebound-repeat', 1, True, False),
        ('disc_fp', 'disc-x87-verified', 'disc-x87-repeat', 24, False, True),
        ('u5_fp', 'u5-x87-verified', 'u5-x87-repeat', 24, True, True),
    ):
        path = E / folder / 'fixture.json'
        assert sha(path) == sha(E / repeat / 'fixture.json')
        f = json.loads(path.read_text())
        assert f['correspondence_sha256'] == PAIR_PIN and not f['native_expected_data_used']
        tool = 'oracle_physics_world_listener_runtime_20261011.py' if name == 'disc' else 'oracle_physics_world_listener_rebound_20261011.py'
        assert f['tool_sha256'] == sha(ROOT / 'tools' / tool)
        assert len(f['cases']) == count
        if name == 'disc':
            assert not f['instruction_patches'] and f['prior_complete_measurement_identical']
            assert f['hash_helper_sha256'] == sha(ROOT / 'tools/original_physics_file_hash.py')
        else:
            assert not f['source_instruction_patches'] and not f['live_gameplay_proof']
            assert f['input_fixture_sha256'] == fixtures['disc']['sha256']
            assert f['input_bindings_sha256'] == sha(binding)
            assert f['declared_x87_input_perturbations'] == stress
        stats = [len(f['cases']), sum(len(c['instructions']) for c in f['cases']),
            sum(len(c['writes']) for c in f['cases']),
            sum(len(bytes.fromhex(b['hex'])) for c in f['cases'] for b in c['final_banks']),
            sum(len(c['packets']) for c in f['cases'])]
        assert stats == [count, 14969 * count, 2930 * count, 5419 * count, 2 * count], (name, stats)
        if stress:
            assert {(c['input_perturbation']['depth'], c['input_perturbation']['top']) for c in f['cases']} == {(d, t) for d in (0, 1, 3) for t in range(8)}
        for c in f['cases']:
            assert not c['complete']
            assert c['exit_entry'] == (paired['address_map']['00208fb0'] if latest else 0x208fb0)
            assert len(c['packets']) == 2 and not c['timestamps']
            seen.update(c['instructions'])
        fixtures[name] = dict(sha256=sha(path), counts=stats,
            stop_entries=sorted({c['exit_entry'] for c in f['cases']}), declared_fp_stress=stress)
        totals = [a + b for a, b in zip(totals, stats)]
    assert totals[0] == 50 and totals[4] == 100
    original = json.loads((E / 'disc-original-verified/fixture.json').read_text())
    rebound = json.loads((E / 'disc-rebound-verified/fixture.json').read_text())
    for a, b in zip(original['cases'], rebound['cases']):
        for key in ('entry_abi', 'exit', 'exit_entry', 'complete', 'initial_banks', 'final_banks', 'writes', 'instructions', 'packets', 'estimates', 'timestamps'):
            assert a[key] == b[key], key
    # The old fallback ran two unmapped TU instructions at numeric old-site
    # collisions. This retained case must stop at the actual virtual target.
    regression = E / 'u5-rebound-v7-strict/fixture.json'
    assert sha(regression) == '522485e7b73cfa83a5147bbb5adff58c74298a5d38fa3cafe37eb9b84b0b42fa'
    c = json.loads(regression.read_text())['cases'][0]
    assert c['exit_entry'] == 0x323020 and len(c['instructions']) == 14805
    assert 0x323020 not in c['instructions'] and 0x323025 not in c['instructions']
    names = ['world_runtime', 'world_runtime_tu15', 'world_runtime_x87', 'world_runtime_x87_tu15',
        'world_runtime_retained_contacts', 'world_runtime_retained_contacts_tu15',
        'world_listener', 'world_listener_tu15', 'world_listener_x87', 'world_listener_x87_tu15',
        'world_listener_manager_boundary', 'world_listener_manager_boundary_tu15']
    logs = {}
    binaries = {}
    for kind, directory in (('production', 'build-v9'), ('asan', 'asan-v9')):
        for stage in ('configure', 'build', 'rebuild', 'test'):
            path = E / f'{kind}-{stage}.log'
            logs[path.name] = sha(path)
        output = (E / f'{kind}-test.log').read_text()
        assert '100% tests passed out of 12' in output and 'Skipped' not in output
        for name in names:
            assert re.search(r'\boriginal_physics_' + name + r'\s+\.+\s+Passed', output), name
        binaries[kind] = sha(BASE / directory / 'physics_world_runtime_tests')
        block = (BASE / directory / 'build.ninja').read_text().split('CMakeFiles/halo2_physics_world_runtime.dir/src/physics_world_runtime_generated.cpp.o:')[1].split('\n\n')[0]
        for flag in ('-fno-fast-math', '-ffp-contract=off', '-frounding-math'):
            assert flag in block
        if kind == 'asan':
            assert '-fsanitize=address,undefined' in block and '-O0' in block
    prior_validation = ROOT / 'analysis/multiplayer/sandbox-source/physics-world-runtime-20261011/validation.json'
    assert sha(prior_validation) == '0d8ae8555b0cca7654da250a7f7c799e500c1a794ab50dec4620d48a805c1a6f'
    result = dict(schema=1, validator_sha256=sha(__file__), source_manifest_sha256=sha(manifest),
        parent_source_manifest_sha256=PARENT_PIN, frozen_files=len(frozen['files']),
        complete_paired_native_bodies=226, lowered_instructions=22716, exercised_instructions=len(seen),
        correspondence_sha256=PAIR_PIN, generated_manifest_sha256=GENERATED_PIN,
        fixtures=fixtures, totals=totals, native_checks_passed=12, sanitizer_checks_passed=12,
        logs=logs, binaries=binaries, retained_manager_address_collision_regression_sha256=sha(regression),
        unchanged_previous_validation_sha256=sha(prior_validation),
        source_and_fixture_repeats_identical=True, production_opcode_decoder=False,
        production_original_execution=False, instruction_patches=[], live_match_mover_replaced=False,
        whole_physics_engine_complete=False, published_launcher_replaced=False,
        limits=['Retained world prefix is not a complete world step or live gameplay',
            'TU1.5 inputs explicitly rebind the 1.1 state, not complete TU1.5 world construction',
            'Declared FP perturbations are not additional gameplay scenarios',
            'Other required lower owners and actual live memory/controller/allocator construction remain',
            'Other motions/shapes, vehicle/ragdoll constraints, Xbox CPU estimates and all-map split-screen/LAN remain',
            'Unused x87 exception bits, sparse stacks and stack faults remain unproven',
            'Physical external asset relocation remains open'])
    path = E / 'validation.json'
    path.write_text(json.dumps(result, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(cases=totals[0], instructions=totals[1], writes=totals[2], full_state_bytes=totals[3], validation_sha256=sha(path))))


if __name__ == '__main__':
    main()
