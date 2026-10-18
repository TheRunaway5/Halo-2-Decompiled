#!/usr/bin/env python3
"""Publish exact scalar-query evidence, completed cleanup and a live relink."""
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
    now = datetime.datetime.now(datetime.timezone.utc).isoformat()
    names = ['smg-original-sound-scalar-entry-v1/report.json', 'smg-sound-scalar-differential-v1/report.json',
        'smg-sound-metadata-cmake-v1/report.json', 'smg-sound-metadata-sanitizers-v1/report.json',
        'smg-sound-scalar-world-compile-v1/report.json', 'smg-baseline-native-firing-frontier-v5/report.json',
        'smg-baseline-native-firing-frontier-v6/report.json', 'smg-original-listener-entry-v1/report.json']
    prefix, native, release, sanitized, world, frontier, routed, listener = [json.loads((BASE / name).read_text()) for name in names]
    assert not prefix['passed'] and prefix['declared_prefix_stop_reached'] and prefix['declared_prefix_stop'] == 0x127b20
    assert native['passed'] and native['fixtures'] == 31 and native['unique_original_instructions_executed'] == 40
    assert native['complete_original_bodies'] == 2 and native['successful_original_cases'] == 30
    assert native['real_missing_context_fault_cases'] == 1 and all(row['equal'] for row in native['comparisons'])
    assert release['passed'] and sanitized['passed'] and world['passed']
    assert not frontier['passed'] and not routed['passed']
    assert routed['last_external_calls'][-1] == dict(instruction=0x12985c, target=0x12a810, tail=False)
    assert not listener['passed'] and listener['declared_prefix_stop_reached'] and listener['declared_prefix_stop'] == 0x12a810
    actual = json.loads((BASE / 'smg-baseline-native-firing-frontier-v6/native-observations.json').read_text())
    original_cpu = listener['actual_CPU']
    assert actual['CPU']['gp'] == original_cpu['gp'] and actual['CPU']['xmm'] == original_cpu['xmm']
    assert actual['CPU']['eflags'] & 0xcd5 == original_cpu['eflags'] & 0xcd5
    assert actual['CPU']['x87_status'] ^ original_cpu['x87_status'] == 0x20
    assert actual['CPU']['x87_control'] ^ original_cpu['x87_control'] == 0x40
    assert not actual['actual_resource_reads'] and not actual['supplied_gameplay_returns']
    generated = ROOT / 'src/source_sound_scalar_tu15_generated.cpp'
    assert sha(generated) == native['native_generated_source_sha256']
    for path, pin in world['source_files_sha256'].items():
        assert sha(path) == pin, path
    checks = []
    for name in names:
        record = json.loads((BASE / name).read_text())
        path = Path(record['build']).parent if record.get('build') else Path(record['runner_scratch'])
        assert path.is_relative_to(ROOT / '.local/test-scratch') and not path.exists(), path
        checks.append(dict(path=str(path), report=name, removed=True))
    prior = json.loads((BASE / 'post-sound-query-storage-audit-v1.json').read_text())
    assert prior['publisher_scratch_removed_after_runner_exit'] and not Path(prior['publisher_scratch']).exists()
    ongoing_path = BASE / 'current-root-sound-metadata-runtime-build-v1/report.json'
    ongoing = json.loads(ongoing_path.read_text())
    assert ongoing['status'] == 'running' and Path(ongoing['build']).exists()
    live = []
    for process in Path('/proc').iterdir():
        if not process.name.isdigit():
            continue
        try:
            args = (process / 'cmdline').read_bytes().split(b'\0')
            if args and Path(os.fsdecode(args[0])).name == 'cmake' and os.fsencode(ongoing['build']) in args:
                os.kill(int(process.name), 0)
                live.append(dict(pid=int(process.name), argv=[os.fsdecode(arg) for arg in args if arg]))
        except (OSError, ProcessLookupError):
            pass
    assert live, 'Observe the existing build handle again; do not infer termination or restart it'
    job = dict(build_report=str(ongoing_path.relative_to(BASE)), build_session=87347,
        native_firing_probe_session=96521, finite_application_session=74534,
        observed_timestamp_utc=now, real_live_CMake_processes=live, private_build=ongoing['build'],
        scratch_cleanup_pending_terminal_result=True, keep_artifacts=False,
        original_entry_probe_report='smg-fresh-native-firing-frontier-v1/report.json',
        application_report='current-root-sound-metadata-application-v1/report.json')
    summary = dict(passed=True, timestamp_utc=now, complete_original_scalar_query=0x127b20,
        original_class_getter=0x228220, complete_original_cfg_instructions=40,
        exact_native_original_cases=31, original_unmodified_entry_cases=1, declared_byte_fixture_cases=30,
        successful_return_cases=30, original_null_context_fault_preserved_cases=1,
        all_GP_XMM_defined_flags_nonreserved_controls_traces_and_ordered_writes_equal=True,
        raw_scalar_patterns=native['raw_float_patterns'], signed_class_and_parameter_indices_checked=True,
        original_prefix_instructions=34802, original_prefix_stores=6768,
        original_prefix_is_not_a_successful_full_firing_call=True,
        release_component_CTests_passed=3, ASAN_UBSAN_component_CTests_passed=3,
        current_world_dispatch_wired_and_syntax_checked=True,
        pre_listener_boundary=dict(target=0x12a810, native_GP_XMM_defined_flags_equal=True,
            raw_original_x87_status=original_cpu['x87_status'], raw_native_x87_status=actual['CPU']['x87_status'],
            raw_status_xor=0x20, raw_control_xor=0x40,
            native_instruction_observations=len(actual['instructions']), original_instructions=listener['instruction_count'],
            canonical_damage_observer_PC_domain_requires_qualified_mapping=True,
            full_CPU_or_trace_parity=False, baseline_library_composition=True),
        complete_original_or_native_firing_verified=False, sound_admission_or_PCM_verified=False,
        source_or_expected_outcome_patches=[], evidence={name: sha(BASE / name) for name in names},
        current_whole_runtime_validation=job, current_whole_runtime_validation_complete=False,
        physical_xbox_verified=False, full_weapon_parity=False, new_large_reference_inputs=False)
    summary_path = BASE / 'sound-scalar-original-native-progress-v1.json'
    write(summary_path, summary)
    previous = BASE / 'weapon-completion-matrix-v46.json'
    matrix = json.loads(previous.read_text())
    weapon = next(row for row in matrix['weapons'] if row['name'] == 'objects\\weapons\\rifle\\smg\\smg')
    weapon['bounded_evidence'].append(dict(report=summary_path.name, sha256=sha(summary_path),
        component='Complete127B20 scalar query and228220 class getter: all40 instructions match across31 cases including exact float bits, signed indices, stack writes and actual null-context fault. Release/ASAN/UBSAN and world syntax pass. Full relink is live; listener owner12A810 and raw FP status remain unresolved.',
        certifies_entire_gate=False, certifies_all_map_variants=False, physical_xbox_verified=False))
    matrix['previous_matrix_sha256'] = sha(previous)
    matrix['evidence_update_tool_sha256'] = sha(Path(__file__))
    assert (matrix['weapon_paths'], matrix['authored_rows'], matrix['maps']) == (63, 1110, 38)
    assert len(matrix['weapons']) == 63 and sum(len(row['gates']) for row in matrix['weapons']) == 630
    assert not matrix['full_weapon_parity'] and matrix['fully_certified_weapons'] == 0 and matrix['cleanup_required_for_completion']
    write(BASE / 'weapon-completion-matrix-v47.json', matrix)
    audit = dict(passed=True, timestamp_utc=now, scratch_checks=checks, completed_validation_scratch_paths_removed=8,
        previous_checkpoint_scratch_removed=True, large_artifacts_retained=[], new_persistent_binaries_or_RAM=0,
        live_test_work=job, active_private_build_intentionally_preserved=True,
        existing_reference_evidence_original_assets_frozen_sources_runtimes_and_unrelated_active_work_preserved=True,
        existing_required_reference_retention='original-weapon-reference-retention-v1.json',
        publisher_scratch=str(scratch), publisher_cleanup_pending_runner_exit=True,
        cleanup_required_for_completion=True, full_weapon_parity=False)
    write(BASE / 'post-sound-scalar-storage-audit-v1.json', audit)
    checkpoint = dict(passed=True, matrix='weapon-completion-matrix-v47.json',
        matrix_sha256=sha(BASE / 'weapon-completion-matrix-v47.json'),
        evidence=summary_path.name, evidence_sha256=sha(summary_path),
        storage_audit='post-sound-scalar-storage-audit-v1.json', previous_checkpoint='checkpoint-sound-query-20261018.json',
        current_whole_runtime_validation=job, current_whole_runtime_validation_complete=False,
        cleanup_required_for_completion=True, fully_certified_weapons=0, full_weapon_parity=False)
    write(BASE / 'checkpoint-sound-scalar-20261018.json', checkpoint)
    shutil.copyfile(Path(__file__), BASE / 'publish-sound-scalar-v47-source.py')
    path = BASE / 'storage-cleanup-current-20261017.json'
    current = json.loads(path.read_text())
    current.update(timestamp_utc=now, latest_weapon_completion_matrix=checkpoint['matrix'],
        latest_storage_audit=checkpoint['storage_audit'], latest_storage_audit_sha256=sha(BASE / checkpoint['storage_audit']),
        latest_current_required_input_check='checkpoint-sound-scalar-20261018.json',
        current_allocated_bytes=int(subprocess.check_output(['du', '-s', '-B1', str(BASE)], text=True).split()[0]))
    current['folder_allocated_size_reduction_bytes'] = current['original_baseline_allocated_bytes'] - current['current_allocated_bytes']
    current['folder_allocated_size_reduction_percent'] = round(100 * current['folder_allocated_size_reduction_bytes'] / current['original_baseline_allocated_bytes'], 3)
    write(path, current)
    print(json.dumps(checkpoint | dict(publisher_scratch=str(scratch), current_goal_allocated_bytes=current['current_allocated_bytes'])), flush=True)


if __name__ == '__main__':
    main()
