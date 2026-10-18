#!/usr/bin/env python3
"""Publish bounded original paging, worker and native file-read evidence."""
import datetime
import json
import os
from pathlib import Path
import subprocess
from original_xbox_pose_case import BASE, ROOT, sha


def write(path,value):path.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')


def main():
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);now=datetime.datetime.now(datetime.timezone.utc).isoformat()
    reports=['original-weapon-cpu-paging-equivalence-v1/report.json',
        'smg-original-fire-v1-kernel-prefix-v7/report.json','smg-original-fire-v1-kernel-prefix-v8/report.json',
        'smg-original-fire-v1-kernel-prefix-v9/report.json','smg-original-fire-v1-replay-v7/report.json',
        'smg-original-fire-v1-kernel-boundary-v5/report.json','smg-original-fire-v1-kernel-boundary-v6/report.json',
        'smg-original-fire-v1-io-request-v3/report.json','smg-original-file-read-native-v2/report.json']
    paging,dispatch,seek,device,replay,_,_,request,native=[json.loads((BASE/name).read_text())for name in reports]
    assert paging['passed']and native['passed']and not replay['passed']
    assert [(r['instructions'],r['source_written_bytes'])for r in paging['cases']]==[(705,303),(5722,4608)]
    assert dispatch['architectural_CPU_paging']and dispatch['kernel_instructions']==408
    original_dispatch=json.loads((BASE/'smg-original-fire-v1-replay-v3/report.json').read_text())
    assert dispatch['source_oracle_sha256']==sha(BASE/'smg-original-fire-v1-replay-v3/report.json')
    assert dispatch['final_integer_CPU']['gp'][4]!=original_dispatch['actual_CPU']['gp'][4]
    assert dispatch['final_integer_CPU']['eip']==0x2e0385 and dispatch['kernel_return_address']==0x2e02d6
    assert seek['kernel_return_reached']and seek['kernel_instructions']==153
    assert device['kernel_instructions']==902 and device['stop']['mnemonic']=='out'
    assert device['stop']['address']==0x8002400d
    assert device['final_integer_CPU']['gp'][2]&0xffff==0x1f6
    assert device['final_integer_CPU']['gp'][0]&255==0xa0
    assert request['seek_offset']==79681536 and request['bytes']==47616
    assert native['exact_original_exit_buffer']and native['exact_original_task_exit_observed']and native['actual_native_IO_matches_request']
    assert not native['native_worker_queue_or_kernel_device_binding_installed']and not native['I_O_completion_timeline_verified']
    for name,digest in native['source_files_sha256'].items():
        assert sha(name)==digest and sha(BASE/'smg-original-file-read-native-v2'/Path(name).name)==digest
    abi=BASE/'smg-original-fire-v1-io-request-v3/nxdk-xboxkrnl.h'
    assert sha(abi)==request['primary_ABI_source']['sha256']
    for row in request['owned_map_content_matches']:assert sha(row['path'])==row['whole_file_sha256']
    previous_checkpoint=BASE/'checkpoint-original-kernel-20261018.json'
    previous=json.loads(previous_checkpoint.read_text());assert sha(BASE/previous['matrix'])==previous['matrix_sha256']
    summary=dict(passed=True,timestamp_utc=now,paging_equivalence=paging['cases'],
        reconstructed_kernel_dispatch_instructions=408,kernel_file_position_instructions=153,
        original_read_file_prefix_instructions=902,original_read_file_boundary=device['stop'],
        stopped_output_port=0x1f6,stopped_output_byte=0xa0,device_output_executed=False,
        native_read_bytes=47616,native_read_offset=79681536,
        native_read_original_buffer_equal=True,native_task_words_match_original_exit_observation=True,
        native_production_sources_freshly_compiled=True,
        native_file_binding='Explicit procedure binding to shared.map, selected by unique exact original exit-buffer content match',
        actual_queue_node_identity_verified=False,native_worker_queue_or_kernel_device_binding_installed=False,
        actual_IO_completion_timeline_verified=False,full_original_call_independently_verified=False,
        evidence={name:sha(BASE/name)for name in reports},new_large_reference_inputs=False,
        supplied_kernel_gameplay_IO_or_scheduler_results=[],original_instruction_patches=[],
        physical_xbox_verified=False,full_weapon_parity=False)
    write(BASE/'original-weapon-io-progress-v1.json',summary)
    matrix=json.loads((BASE/previous['matrix']).read_text())
    for name,description in (
        ('objects\\weapons\\rifle\\smg\\smg','Original captured paging/dispatch resumes the worker and reaches NtReadFile/device OUT8002400D; current native owned-file reader independently matches47616 resource bytes and an observed task exit under explicit procedure bindings'),
        ('objects\\weapons\\pistol\\plasma_pistol\\plasma_pistol','Original architectural paging matches the earlier705-instruction input-routing replay and all303 ordered source-write bytes'),
        ('objects\\weapons\\pistol\\needler\\needler','Original architectural paging matches the earlier5722-instruction firing model-preparation replay and all4608 ordered source-write bytes')):
        weapon=next(w for w in matrix['weapons']if w['name']==name)
        weapon['bounded_evidence'].append(dict(report='original-weapon-io-progress-v1.json',sha256=sha(BASE/'original-weapon-io-progress-v1.json'),
            component=description,map='assets/disc/maps/lockout.map',certifies_entire_gate=False,
            certifies_all_map_variants=False,complete_current_root_runtime_verified=False,physical_xbox_verified=False))
        if name.endswith('smg\\smg'):
            weapon['gates']['trigger_resources']['remaining']+=' Later uninterrupted original paging/dispatch reaches the real shared-resource read and stops before device OUT8002400D (port1F6,byteA0). Current native file read matches47616 original bytes and an observed complete task update with explicit file/chunk inputs. Real native queue/worker binding, allocation/readiness, device/IRQ completion and the full firing call remain unverified.'
    matrix['previous_matrix_sha256']=sha(BASE/previous['matrix']);matrix['evidence_update_tool_sha256']=sha(Path(__file__))
    assert (matrix['weapon_paths'],matrix['authored_rows'],matrix['maps'])==(63,1110,38)
    assert len(matrix['weapons'])==63 and sum(len(w['gates'])for w in matrix['weapons'])==630
    assert matrix['fully_certified_weapons']==0 and not matrix['full_weapon_parity']and matrix['cleanup_required_for_completion']
    write(BASE/'weapon-completion-matrix-v43.json',matrix)
    folders=sorted(p for p in BASE.iterdir()if p.is_dir()and(p.name.startswith((
        'smg-original-','plasma-original-','xemu-tu15-plasma-reference-','smg-reference-compressed-'))or
        p.name in ('first-person-original-fire-call-v2-paging-v1','original-weapon-cpu-paging-equivalence-v1')))
    checks=[];seen=set();duplicates=[];grouped={}
    for folder in folders:
        for path in folder.rglob('*.json'):
            data=json.loads(path.read_text())
            if isinstance(data,dict)and data.get('runner_scratch')and data['runner_scratch']not in seen:
                value=data['runner_scratch'];assert not Path(value).exists(),value;seen.add(value)
                checks.append(dict(path=value,report=str(path.relative_to(BASE)),removed=True))
        for path in sorted(folder.rglob('*')):
            if not path.is_file()or path.is_symlink():continue
            assert path.stat().st_size<4*1024*1024,path
            with path.open('rb')as source:assert source.read(4)!=b'\x7fELF',path
            if path.suffix not in ('.py','.cpp','.h','.hpp','.bin')and path.name not in ('instructions.json','writes.json'):continue
            key=(sha(path),path.stat().st_size,path.stat().st_mode&0o777)
            if key not in grouped:grouped[key]=path;continue
            canonical=grouped[key]
            if path.stat().st_ino==canonical.stat().st_ino:continue
            temporary=scratch/'same-evidence';os.link(canonical,temporary);os.replace(temporary,path)
            assert sha(path)==key[0]
            duplicates.append(dict(path=str(path.relative_to(BASE)),canonical=str(canonical.relative_to(BASE)),bytes=key[1],sha256=key[0]))
    prior_audit=json.loads((BASE/previous['storage_audit']).read_text());assert not Path(prior_audit['publisher_scratch']).exists()
    audit=dict(passed=True,timestamp_utc=now,mandatory_cleanup_contract='goal-completion-contract-20261018.json',
        scratch_checks=checks,prior_publisher_scratch_removed=True,new_large_reference_inputs=False,
        persistent_new_compiler_objects_executables_or_raw_RAM=0,new_CMake_build_trees=0,
        exact_evidence_duplicates_coalesced=duplicates,all_evidence_paths_and_bytes_preserved=True,
        reference_retention_unchanged='original-weapon-reference-retention-v1.json',
        original_assets_frozen_sources_launchers_and_unrelated_active_work_preserved=True,
        publisher_scratch=str(scratch),publisher_scratch_cleanup_pending_runner_exit=True,full_weapon_parity=False)
    write(BASE/'post-original-io-storage-audit-v1.json',audit)
    checkpoint=dict(passed=True,matrix='weapon-completion-matrix-v43.json',matrix_sha256=sha(BASE/'weapon-completion-matrix-v43.json'),
        evidence='original-weapon-io-progress-v1.json',evidence_sha256=sha(BASE/'original-weapon-io-progress-v1.json'),
        previous_checkpoint=str(previous_checkpoint.relative_to(BASE)),previous_checkpoint_sha256=sha(previous_checkpoint),
        storage_audit='post-original-io-storage-audit-v1.json',cleanup_required_for_completion=True,
        full_weapon_parity=False,fully_certified_weapons=0)
    write(BASE/'checkpoint-original-io-20261018.json',checkpoint)
    path=BASE/'storage-cleanup-current-20261017.json';current=json.loads(path.read_text())
    current.update(timestamp_utc=now,latest_weapon_completion_matrix=checkpoint['matrix'],latest_storage_audit=checkpoint['storage_audit'],
        latest_storage_audit_sha256=sha(BASE/checkpoint['storage_audit']),latest_current_required_input_check='checkpoint-original-io-20261018.json',
        current_allocated_bytes=int(subprocess.check_output(['du','-s','-B1',str(BASE)],text=True).split()[0]))
    current['folder_allocated_size_reduction_bytes']=current['original_baseline_allocated_bytes']-current['current_allocated_bytes']
    current['folder_allocated_size_reduction_percent']=round(100*current['folder_allocated_size_reduction_bytes']/current['original_baseline_allocated_bytes'],3)
    write(path,current)
    print(json.dumps(dict(passed=True,matrix=checkpoint['matrix'],native_read_bytes=47616,scratch_paths_removed=len(checks),
        new_large_reference_inputs=False,coalesced_duplicate_evidence_copies=len(duplicates),
        current_goal_allocated_bytes=current['current_allocated_bytes'],publisher_scratch=str(scratch))),flush=True)


if __name__=='__main__':main()
