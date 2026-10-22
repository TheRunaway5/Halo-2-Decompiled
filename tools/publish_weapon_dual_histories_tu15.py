#!/usr/bin/env python3
"""Publish bounded live dual-hand and original HUD-storage evidence.

Run through tools/run_test.py. Full firing/audio and framebuffer gates remain
open; private build/source/scratch removal is required before publication.
"""
import copy
import hashlib
import json
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/weapons-tu15-goal-20261013'
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def load(name): return json.loads((BASE / name).read_text())
def save(name, value): (BASE / name).write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def main():
    comparisons = [load(f'weapon-dual-source-history-comparison-v{v}.json') for v in (1, 2)]
    assert all(r['passed'] and not r['full_weapon_parity'] for r in comparisons)
    assert sum(c['matched_hand_samples'] for c in comparisons[0]['cases']) == 663
    assert sum(c['matched_hand_samples'] for c in comparisons[1]['cases']) == 745
    assert all(not c['render_loop_completed'] for c in comparisons[0]['cases'])
    assert all(c['render_loop_completed'] and c['complete_pickup_fire_reload_drop_observed']
               for c in comparisons[1]['cases'])
    cases, references, scratch = [], [], set()
    for version in (1, 2):
        folder = f'weapon-dual-source-history-validation-v{version}'
        parent, child = load(folder + '/report.json'), load(folder + '/release/report.json')
        assert parent['status'] == child['status'] == 'failed'
        assert parent['source_scratch_removed_after_runner_exit']
        assert all(row['exit_code'] == 0 for row in child['steps'][:-1])
        assert child['steps'][-1]['name'] == 'tests' and child['steps'][-1]['exit_code'] == 8
        scratch.update((parent['runner_scratch'], child['build']))
        references += [folder + '/' + name for name in
                       ('report.json', 'release/report.json', 'release/tests.log', 'source-manifest.json')]
        log = (BASE / folder / 'release/tests.log').read_text()
        if version == 1:
            assert log.count('at 25aa73: Required original runtime backing unavailable at0x82') == 2
        else:
            assert 'at 25aa73:' not in log
            assert log.count('Live dual SMGs did not submit exactly one authored sound per fired round') == 2
        assert log.count('   Passed') == 4
        for species, representation in (('spartan', 2), ('elite', 3)):
            name = folder + '/release/evidence/weapon-source-dual-' + species + '.json'
            references.append(name)
            trace = load(name)
            if version == 1:
                continue
            assert trace['representation'] == representation
            assert trace['publication_mismatches'] == 0
            assert all(row['unit_definition'] in (0xffffffff, trace['expected_unit_definition'])
                       for row in trace['samples'])
            bindings = sorted({row['hud_storage'] for row in trace['samples']})
            definitions = sorted({row['hud_definition_address'] for row in trace['samples']})
            assert len(bindings) == len(definitions) == 1 and bindings[0] and definitions[0]
            observation = trace['observations']
            assert observation['fired_rounds'] == 7 and observation['firing_sounds'] == 0
            assert len(observation['posed_weapons']) == 2 and len(observation['clips']) == 7
            assert observation['offhand_radar'] and observation['primary_radar']
            cases.append(dict(species=species, representation=representation,
                source_unit_definition=trace['expected_unit_definition'], samples=len(trace['samples']),
                matched_hand_samples=trace['publication_checks'], publication_mismatches=0,
                hud_storage=bindings[0], hud_definition_address=definitions[0],
                complete_pickup_fire_reload_throw_drop_observed=True,
                native_firing_sequence_passed=False, **observation))
    minimizer_name = 'weapon-dual-hud-storage-minimizer-v1.json'
    minimizer = load(minimizer_name)
    assert minimizer['passed'] and len(minimizer['cases']) == 4
    assert minimizer['cases'][0]['faults'][0]['address'] == 0x82
    assert all(c['returned'] for c in minimizer['cases'][1:])
    identity_name = 'weapon-dual-hud-existing-owner-comparison-v1.json'
    identity = load(identity_name)
    assert identity['prior_bodies'] == 63 and not identity['existing_body_differences_after_literal_identity_normalization']
    lowering = 'weapon-dual-hud-owner-lowering-v1'
    bodies = load(lowering + '/ui-script-startup-bodies.json')
    new_instructions = sum(len(bodies['body_sites'][f'{entry:08x}'])
                           for entry in (0x1976fe, 0x1410f5, 0x13bc32, 0x231048))
    assert new_instructions == 92
    references += [minimizer_name, identity_name] + [lowering + '/' + n for n in
                    ('generated.json', 'data.json', 'ui-script-startup-bodies.json')]
    for version, report in enumerate(comparisons, 1):
        references.append(f'weapon-dual-source-history-comparison-v{version}.json')
        scratch.add(report['runner_scratch'])
    scratch.update((minimizer['runner_scratch'], identity['runner_scratch']))
    checks = [dict(path=p, removed=not Path(p).exists()) for p in sorted(scratch)]
    assert all(row['removed'] for row in checks)
    source_files = ['src/physics/physics_live_world_tu15.cpp', 'src/ui/source_ui_script_startup_tu15_generated.cpp',
        'src/ui/native_tu15_ui_script_startup_data.inc', 'tests/native_weapon_runtime_tests.cpp',
        'tools/validate_weapon_material_snapshot_tu15.py', 'tools/generate_ui_script_startup_tu15.py',
        'tools/export_ui_script_startup_data_tu15.py']
    pins = load('weapon-dual-source-history-validation-v2/source-manifest.json')['source_files_sha256']
    assert all(pins[name] == sha(ROOT / name) for name in source_files)
    next_work = ('Continue remaining applicable dual-hand weapon pairs, drop ownership/lifetimes and '
        'reload/interrupt histories against original owners. Complete original rendered reference '
        'sequences for all63 weapon paths/630 gates and real device/IRQ/DSP/output audio activation. '
        'The source HUD binding crash is fixed; both live SMG sequences still fail on zero firing sounds. '
        'Remove unused generated test/build/scratch files after every validation; preserve small evidence '
        'and required original/reference inputs.')
    progress_name = 'weapon-dual-source-history-progress-v1.json'
    progress = dict(passed=True, component='Source dual-hand histories and original HUD storage lifecycle',
        cases=cases, matched_hand_samples=745, previous_matched_hand_samples_before_drop_crash=663,
        new_original_map_reset_instructions=92, existing_63_original_bodies_preserved=True,
        original_constructor='1976CF', original_message_storage_constructor='25A71C',
        original_new_map_reset='1976FE', original_hud_binding='50EECC',
        original_fault_pc='25AA73', original_fault_address='82', original_fault_removed_in_both_species=True,
        original_instruction_patches=[], supplied_callee_results=[], native_tick_adjustment=0,
        full_firing_sound_sequence_passed=False, full_weapon_parity=False, framebuffer_parity_verified=False,
        fully_certified_weapons=0, next_work=next_work, large_artifacts_retained=[],
        references=[dict(report=name, sha256=sha(BASE / name)) for name in references],
        source_files_sha256={name: sha(ROOT / name) for name in source_files},
        evidence_update_tool_sha256=sha(Path(__file__)))
    save(progress_name, progress)
    matrix = copy.deepcopy(load('weapon-completion-matrix-v60.json'))
    assert len(matrix['weapons']) == 63 and sum(len(w['gates']) for w in matrix['weapons']) == 630
    smg = next(w for w in matrix['weapons'] if w['name'] == r'objects\weapons\rifle\smg\smg')
    smg['bounded_evidence'].append(dict(report=progress_name, sha256=sha(BASE / progress_name),
        component='Both source species complete actual SDL pickup/switch/dual fire/reload/grenade/offhand drop;745 independent hand publications match source banks. Original HUD storage constructor/map reset removes the real drop crash.',
        map='assets/disc/maps/lockout.map', certifies_entire_gate=False,
        certifies_all_map_variants=False, component_covers_all_scoped_maps_for_weapon=False))
    smg['gates']['inventory_lifetime'].update(status='partial', remaining=
        'Both species complete the observed SDL inventory/drop sequence on Lockout after original HUD storage initialization. Full drop-world ownership/retirement, other map variants, mixed pairs and interruptions remain required.')
    smg['gates']['hud_audio_publication']['remaining'] += (
        ' Subsequent live Spartan/Elite dual-SMG histories match745 hand samples through drop; '
        'source HUD constructor/map reset now runs. Both full tests still fail: seven rounds and zero firing sounds each.')
    matrix['previous_matrix_sha256'] = sha(BASE / 'weapon-completion-matrix-v60.json')
    matrix['evidence_update_tool_sha256'] = sha(Path(__file__))
    matrix_name = 'weapon-completion-matrix-v61.json'
    save(matrix_name, matrix)
    publisher = os.environ['HALO2_TEST_SCRATCH']
    audit_name = 'post-weapon-dual-source-history-storage-audit-v1.json'
    save(audit_name, dict(passed=True, scratch_checks=checks, publisher_scratch=publisher,
        publisher_cleanup_pending_runner_exit=True, live_owned_builds=[], large_artifacts_retained=[],
        cleanup_required_for_completion=True,
        original_assets_reference_evidence_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True))
    checkpoint_name = 'checkpoint-weapon-dual-source-history-20261019.json'
    save(checkpoint_name, dict(passed=True, previous_checkpoint='checkpoint-weapon-glow-species-view-20261019.json',
        evidence=progress_name, evidence_sha256=sha(BASE / progress_name), matrix=matrix_name,
        matrix_sha256=sha(BASE / matrix_name), storage_audit=audit_name, fully_certified_weapons=0,
        full_weapon_parity=False, cleanup_required_for_completion=True, next_work=next_work))
    state = load('continuation-working-state-v60.json')
    state.update(latest_checkpoint=checkpoint_name, latest_matrix=matrix_name,
        progress=progress['component'], next_work=next_work, dual_source_history_cases=cases,
        full_firing_failure='weapon-dual-source-history-validation-v2/release/tests.log',
        publisher_scratch=publisher, publisher_cleanup_pending_runner_exit=True,
        publisher_scratch_removed_after_runner_exit=False)
    save('continuation-working-state-v61.json', state)
    doc = ROOT / 'docs/WEAPONS_TU15_PARITY_GOAL.md'
    text = doc.read_text().replace('`weapon-completion-matrix-v60.json` expands',
                                 '`weapon-completion-matrix-v61.json` expands', 1)
    text += '''
## Live dual-SMG histories and HUD storage, October19

Both actual source species now complete the SDL map pickup, carried switch,
dual pickup, independent trigger fire, reload, grenade throw and offhand-drop
sequence. All745 hand identity/ammunition/preparation-tick publications match
their original source banks. The two datums submit616 animated weapon poses
across seven clips per species. See `checkpoint-weapon-dual-source-history-20261019.json`.

The first run in each species stopped at original25AA73 reading NULL+82.
The unchanged original reproduces that exact fault independently. Actual
25A71C/1976CF allocation and1976FE new-map initialization now own the missing
50EECC HUD storage;92 new original instructions retain the complete map reset.
The existing63 routine bodies are unchanged after identity-address normalization.
The corrected live runs retain a stable nonzero HUD binding through drop.

The four focused checks pass. Both complete tests remain failed on seven
rounds and zero authored firing-sound submissions each. Full framebuffer/audio,
all maps/weapon pairs, interruption and drop-world lifetime acceptance remain
open. Private builds/source/scratch are removed; small reports retain the
original failures and corrected observations. No large new captures or binaries
are retained. Full certification remains zero of63 weapons.
'''
    doc.write_text(text)
    print(json.dumps(dict(passed=True, matched_hand_samples=745, original_hud_drop_fault_fixed=True,
                         full_weapon_parity=False)), flush=True)


if __name__ == '__main__':
    main()
