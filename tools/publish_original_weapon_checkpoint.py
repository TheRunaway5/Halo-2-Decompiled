#!/usr/bin/env python3
"""Publish bounded original weapon evidence with verified storage cleanup."""
import datetime
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
from original_xbox_pose_case import BASE, ROOT, physical_ram, sha
from xbox_tu15_reference_storage import verify_reference_disk


def write(path, data):
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + '\n')


def allocated(path):
    return int(subprocess.check_output(['du', '-s', '-B1', str(path)], text=True).split()[0])


def main():
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    now = datetime.datetime.now(datetime.timezone.utc).isoformat()
    plasma = 'plasma-original-charge-start-v1'
    smg = 'smg-original-fire-v1'
    reports = [plasma + '-replay-v1/report.json', plasma + '-native-v1/report.json',
               plasma + '-host-v1/report.json', smg + '/call.json', smg + '-replay-v1/report.json',
               smg + '-kernel-boundary-v2/report.json', smg + '-kernel-boundary-v2/kernel-api-name.json',
               smg + '-kernel-prefix-v3/report.json',
               'xemu-tu15-plasma-reference-v2/reference-disk-retention.json',
               'smg-reference-compressed-restore-v3/census/report.json']
    oracle = json.loads((BASE / reports[0]).read_text())
    native = json.loads((BASE / reports[1]).read_text())
    host = json.loads((BASE / reports[2]).read_text())
    failed = json.loads((BASE / reports[4]).read_text())
    kernel = json.loads((BASE / reports[7]).read_text())
    assert oracle['passed'] and host['passed'] and not native['passed'] and not failed['passed']
    assert oracle['instruction_count'] == host['native_instructions'] == 705
    assert host['native_ordered_stores'] == 92 and host['native_written_bytes'] == 303
    assert {row['kind'] for row in native['errors']} == {'x87_control', 'x87_status'}
    assert failed['exit'] == 0x80020f5a and failed['instruction_count'] == 43941
    assert kernel['kernel_instructions'] == 59 and kernel['kernel_ordered_stores'] == 27
    assert kernel['stop']['address'] == 0x8001436f and kernel['stop']['mnemonic'] == 'cli'
    assert not kernel['kernel_return_reached'] and not kernel['supplied_gameplay_or_kernel_returns']
    body = BASE / smg / 'before-weapon-body.bin'
    assert int.from_bytes(body.read_bytes()[0x22c:0x22e], 'little') == 60
    assert int.from_bytes(body.with_name('after-weapon-body.bin').read_bytes()[0x22c:0x22e], 'little') == 59
    for name in (reports[0], reports[1], reports[2], reports[4]):
        data = json.loads((BASE / name).read_text())
        for key in ('case_files_sha256', 'retained_files_sha256'):
            for path, digest in data.get(key, {}).items():
                item = Path(path) if Path(path).is_absolute() else (BASE / name).parent / path
                assert sha(item) == digest, item

    folders = sorted(p for p in BASE.iterdir() if p.is_dir() and p.name.startswith((
        'plasma-original-', 'smg-original-', 'xemu-tu15-plasma-reference-', 'smg-reference-compressed-')))
    checks, seen, processes = [], set(), []
    for folder in folders:
        for path in folder.rglob('*.json'):
            data = json.loads(path.read_text())
            if not isinstance(data, dict):
                continue
            value = data.get('runner_scratch')
            if value and value not in seen:
                seen.add(value)
                assert not Path(value).exists(), value
                checks.append(dict(report=str(path.relative_to(BASE)), path=value, removed=True))
            if path.name == 'launch.json':
                for key in ('pid', 'xvfb_pid'):
                    if key in data:
                        assert not Path('/proc', str(data[key])).exists()
                        processes.append(data[key])
        for path in folder.rglob('*'):
            if path.is_file():
                with path.open('rb') as source:
                    assert source.read(4) != b'\x7fELF', path
                assert path.stat().st_size < 4 * 1024 * 1024, path

    # Completed immutable copies can share storage while preserving every
    # evidence pathname and exact historical source hash.
    grouped, coalesced = {}, []
    for folder in folders:
        for path in sorted(folder.rglob('*')):
            if not path.is_file() or path.is_symlink() or not (
                path.suffix in ('.py', '.cpp', '.bin') or path.name in ('instructions.json', 'writes.json')):
                continue
            key = (sha(path), path.stat().st_size, path.stat().st_mode & 0o777)
            if key not in grouped:
                grouped[key] = path
                continue
            canonical = grouped[key]
            if path.stat().st_ino == canonical.stat().st_ino:
                continue
            temporary = scratch / 'coalesced-evidence'
            os.link(canonical, temporary)
            os.replace(temporary, path)
            assert sha(path) == key[0]
            coalesced.append(dict(path=str(path.relative_to(BASE)), canonical=str(canonical.relative_to(BASE)),
                                  sha256=key[0], bytes=key[1]))

    references, chunks = [], {}
    spec = importlib.util.spec_from_file_location('reference_content', BASE / 'archive-content-retention-v1.py')
    content = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(content)
    for name in (plasma, smg):
        index = json.loads((BASE / name / 'physical-memory-retention.json').read_text())
        for label, record in index['files'].items():
            with physical_ram(BASE / name, label) as memory:
                assert hashlib.sha256(memory).hexdigest() == record['raw_sha256']
            references.append(dict(case=name, label=label, **record))
    session = BASE / 'xemu-tu15-plasma-reference-v2'
    evidence = json.loads((session / 'reference-evidence.json').read_text())
    disk = json.loads((session / 'reference-disk-retention.json').read_text())
    assert disk['compressed_only_VM_restore_verified'] and not (session / 'session-hdd.qcow2').exists()
    assert verify_reference_disk(session, evidence) == disk['raw_sha256']
    assert content.reconstruct(disk['raw_sha256']) == disk['raw_bytes']
    assert sha(disk['restore_smoke_report']) == disk['restore_smoke_report_sha256']
    for record in references + [disk]:
        recipe = Path(record['content_recipe'])
        assert sha(recipe) == record['content_recipe_sha256']
        for chunk in json.loads(recipe.read_text())['chunks']:
            path = content.content_path('chunks', chunk['sha256'], '.z')
            chunks[str(path)] = dict(path=str(path.relative_to(BASE)), compressed_bytes=path.stat().st_size,
                                     raw_sha256=chunk['sha256'])
    retention = dict(passed=True, reason='Required original live weapon entry/exit RAM and normally played original VM reference',
        references=references, original_VM_manifest=str((session / 'reference-disk-retention.json').relative_to(BASE)),
        raw_RAM_reference_bytes=sum(r['raw_bytes'] for r in references), raw_VM_reference_bytes=disk['raw_bytes'],
        shared_chunks=list(chunks.values()), referenced_unique_compressed_chunk_bytes=sum(c['compressed_bytes'] for c in chunks.values()),
        all_exact_reconstructions_verified=True, compressed_only_VM_restore_verified=True,
        persistent_raw_RAM_dumps=False, persistent_raw_VM_disk_copy=False, full_weapon_parity=False)
    write(BASE / 'original-weapon-reference-retention-v1.json', retention)

    # This invocation owns the newly downloaded OCR cache, whose source recipe
    # remains available. Completed menu diagnostics and actual tag observations
    # do not require keeping the download. Preserve all unrelated tool inputs.
    model = ROOT / '.tools/tesseract-eng/eng.traineddata'
    source = json.loads(model.with_name('source.json').read_text())
    for process in Path('/proc').iterdir():
        if process.name.isdigit():
            try:
                assert (process / 'exe').readlink().name != 'tesseract'
            except (FileNotFoundError, PermissionError, ProcessLookupError):
                pass
    retired = dict(path=str(model), bytes=source['bytes'], sha256=source['sha256'],
                   source_recipe=str(model.with_name('source.json')), no_live_OCR_processes=True)
    if model.exists():
        assert sha(model) == source['sha256'] and model.stat().st_size == source['bytes']
        model.unlink()
    assert not model.exists()
    write(BASE / 'retire-unused-menu-OCR-cache-v1.json', dict(passed=True, retired=retired,
        reports_and_actual_tag_reference_evidence_preserved=True, full_weapon_parity=False))

    previous = BASE / 'weapon-completion-matrix-v40.json'
    matrix = json.loads(previous.read_text())
    plasma_weapon = next(w for w in matrix['weapons'] if w['name'] == 'objects\\weapons\\pistol\\plasma_pistol\\plasma_pistol')
    smg_weapon = next(w for w in matrix['weapons'] if w['name'] == 'objects\\weapons\\rifle\\smg\\smg')
    for weapon, names, description in (
        (plasma_weapon, reports[:3], 'Actual initial primary-input routing through1030F0:705 instructions,92 stores and303 bytes; native equals unchanged original host execution, emulator raw FP fields differ'),
        (smg_weapon, reports[3:8], 'Actual normally played SMG1030F0 firing60-to59 captured; independent replay stops at original kernel wait80020F5A, followed diagnostically through59 kernel instructions to CLI8001436F; no kernel return supplied')):
        for name in names:
            weapon['bounded_evidence'].append(dict(report=name, sha256=sha(BASE / name), component=description,
                map='assets/disc/maps/lockout.map', certifies_entire_gate=False, certifies_all_map_variants=False,
                complete_current_root_runtime_verified=False, physical_xbox_verified=False))
        gate = weapon['gates']['trigger_resources']
        gate['status'] = 'partial'
        gate['remaining'] = description + '. Complete native input/resource history, charge/release/reload/interruptions, authored variants and full gameplay/render/audio sequence remain required.'
    matrix['previous_matrix_sha256'] = sha(previous)
    matrix['evidence_update_tool_sha256'] = sha(Path(__file__))
    assert matrix['weapon_paths'] == 63 and matrix['authored_rows'] == 1110 and matrix['maps'] == 38
    assert len(matrix['weapons']) == 63 and sum(len(w['gates']) for w in matrix['weapons']) == 630
    assert matrix['fully_certified_weapons'] == 0 and not matrix['full_weapon_parity'] and matrix['cleanup_required_for_completion']
    write(BASE / 'weapon-completion-matrix-v41.json', matrix)
    audit = dict(passed=True, timestamp_utc=now, goal_cleanup_contract='goal-completion-contract-20261018.json',
        scratch_checks=checks, all_owned_reference_processes_terminal=True, checked_terminal_PIDs=processes,
        new_persistent_compiler_objects_or_executables=0, new_persistent_CMake_build_trees=0,
        new_original_reference_retention='original-weapon-reference-retention-v1.json',
        required_reference_unique_compressed_chunk_bytes=retention['referenced_unique_compressed_chunk_bytes'],
        raw_VM_copy_retired_bytes=disk['raw_bytes'], unused_OCR_cache_retired_bytes=retired['bytes'],
        evidence_paths_and_bytes_preserved=True, coalesced_copies=coalesced,
        unrelated_active_test_work_preserved=True, publisher_scratch=str(scratch),
        publisher_scratch_cleanup_pending_runner_exit=True, full_weapon_parity=False)
    write(BASE / 'post-original-weapon-storage-audit-v1.json', audit)
    checkpoint = dict(passed=True, timestamp_utc=now, full_weapon_parity=False, fully_certified_weapons=0,
        matrix='weapon-completion-matrix-v41.json', matrix_sha256=sha(BASE / 'weapon-completion-matrix-v41.json'),
        evidence={name: sha(BASE / name) for name in reports},
        retention='original-weapon-reference-retention-v1.json', retention_sha256=sha(BASE / 'original-weapon-reference-retention-v1.json'),
        storage_audit='post-original-weapon-storage-audit-v1.json', cleanup_required_for_completion=True,
        production_game_code_changed=False, complete_current_root_runtime_verified=False, physical_xbox_verified=False)
    write(BASE / 'checkpoint-original-weapon-20261018.json', checkpoint)
    current_path = BASE / 'storage-cleanup-current-20261017.json'
    current = json.loads(current_path.read_text())
    current.update(timestamp_utc=now, latest_weapon_completion_matrix=checkpoint['matrix'],
        latest_storage_audit=checkpoint['storage_audit'], latest_storage_audit_sha256=sha(BASE / checkpoint['storage_audit']),
        latest_current_required_input_check='checkpoint-original-weapon-20261018.json',
        current_original_weapon_reference_retention=checkpoint['retention'],
        archive_content_store_allocated_bytes=allocated(BASE / 'retained-archive-content-v1'), current_allocated_bytes=allocated(BASE))
    current['folder_allocated_size_reduction_bytes'] = current['original_baseline_allocated_bytes'] - current['current_allocated_bytes']
    current['folder_allocated_size_reduction_percent'] = round(100 * current['folder_allocated_size_reduction_bytes'] / current['original_baseline_allocated_bytes'], 3)
    write(current_path, current)
    print(json.dumps(dict(passed=True, matrix=checkpoint['matrix'], fully_certified_weapons=0,
        scratch_paths_removed=len(checks), raw_VM_copy_retired_bytes=disk['raw_bytes'],
        unused_OCR_cache_retired_bytes=retired['bytes'], retained_unique_compressed_reference_bytes=retention['referenced_unique_compressed_chunk_bytes'],
        current_goal_allocated_bytes=current['current_allocated_bytes'], publisher_scratch=str(scratch))), flush=True)


if __name__ == '__main__':
    main()
