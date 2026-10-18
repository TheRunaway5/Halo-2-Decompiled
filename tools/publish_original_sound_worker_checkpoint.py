#!/usr/bin/env python3
"""Publish original SMG worker proof, production sound I/O and storage cleanup."""
import datetime
import json
import os
from pathlib import Path
import shutil
import subprocess
from original_xbox_pose_case import BASE,ROOT,sha


def write(path,value):path.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')


def main():
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);now=datetime.datetime.now(datetime.timezone.utc).isoformat()
    names=['smg-original-worker-native-v1/report.json','smg-original-worker-caller-v1/report.json',
        'smg-original-sound-read-entry-v1/report.json','smg-native-sound-cache-io-v2/report.json',
        'smg-sound-cache-io-cmake-v1/report.json','smg-sound-cache-io-sanitizers-v2/report.json',
        'smg-sound-cache-world-compile-v1/report.json']
    worker,caller,prefix,native,release,sanitizers,world=[json.loads((BASE/name).read_text())for name in names]
    assert all(r['passed']for r in (worker,caller,native,release,sanitizers,world))
    assert not prefix['passed']and prefix['declared_prefix_stop_reached']and prefix['declared_prefix_stop']==0x1a3b70
    assert worker['original_node_identity_verified']and worker['original_node']==0x4ece38 and worker['original_job']==0x156808
    assert worker['actual_chunk_limit']==0x40000 and len(worker['native']['source_stores'])==8
    assert all(row['equal']for row in worker['comparisons'])and all(row['equal']for row in native['comparisons'])
    assert len(native['native']['negative_controls'])==9
    assert [r['site']for r in caller['request_calls']]==[0x21f4bd]and not caller['unmatched_returns']
    assert not caller['direct_request_sites_covered_by_those_handlers']
    for path,pin in native['source_files_sha256'].items():assert sha(path)==pin,path
    for path,pin in worker['source_files_sha256'].items():assert sha(path)==pin,path
    # Only the world TU and public ABI header participate in its syntax check.
    # The later out-of-line failure-ordering guard has its own final checks.
    for name in ('src/physics_live_world_tu15.cpp','include/halo2/source_sound_cache_io_tu15.hpp'):
        path=ROOT/name;assert sha(path)==world['source_files_sha256'][str(path)]
    generated=ROOT/'src/physics_live_support_tu15_generated.cpp';source=generated.read_text()
    assert 'dispatch(s,target,0x21f4bdu,false);'in source and 'static void run_1a3b70('not in source
    production=[ROOT/name for name in ('src/source_sound_cache_io_tu15.cpp','include/halo2/source_sound_cache_io_tu15.hpp',
        'src/physics_live_world_tu15.cpp','cmake/PhysicsLiveWorldTu15.cmake','tests/source_sound_cache_io_tu15_tests.cpp',
        'tools/validate_original_sound_cache_io_tu15.py')]
    policy='Actual bound sound-resource read and retained completed receipt use the existing synchronous native world policy; full original queue/worker/kernel timing remains unverified'
    summary=dict(passed=True,timestamp_utc=now,original_node=0x4ece38,original_job=0x156808,
        actual_chunk_limit=0x40000,native_worker_stopped_and_joined=True,actual_native_synchronization_objects=3,
        native_worker_source_stores=8,exact_original_queue_bank_bytes=9632,exact_original_resource_bytes=47616,
        exact_original_task_current_node_and_done=True,source_call_chain_request_site=0x21f4bd,
        previously_existing_world_handlers_do_not_cover_this_source_caller=True,
        new_production_sound_cache_io_owner=True,world_dispatch_wiring_syntax_checked=True,
        world_generated_call_reaches_required_owner=True,world_generated_source_sha256=sha(generated),
        production_files_sha256={str(p):sha(p)for p in production},production_native_policy=policy,
        release_component_CMake_passed=True,ASAN_UBSAN_component_CMake_passed=True,negative_controls=9,
        complete_application_link_runtime_or_audio_output_verified=False,complete_original_or_native_firing_verified=False,
        source_instruction_patches=[],supplied_original_kernel_gameplay_or_device_results=[],
        native_constructor_priority_TLS_scheduler_and_device_owners_verified=False,
        evidence={name:sha(BASE/name)for name in names},new_large_reference_inputs=False,
        physical_xbox_verified=False,full_weapon_parity=False)
    write(BASE/'original-sound-worker-production-progress-v1.json',summary)
    prior=BASE/'weapon-completion-matrix-v43.json';matrix=json.loads(prior.read_text())
    weapon=next(w for w in matrix['weapons']if w['name']=='objects\\weapons\\rifle\\smg\\smg')
    weapon['bounded_evidence'].append(dict(report='original-sound-worker-production-progress-v1.json',
        sha256=sha(BASE/'original-sound-worker-production-progress-v1.json'),
        component='Native real-thread worker matches complete9632-byte original queue bank, task/current-node/done and47616 resource bytes. Exact sound-cache caller21F4BD now has a checked production native I/O owner; Release/ASAN/UBSAN and world syntax pass',
        map='assets/disc/maps/lockout.map',certifies_entire_gate=False,certifies_all_map_variants=False,
        complete_current_root_runtime_verified=False,physical_xbox_verified=False))
    weapon['gates']['hud_audio_publication']['status']='partial'
    weapon['gates']['hud_audio_publication']['remaining']='Actual SMG firing queues sound-cache data at21F4BD. Current native worker matches its complete queue/task/buffer/done state, and the world now owns that exact sound-cache read under its synchronous native resource policy. Source worker timing/priority/TLS/device/IRQ/PCM, full authored sounds, HUD and local/split/LAN histories remain required.'
    matrix['previous_matrix_sha256']=sha(prior);matrix['evidence_update_tool_sha256']=sha(Path(__file__))
    assert (matrix['weapon_paths'],matrix['authored_rows'],matrix['maps'])==(63,1110,38)
    assert len(matrix['weapons'])==63 and sum(len(w['gates'])for w in matrix['weapons'])==630
    assert matrix['fully_certified_weapons']==0 and not matrix['full_weapon_parity']and matrix['cleanup_required_for_completion']
    write(BASE/'weapon-completion-matrix-v44.json',matrix)
    directories=sorted(p for p in BASE.iterdir()if p.is_dir()and p.name.startswith((
        'smg-original-worker-','smg-original-sound-read-','smg-native-sound-cache-','smg-sound-cache-')))
    checks=[];seen=set();duplicates=[];grouped={}
    for folder in directories:
        for path in folder.rglob('*.json'):
            record=json.loads(path.read_text())
            if not isinstance(record,dict):continue
            value=record.get('runner_scratch')
            if record.get('build'):value=str(Path(record['build']).parent)
            if value and value not in seen:
                assert not Path(value).exists(),value;seen.add(value)
                checks.append(dict(path=value,report=str(path.relative_to(BASE)),removed=True))
        for path in sorted(folder.rglob('*')):
            if not path.is_file()or path.is_symlink():continue
            assert path.stat().st_size<4*1024*1024,path
            with path.open('rb')as stream:assert stream.read(4)!=b'\x7fELF',path
            if path.suffix not in ('.cpp','.hpp','.py','.h','.cmake'):continue
            key=(sha(path),path.stat().st_size,path.stat().st_mode&0o777)
            if key not in grouped:grouped[key]=path;continue
            canonical=grouped[key]
            if path.stat().st_ino==canonical.stat().st_ino:continue
            temporary=scratch/'same-evidence';os.link(canonical,temporary);os.replace(temporary,path)
            assert sha(path)==key[0]
            duplicates.append(dict(path=str(path.relative_to(BASE)),canonical=str(canonical.relative_to(BASE)),bytes=key[1],sha256=key[0]))
    previous_cleanup=json.loads((BASE/'post-original-io-storage-audit-v1.json').read_text())
    assert not Path(previous_cleanup['publisher_scratch']).exists()
    audit=dict(passed=True,timestamp_utc=now,mandatory_cleanup_contract='goal-completion-contract-20261018.json',
        scratch_checks=checks,previous_checkpoint_scratch_removed=True,persistent_new_builds_binaries_objects_or_raw_RAM=0,
        exact_evidence_duplicate_copies_coalesced=duplicates,all_evidence_paths_and_bytes_preserved=True,
        new_large_reference_inputs=False,existing_required_reference_retention='original-weapon-reference-retention-v1.json',
        original_assets_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True,
        publisher_scratch=str(scratch),publisher_scratch_cleanup_pending_runner_exit=True,full_weapon_parity=False)
    write(BASE/'post-original-sound-worker-storage-audit-v1.json',audit)
    checkpoint=dict(passed=True,matrix='weapon-completion-matrix-v44.json',matrix_sha256=sha(BASE/'weapon-completion-matrix-v44.json'),
        evidence='original-sound-worker-production-progress-v1.json',evidence_sha256=sha(BASE/'original-sound-worker-production-progress-v1.json'),
        previous_checkpoint='checkpoint-original-io-20261018.json',previous_checkpoint_sha256=sha(BASE/'checkpoint-original-io-20261018.json'),
        storage_audit='post-original-sound-worker-storage-audit-v1.json',cleanup_required_for_completion=True,
        production_io_owner_changed=True,fully_certified_weapons=0,full_weapon_parity=False)
    write(BASE/'checkpoint-original-sound-worker-20261018.json',checkpoint)
    shutil.copyfile(Path(__file__),BASE/'publish-original-sound-worker-v44-source.py')
    path=BASE/'storage-cleanup-current-20261017.json';current=json.loads(path.read_text())
    current.update(timestamp_utc=now,latest_weapon_completion_matrix=checkpoint['matrix'],latest_storage_audit=checkpoint['storage_audit'],
        latest_storage_audit_sha256=sha(BASE/checkpoint['storage_audit']),latest_current_required_input_check='checkpoint-original-sound-worker-20261018.json',
        current_allocated_bytes=int(subprocess.check_output(['du','-s','-B1',str(BASE)],text=True).split()[0]))
    current['folder_allocated_size_reduction_bytes']=current['original_baseline_allocated_bytes']-current['current_allocated_bytes']
    current['folder_allocated_size_reduction_percent']=round(100*current['folder_allocated_size_reduction_bytes']/current['original_baseline_allocated_bytes'],3)
    write(path,current)
    print(json.dumps(dict(passed=True,matrix=checkpoint['matrix'],new_production_sound_cache_IO=True,
        native_worker_queue_bytes=9632,native_sound_resource_bytes=47616,owned_scratch_paths_removed=len(checks),
        current_goal_allocated_bytes=current['current_allocated_bytes'],new_large_reference_inputs=False,
        publisher_scratch=str(scratch))),flush=True)


if __name__=='__main__':main()
