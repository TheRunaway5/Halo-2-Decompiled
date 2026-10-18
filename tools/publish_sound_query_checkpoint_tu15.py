#!/usr/bin/env python3
"""Publish the literal sound metadata query port and its bounded validation."""
import datetime
import json
import os
from pathlib import Path
import shutil
import subprocess
from original_xbox_pose_case import BASE, ROOT, sha


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def main():
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    names = ['smg-original-sound-query-entry-v1/report.json',
             'smg-sound-query-differential-v1/report.json',
             'smg-sound-query-differential-v2/report.json',
             'smg-sound-query-cmake-v1/report.json',
             'smg-sound-query-sanitizers-v1/report.json',
             'smg-sound-query-world-compile-v1/report.json',
             'smg-baseline-native-firing-frontier-v4/report.json']
    prefix, raw, native, release, sanitized, world, frontier = [json.loads((BASE / name).read_text()) for name in names]
    assert not prefix['passed'] and prefix['declared_prefix_stop_reached'] and prefix['declared_prefix_stop'] == 0x1296e0
    assert not raw['passed'] and len(raw['comparisons']) == 49
    assert all(row['native']['gp'] == row['original']['gp'] and row['native']['trace'] == row['original']['trace'] for row in raw['comparisons'])
    assert native['passed'] and native['fixtures'] == 49 and native['unique_original_instructions_executed'] == 27
    assert all(row['equal'] and row['raw_control_xor'] == 0x40 for row in native['comparisons'])
    assert release['passed'] and sanitized['passed'] and world['passed']
    assert frontier['diagnostic_completed'] and not frontier['full_native_call_returned']
    assert frontier['last_external_calls'][-1] == dict(instruction=0x129846, target=0x127b20, tail=False)
    assert not frontier['actual_resource_reads']
    assert sha(ROOT / 'src/source_sound_cache_query_tu15_generated.cpp') == native['native_generated_source_sha256']
    for report in (native, world):
        for path, pin in report['source_files_sha256'].items():
            assert sha(path) == pin, path
    checks = []
    for name in names:
        record = json.loads((BASE / name).read_text())
        path = Path(record['build']).parent if record.get('build') else Path(record['runner_scratch'])
        assert path.is_relative_to(ROOT / '.local/test-scratch') and not path.exists(), path
        checks.append(dict(path=str(path), report=name, removed=True))
    prior_audit = json.loads((BASE / 'post-weapon-runtime-storage-audit-v1.json').read_text())
    assert prior_audit['publisher_scratch_removed_after_runner_exit']
    assert all(not Path(row['path']).exists() for row in prior_audit['scratch_checks'])
    now = datetime.datetime.now(datetime.timezone.utc).isoformat()
    production = [ROOT / name for name in ('src/source_sound_cache_query_tu15_generated.cpp',
        'include/halo2/source_sound_cache_query_tu15.hpp', 'src/physics_live_world_tu15.cpp',
        'cmake/PhysicsLiveWorldTu15.cmake', 'tools/generate_sound_cache_query_tu15.py')]
    result = dict(passed=True, timestamp_utc=now, complete_original_metadata_predicate=0x1296e0,
        original_call_site=0x129833, original_full_cfg_instructions=27, native_query_fixtures=49,
        actual_original_input_cases=1, declared_branch_input_cases=48,
        all_GP_defined_flags_XMM_nonreserved_FP_controls_and_instruction_traces_equal=True,
        raw_x87_control_difference=dict(original=575, native=639, xor=64, reserved_bit=6,
            existing_measured_authority='first-person-original-call-v1-native-evidence-v1.json'),
        all_original_query_instructions_executed=True, query_source_writes_or_external_returns=0,
        current_world_dispatch_wired_and_syntax_checked=True, component_release_and_ASAN_UBSAN_passed=True,
        earlier_full_application_checkpoint='checkpoint-weapon-runtime-20261018.json',
        full_application_relinked_after_this_query_port=False,
        native_firing_frontier=dict(site=0x129846, target=0x127b20, returned=False,
            library_provenance='Existing build-multiplayer archives plus freshly compiled query and cache-I/O source; not a fresh whole application'),
        complete_original_or_native_firing_verified=False, sound_admission_or_PCM_verified=False,
        source_files_sha256={str(path): sha(path) for path in production},
        evidence={name: sha(BASE / name) for name in names},
        large_artifacts_retained=[], physical_xbox_verified=False, full_weapon_parity=False)
    summary = BASE / 'sound-query-original-native-progress-v1.json'
    write(summary, result)
    previous = BASE / 'weapon-completion-matrix-v45.json'
    matrix = json.loads(previous.read_text())
    weapon = next(w for w in matrix['weapons'] if w['name'] == 'objects\\weapons\\rifle\\smg\\smg')
    weapon['bounded_evidence'].append(dict(report=summary.name, sha256=sha(summary),
        component='Original1296E0 complete27-instruction metadata predicate; native matches49 cases, Release/ASAN/UBSAN and world syntax pass. Baseline-library firing reaches the next unowned127B20 without supplied game returns.',
        certifies_entire_gate=False, certifies_all_map_variants=False,
        full_application_relinked_after_query_port=False, physical_xbox_verified=False))
    matrix['previous_matrix_sha256'] = sha(previous)
    matrix['evidence_update_tool_sha256'] = sha(Path(__file__))
    assert (matrix['weapon_paths'], matrix['authored_rows'], matrix['maps']) == (63, 1110, 38)
    assert len(matrix['weapons']) == 63 and sum(len(w['gates']) for w in matrix['weapons']) == 630
    assert matrix['fully_certified_weapons'] == 0 and not matrix['full_weapon_parity'] and matrix['cleanup_required_for_completion']
    write(BASE / 'weapon-completion-matrix-v46.json', matrix)
    audit = dict(passed=True, timestamp_utc=now, scratch_checks=checks,
        previous_checkpoint_scratch_paths_verified_absent=True,
        previous_storage_audit='post-weapon-runtime-storage-audit-v1.json',
        no_new_persistent_builds_objects_libraries_executables_or_raw_banks=True,
        original_assets_existing_reference_evidence_frozen_snapshots_and_selected_runtimes_preserved=True,
        explicitly_retained_large_artifacts=[],
        existing_required_reference_retention='original-weapon-reference-retention-v1.json',
        publisher_scratch=str(scratch), publisher_cleanup_pending_runner_exit=True,
        cleanup_required_for_completion=True, full_weapon_parity=False)
    write(BASE / 'post-sound-query-storage-audit-v1.json', audit)
    checkpoint = dict(passed=True, matrix='weapon-completion-matrix-v46.json',
        matrix_sha256=sha(BASE / 'weapon-completion-matrix-v46.json'),
        evidence=summary.name, evidence_sha256=sha(summary),
        storage_audit='post-sound-query-storage-audit-v1.json',
        previous_checkpoint='checkpoint-weapon-runtime-20261018.json',
        production_sound_query_changed=True, cleanup_required_for_completion=True,
        fully_certified_weapons=0, full_weapon_parity=False)
    write(BASE / 'checkpoint-sound-query-20261018.json', checkpoint)
    shutil.copyfile(Path(__file__), BASE / 'publish-sound-query-v46-source.py')
    path = BASE / 'storage-cleanup-current-20261017.json'
    current = json.loads(path.read_text())
    current.update(timestamp_utc=now, latest_weapon_completion_matrix=checkpoint['matrix'],
        latest_storage_audit=checkpoint['storage_audit'], latest_storage_audit_sha256=sha(BASE / checkpoint['storage_audit']),
        latest_current_required_input_check='checkpoint-sound-query-20261018.json',
        current_allocated_bytes=int(subprocess.check_output(['du', '-s', '-B1', str(BASE)], text=True).split()[0]))
    current['folder_allocated_size_reduction_bytes'] = current['original_baseline_allocated_bytes'] - current['current_allocated_bytes']
    current['folder_allocated_size_reduction_percent'] = round(100 * current['folder_allocated_size_reduction_bytes'] / current['original_baseline_allocated_bytes'], 3)
    write(path, current)
    print(json.dumps(checkpoint | dict(publisher_scratch=str(scratch), current_goal_allocated_bytes=current['current_allocated_bytes'])), flush=True)


if __name__ == '__main__':
    main()
