#!/usr/bin/env python3
"""Publish bounded TU1.5 weapon-material evidence after validation and cleanup.

Run through tools/run_test.py. This does not close a weapon or completion gate.
"""
import datetime
import json
import os
from pathlib import Path
import re
import subprocess
from original_xbox_pose_case import BASE, ROOT, sha


def write(name,value):
    (BASE/name).write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')


def main():
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    validation=BASE/'weapon-material-input-snapshot-v2'
    complete=json.loads((validation/'report.json').read_text())
    assert complete['passed'] and complete['snapshot_source_files_unchanged']
    assert all(s['exit_code']==0 and s['private_build_removed'] for s in complete['steps'])
    assert not Path(complete['runner_scratch']).exists()
    phases={}
    for label in ('release','sanitizers'):
        child=json.loads((validation/label/'report.json').read_text())
        assert child['passed'] and not Path(child['build']).parent.exists()
        acceptance=json.loads((validation/label/'evidence/weapon-material-input-acceptance.json').read_text())
        assert acceptance['passed'] and acceptance['complete_original_authored_alias_cases']==25
        phases[label]=dict(report=str((validation/label/'report.json').relative_to(BASE)),
            sha256=sha(validation/label/'report.json'),acceptance=acceptance)
    scope_path=BASE/'weapon-material-input-scope-v2/report.json'
    scope=json.loads(scope_path.read_text())
    assert (scope['maps'],scope['authored_weapon_rows'],scope['builtin_one_model_material_references'])==(38,1110,192)
    oracle_path=BASE/'weapon-material-builtin-original-v4/report.json'
    oracle=json.loads(oracle_path.read_text())
    assert oracle['passed'] and oracle['fixture_sha256']==sha(ROOT/'tests/fixtures/weapon_material_builtins_tu15.json')
    comparison_path=BASE/'weapon-gauge-original-comparison-v2/report.json'
    comparison=json.loads(comparison_path.read_text())
    assert comparison['passed'] and comparison['compared_words']==188 and not comparison['native_different_words']
    assert comparison['legacy_different_words']==[19] and comparison['callback_is_unchanged_original']
    assert comparison['supplied_callee_results']==comparison['original_instruction_patches']==[]
    assert comparison['original_gauge_fixture_sha256']==sha(ROOT/'tests/fixtures/weapon_gauge_animation_tu15.json')
    assert json.loads((comparison_path.parent/'native-acceptance.json').read_text())['original_gauge_words_matched']==188
    assert not Path(comparison['runner_scratch']).exists()
    runtime_validation=BASE/'weapon-gauge-live-validation-v5'
    live_parent=json.loads((runtime_validation/'report.json').read_text())
    assert live_parent['passed'] and not Path(live_parent['runner_scratch']).exists()
    runtime_path=runtime_validation/'release/evidence/live-gauge-acceptance.json'
    runtime=json.loads(runtime_path.read_text())
    assert runtime['passed'] and runtime['actual_gauge_material_observations']>0
    assert runtime['component_scope']=='held_gauge_input' and not runtime['full_weapon_sequence_verified']
    live_component=json.loads((runtime_validation/'release/evidence/weapon-material-input-acceptance.json').read_text())
    assert live_component['gauge_reference_vectors']['native']==comparison['original_bank_words']
    owned_sources=['include/halo2/source_weapon_material_input.hpp','src/source_weapon_material_input.cpp']
    manifest=json.loads((validation/'source-manifest.json').read_text())['source_files_sha256']
    assert all(sha(ROOT/p)==manifest[p] for p in owned_sources)
    source_pins={p:sha(ROOT/p) for p in owned_sources+['tests/source_weapon_material_input_tests.cpp',
        'tests/fixtures/weapon_material_builtins_tu15.json','tests/fixtures/weapon_gauge_animation_tu15.json',
        'tools/oracle_weapon_material_builtins_tu15.py',
        'tools/compare_weapon_gauge_animation_tu15.py','tools/audit_weapon_material_inputs_tu15.py',
        'tools/validate_weapon_material_snapshot_tu15.py','tests/native_covenant_weapon_runtime_tests.cpp']}
    assert source_pins['tests/source_weapon_material_input_tests.cpp']==comparison['native_test_source_sha256']
    live_manifest=json.loads((runtime_validation/'source-manifest.json').read_text())['source_files_sha256']
    assert source_pins['tests/native_covenant_weapon_runtime_tests.cpp']==live_manifest['tests/native_covenant_weapon_runtime_tests.cpp']
    integration_source=dict(path='src/multiplayer_runtime.cpp',
        live_validation_sha256=live_manifest['src/multiplayer_runtime.cpp'],
        current_root_sha256=sha(ROOT/'src/multiplayer_runtime.cpp'),
        current_root_matches_live_snapshot=sha(ROOT/'src/multiplayer_runtime.cpp')==live_manifest['src/multiplayer_runtime.cpp'])
    paths=set()
    for pattern in ('weapon-material-*','weapon-gauge-*'):
        for directory in BASE.glob(pattern):
            if not directory.is_dir():continue
            for name in ('attempt.json','report.json','failure.json','early-command.json'):
                path=directory/name
                if not path.is_file():continue
                for found in re.findall(r'/home/eric/Developer/Halo-2-Decompiled/\.local/test-scratch/halo2-test-[A-Za-z0-9_-]+',path.read_text()):
                    paths.add(found)
            for path in directory.glob('*/report.json'):
                for found in re.findall(r'/home/eric/Developer/Halo-2-Decompiled/\.local/test-scratch/halo2-test-[A-Za-z0-9_-]+',path.read_text()):
                    paths.add(found)
    checks=[dict(path=p,removed=not Path(p).exists())for p in sorted(paths)]
    assert all(row['removed']for row in checks),[r for r in checks if not r['removed']]
    assert not (ROOT/'tools/probe_weapon_material_input_tu15.py').exists()
    audit_name='post-weapon-material-input-storage-audit-v1.json'
    audit=dict(passed=True,cleanup_required_for_completion=True,scratch_checks=checks,
        completed_validation_private_directories_removed=len(checks),live_owned_builds=[],
        publisher_scratch=str(scratch),publisher_cleanup_pending_runner_exit=True,
        retired_unused_helper='retired-material-input-probe-v1.json',large_artifacts_retained=[],
        retired_expired_shader_link_probe='retired-live-gauge-link-probe-v1.json',
        original_assets_reference_evidence_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True)
    write(audit_name,audit)
    progress_name='weapon-material-input-original-native-progress-v1.json'
    progress=dict(passed=True,scope=dict(weapon_paths=63,authored_rows=1110,maps=38,gates=630),
        production_change='Weapon material callbacks resolve original one/zero and authored functions through bound heat,age,ammunition and optional actual primary-trigger owners. Unknown inputs remain unresolved.',
        original_builtin_cases=2,original_complete_authored_alias_cases=25,
        mandatory_original_gauge_regression_words=188,
        actual_native_heat_alias_cases=9,builtin_one_authored_model_material_references=192,
        actual_live_gauge_material_observations=runtime['actual_gauge_material_observations'],
        actual_SDL_held_gauge_component_passed=True,private_X_server_used=True,
        full_firing_sequence_passed=False,full_firing_failure='weapon-gauge-live-validation-v4/release/tests.log',
        builtin_one_weapon_paths=scope['builtin_one_weapon_paths'],
        actual_gauge=dict(compared_words=188,native_different_words=[],legacy_different_words=[19],
            original_word19='9d6666ff',native_word19='9d6666ff',legacy_word19='9b6666ff',
            original_executed_instructions=comparison['original_instructions_executed'],
            original_object_callback_calls=comparison['original_adapter_calls'],
            original_overlay_calls=comparison['original_overlay_calls']),
        heat_alias_model_material_consumption_not_claimed=True,
        primary_trigger_authored_model_material_references=scope['primary_trigger_model_material_references'],
        validation_phases=phases,source_files_sha256=source_pins,integration_source=integration_source,
        diagnostic_test_serialization_expanded_after_snapshot_and_compiled_against_same_coherent_libraries=True,
        diagnostic_source_sha256=comparison['native_test_source_sha256'],
        references=[dict(report=str(p.relative_to(BASE)),sha256=sha(p))for p in
            (scope_path,oracle_path,comparison_path,runtime_path,runtime_validation/'report.json',
             validation/'report.json',validation/'source-manifest.json')],
        preserved_failures=['weapon-material-input-cmake-v1/report.json','weapon-material-input-snapshot-v1/early-failure.json',
            'weapon-material-input-snapshot-v1/gauge-authored-pass.json','retired-material-input-probe-v1.json',
            'weapon-gauge-live-runtime-v1/report.json','retired-live-gauge-link-probe-v1.json',
            'weapon-gauge-live-validation-v2/report.json','weapon-gauge-live-validation-v3/report.json',
            'weapon-gauge-live-validation-v4/report.json'],
        corrected_test_setup='Actual held-weapon material preparation includes the runtime explicit zero camouflage input.',
        live_fixture_uses_original_position_owner=True,live_fixture_forces_private_X11=True,
        private_validation_builds_removed=True,storage_audit=audit_name,
        original_callback_registration_lifecycle_verified=False,live_rendered_framebuffer_verified=False,
        physical_xbox_verified=False,full_weapon_parity=False,fully_certified_weapons=0,
        large_artifacts_retained=[])
    write(progress_name,progress)
    previous=BASE/'weapon-completion-matrix-v54.json'
    matrix=json.loads(previous.read_text())
    assert len(matrix['weapons'])==63 and sum(len(w['gates'])for w in matrix['weapons'])==630
    alias_names={row['weapon']for row in oracle['alias_cases']}
    for weapon in matrix['weapons']:
        if weapon['name'] in set(scope['builtin_one_weapon_paths'])|alias_names:
            weapon['bounded_evidence'].append(dict(component='Original weapon material builtins and authored aliases; actual plasma-rifle gauge banks match complete TU1.5 consumers and original callback.',
                report=progress_name,sha256=sha(BASE/progress_name),certifies_entire_gate=False,
                certifies_all_map_variants=False,component_covers_all_scoped_maps_for_weapon=False,
                exact_gauge_comparison_for_this_weapon=weapon['name']=='objects\\weapons\\rifle\\plasma_rifle\\plasma_rifle',
                original_authored_alias_cases_for_this_weapon=weapon['name'] in alias_names))
    matrix.update(previous_matrix_sha256=sha(previous),evidence_update_tool_sha256=sha(Path(__file__)))
    write('weapon-completion-matrix-v55.json',matrix)
    write('checkpoint-weapon-material-input-20261018.json',dict(passed=True,
        previous_checkpoint='checkpoint-original-host-weapon-validation-20261018.json',
        evidence=progress_name,evidence_sha256=sha(BASE/progress_name),matrix='weapon-completion-matrix-v55.json',
        matrix_sha256=sha(BASE/'weapon-completion-matrix-v55.json'),storage_audit=audit_name,
        cleanup_required_for_completion=True,full_weapon_parity=False,fully_certified_weapons=0,
        next_work='Continue complete weapon feature and rendered-reference sequences, live sound activation and DSP/PCM acceptance. Preserve the independent emulator first-person discrepancy and genuine platform boundaries.'))
    write('continuation-working-state-v55.json',dict(goal_status='active',
        latest_checkpoint='checkpoint-weapon-material-input-20261018.json',latest_matrix='weapon-completion-matrix-v55.json',
        progress=progress['production_change'],exact_original_gauge_words=188,
        original_gauge_instruction_observations=comparison['original_instructions_executed'],
        release_and_sanitizers_passed=True,all_owned_builds_and_probes_terminal=True,
        publisher_cleanup_pending_runner_exit=True,publisher_scratch=str(scratch),
        no_new_large_artifacts_retained=True,fully_certified_weapons=0,scope=progress['scope'],
        next_work='Continue full weapon feature and framebuffer/audio acceptance. Five paths have authored gauge one references; only the Lockout plasma-rifle gauge has this exact shader-bank comparison. Native live sound device activation and DSP/PCM remain open.'))
    document=ROOT/'docs/WEAPONS_TU15_PARITY_GOAL.md'
    text=document.read_text().replace('`weapon-completion-matrix-v54.json` expands','`weapon-completion-matrix-v55.json` expands',1)
    text+='''

## Original weapon material inputs and plasma-rifle gauge (October18)

The live held/body and world weapon material callbacks now resolve original
one/zero builtins and authored object functions through actual bound weapon
inputs. Enabled zero, age gates, authored curves and optional primary-trigger
state retain their source ownership; an unavailable producer stays unresolved.
Whole unchanged TU1.5 handlers substantiate two builtins and25 actual authored
heat/ammunition cases. Nine additional native cases retain actual heat gates.

The38-map metadata audit finds192 authored one references across five weapon
paths. The concrete Lockout plasma-rifle gauge comparison executes1789 original
instructions through the unchanged original shader consumers and callback.
All188 shader words match native output; the previous callback differs at
word19,9b6666ff instead of original/native9d6666ff. Its test prepares the actual
pass with the runtime explicit zero camouflage value. The initial test omitted
that required context and failed; its diagnostics remain retained.
The188 unchanged-original words are also retained as a small mandatory native
regression fixture, so matching only the previous callback is insufficient.

Release application validation, all six selected checks, finite120-tick one/four
player simulations and four sanitizer checks pass from one disposable coherent
source snapshot. A separate diagnostic serialization build uses the same copied
headers/libraries and records the complete gauge banks. All recorded private
build/probe directories are removed; no new large artifact is retained. The
focused actual SDL pickup/held-gauge probe passes on a private X11 display,
observing the corrected one input on live plasma-rifle gauge materials. Its
fixture delivers placement through original B95D0, rather than changing only
the simulation position overwritten by source physics readback.

The separate complete firing probe picks up the actual weapon at tick42 and
submits two attack poses, but fails its combined shot/sound observation gate.
No published shot_tick transition is observed after the real SDL fire input.
That failure remains preserved and unresolved; the held-gauge result does not
certify firing or sound. Source readback also retains the legacy last_shot
publication field; do not infer actual shot count solely from that field.

Metadata references do not prove all those model variants render correctly.
Original callback registration, framebuffer, physical Xbox and full audio
acceptance remain open. Matrixv55 retains all63 weapons and630 gates, with zero
fully certified weapons. See weapon-material-input-original-native-progress-v1.json,
checkpoint-weapon-material-input-20261018.json and
post-weapon-material-input-storage-audit-v1.json in the goal analysis folder.
'''
    document.write_text(text)
    storage=json.loads((BASE/'storage-cleanup-current-20261017.json').read_text())
    storage.update(latest_current_required_input_check='checkpoint-weapon-material-input-20261018.json',
        latest_storage_audit=audit_name,latest_storage_audit_sha256=sha(BASE/audit_name),
        latest_weapon_completion_matrix='weapon-completion-matrix-v55.json',
        current_allocated_bytes=int(subprocess.check_output(['du','-s','--block-size=1',str(BASE)],text=True).split()[0]),
        timestamp_utc=datetime.datetime.now(datetime.timezone.utc).isoformat())
    write('storage-cleanup-current-20261017.json',storage)
    print(json.dumps(dict(passed=True,matrix='v55',exact_gauge_words=188,completed_private_directories_removed=len(checks),publisher_scratch=str(scratch))),flush=True)


if __name__=='__main__':main()
