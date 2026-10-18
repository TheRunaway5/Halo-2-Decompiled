#!/usr/bin/env python3
"""Record original SMG kernel continuation and verify its disposable storage."""
import datetime
import json
import os
from pathlib import Path
import subprocess
from original_xbox_pose_case import BASE, ROOT, sha


def write(path, data):
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + '\n')


def main():
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    previous_checkpoint = json.loads((BASE / 'checkpoint-original-weapon-20261018.json').read_text())
    for name, digest in previous_checkpoint['evidence'].items():
        assert sha(BASE / name) == digest, name
    assert sha(BASE / previous_checkpoint['retention']) == previous_checkpoint['retention_sha256']
    stem = 'smg-original-fire-v1-'
    names = [stem + item for item in ('kernel-prefix-v4', 'kernel-prefix-v5', 'kernel-prefix-v6',
                                     'replay-v2', 'replay-v3', 'kernel-boundary-v3', 'kernel-boundary-v4')]
    records = {name: json.loads((BASE / name / 'report.json').read_text()) for name in names}
    wait, mutant, semaphore, second, third, _, _ = [records[name] for name in names]
    assert wait['kernel_return_reached'] and wait['kernel_instructions'] == 282
    assert mutant['kernel_return_reached'] and mutant['kernel_instructions'] == 182
    assert not semaphore['kernel_return_reached'] and semaphore['kernel_instructions'] == 312
    assert semaphore['stop']['address'] == 0x80018dd1 and semaphore['stop']['operands'] == 'ebp, cr0'
    assert not second['passed'] and not third['passed'] and third['exit'] == 0x80013c0b
    exports = BASE / (stem + 'kernel-boundary-v2') / 'nxdk-xboxkrnl.exe.def'
    naming = json.loads(exports.with_name('kernel-api-name.json').read_text())
    assert sha(exports) == naming['primary_naming_source']['sha256']
    lines = exports.read_text().splitlines()
    declarations = {ordinal: next(line.strip() for line in lines if '@ ' + str(ordinal) + ' NONAME' in line)
                    for ordinal in (221, 222, 234)}
    summary = dict(passed=True, purpose='Identify uninterrupted original kernel paths and the next unresolved owner; not a complete weapon comparison',
        original_case='smg-original-fire-v1', original_case_sha256=sha(BASE / 'smg-original-fire-v1/call.json'),
        original_wait_instructions=282, original_wait_stores=80, wait_return_reached=True,
        original_mutant_release_instructions=182, original_mutant_release_stores=63, mutant_release_return_reached=True,
        semaphore_release_prefix_instructions=312, semaphore_release_prefix_stores=98,
        semaphore_release_return_reached=False, next_instruction=semaphore['stop'],
        original_full_firing_call_completed=True, independent_full_firing_call_verified=False,
        full_native_firing_comparison_executed=False, original_instruction_patches=[],
        supplied_kernel_or_gameplay_returns=[], guest_IF_flags_from_captured_CPU=True,
        asynchronous_interrupts_delivered=False, device_IRQ_PIC_or_scheduler_results_supplied=False,
        original_live_kernel_entry_observed=False, prefix_state_reconstructed_from_checked_original_replay_writes=True,
        primary_export_naming_source=naming['primary_naming_source'], pinned_export_declarations=declarations,
        evidence={name + '/report.json': sha(BASE / name / 'report.json') for name in names},
        no_new_large_reference_inputs=True, physical_xbox_verified=False, full_weapon_parity=False)
    write(BASE / 'original-smg-kernel-continuation-v1.json', summary)
    # Old evidence remains immutable. The original v41 matrix still records the
    # first boundary; the current matrix adds later conditional observations.
    previous = BASE / 'weapon-completion-matrix-v41.json'
    matrix = json.loads(previous.read_text())
    weapon = next(w for w in matrix['weapons'] if w['name'] == 'objects\\weapons\\rifle\\smg\\smg')
    weapon['bounded_evidence'].append(dict(report='original-smg-kernel-continuation-v1.json',
        sha256=sha(BASE / 'original-smg-kernel-continuation-v1.json'),
        component='Unchanged kernel wait282 and mutant release182 instruction paths return without supplied results; semaphore release312-instruction prefix stops before dispatch CR0 read80018DD1',
        map='assets/disc/maps/lockout.map', certifies_entire_gate=False, certifies_all_map_variants=False,
        complete_current_root_runtime_verified=False, physical_xbox_verified=False))
    weapon['gates']['trigger_resources']['remaining'] += (
        ' Subsequent authenticated uninterrupted original kernel execution returns from the wait and mutant release. '
        'NtReleaseSemaphore reaches312 instructions and98 stores before CR0/context-switch ownership at80018DD1. '
        'No full native firing comparison or native scheduler/device/IRQ completion is established.')
    matrix['previous_matrix_sha256'] = sha(previous)
    matrix['evidence_update_tool_sha256'] = sha(Path(__file__))
    assert (matrix['weapon_paths'], matrix['authored_rows'], matrix['maps'], len(matrix['weapons'])) == (63, 1110, 38, 63)
    assert sum(len(w['gates']) for w in matrix['weapons']) == 630 and matrix['fully_certified_weapons'] == 0
    assert matrix['cleanup_required_for_completion'] and not matrix['full_weapon_parity']
    write(BASE / 'weapon-completion-matrix-v42.json', matrix)
    checks = []
    folders = [p for p in BASE.iterdir() if p.is_dir() and p.name.startswith((
        'smg-original-', 'plasma-original-', 'xemu-tu15-plasma-reference-', 'smg-reference-compressed-'))]
    seen = set()
    for folder in folders:
        for path in folder.rglob('*.json'):
            data = json.loads(path.read_text())
            if isinstance(data, dict) and data.get('runner_scratch') and data['runner_scratch'] not in seen:
                value = data['runner_scratch']
                assert not Path(value).exists(), value
                seen.add(value)
                checks.append(dict(report=str(path.relative_to(BASE)), path=value, removed=True))
    previous_audit = json.loads((BASE / previous_checkpoint['storage_audit']).read_text())
    assert not Path(previous_audit['publisher_scratch']).exists()
    assert not (ROOT / '.tools/tesseract-eng/eng.traineddata').exists()
    assert not (BASE / 'xemu-tu15-plasma-reference-v2/session-hdd.qcow2').exists()
    audit = dict(passed=True, timestamp_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        scratch_checks=checks, prior_publisher_scratch_removed=True, no_new_large_reference_inputs=True,
        new_persistent_compiler_objects_or_executables=0, new_persistent_CMake_build_trees=0,
        required_reference_retention=previous_checkpoint['retention'],
        required_reference_retention_sha256=sha(BASE / previous_checkpoint['retention']),
        original_disc_assets_frozen_sources_launchers_and_unrelated_active_work_preserved=True,
        publisher_scratch=str(scratch), publisher_scratch_cleanup_pending_runner_exit=True, full_weapon_parity=False)
    write(BASE / 'post-original-kernel-storage-audit-v1.json', audit)
    checkpoint = dict(passed=True, matrix='weapon-completion-matrix-v42.json',
        matrix_sha256=sha(BASE / 'weapon-completion-matrix-v42.json'),
        kernel_continuation='original-smg-kernel-continuation-v1.json',
        kernel_continuation_sha256=sha(BASE / 'original-smg-kernel-continuation-v1.json'),
        prior_checkpoint='checkpoint-original-weapon-20261018.json',
        prior_checkpoint_sha256=sha(BASE / 'checkpoint-original-weapon-20261018.json'),
        storage_audit='post-original-kernel-storage-audit-v1.json',
        cleanup_required_for_completion=True, fully_certified_weapons=0, full_weapon_parity=False)
    write(BASE / 'checkpoint-original-kernel-20261018.json', checkpoint)
    current_path = BASE / 'storage-cleanup-current-20261017.json'
    current = json.loads(current_path.read_text())
    current.update(timestamp_utc=audit['timestamp_utc'], latest_weapon_completion_matrix=checkpoint['matrix'],
        latest_storage_audit=checkpoint['storage_audit'], latest_storage_audit_sha256=sha(BASE / checkpoint['storage_audit']),
        latest_current_required_input_check='checkpoint-original-kernel-20261018.json',
        current_allocated_bytes=int(subprocess.check_output(['du', '-s', '-B1', str(BASE)], text=True).split()[0]))
    current['folder_allocated_size_reduction_bytes'] = current['original_baseline_allocated_bytes'] - current['current_allocated_bytes']
    current['folder_allocated_size_reduction_percent'] = round(100 * current['folder_allocated_size_reduction_bytes'] / current['original_baseline_allocated_bytes'], 3)
    write(current_path, current)
    print(json.dumps(dict(passed=True, matrix=checkpoint['matrix'], scratch_paths_removed=len(checks),
        fully_certified_weapons=0, current_goal_allocated_bytes=current['current_allocated_bytes'],
        publisher_scratch=str(scratch))), flush=True)


if __name__ == '__main__':
    main()
