#!/usr/bin/env python3
"""Publish bounded original pose evidence, exact retention and scratch cleanup."""
import datetime
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
from original_xbox_pose_case import BASE,ROOT,physical_ram,sha

def write(path,data):path.write_text(json.dumps(data,indent=2,sort_keys=True)+'\n')
def allocated(path):return int(subprocess.check_output(['du','-s','-B1',str(path)],text=True).split()[0])

def main():
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);now=datetime.datetime.now(datetime.timezone.utc).isoformat()
    reports=['first-person-original-call-v1-native-evidence-v1.json',
        'first-person-original-fire-call-v2-native-evidence-v1.json',
        'first-person-original-update-fire-v1-host-v1/report.json',
        'first-person-original-update-fire-v1-cpu-boundary-v1/report.json']
    for name in reports:
        report=json.loads((BASE/name).read_text());assert report['passed']
        for path,digest in report.get('linked_files_sha256',{}).items():assert sha(path)==digest,path
    folders=sorted(p for p in BASE.glob('first-person-original*')if p.is_dir())
    # Only immutable, completed evidence copies are coalesced. Every pathname
    # and exact historical hash remains available; no report is rewritten.
    grouped={};deduplicated=[]
    for folder in folders:
        for path in sorted(folder.iterdir()):
            if not path.is_file()or path.is_symlink():continue
            if not(path.suffix in ('.py','.cpp','.bin')or path.name in ('instructions.json','writes.json')):continue
            key=(sha(path),path.stat().st_size,path.stat().st_mode&0o777)
            if key not in grouped:grouped[key]=path;continue
            canonical=grouped[key]
            if path.stat().st_ino==canonical.stat().st_ino:continue
            temporary=scratch/f'evidence-link-{len(deduplicated)}';os.link(canonical,temporary);os.replace(temporary,path)
            assert sha(path)==key[0]
            deduplicated.append(dict(path=str(path.relative_to(BASE)),canonical=str(canonical.relative_to(BASE)),sha256=key[0],bytes=key[1]))
    write(BASE/'original-pose-evidence-deduplication-v1.json',dict(passed=True,exact_paths_and_bytes_preserved=True,
        replaced_duplicate_copies=deduplicated,logical_duplicate_bytes=sum(r['bytes']for r in deduplicated),
        exclusive_physical_bytes_freed_claimed=False))
    checks=[];seen=set()
    for folder in folders:
        for name in ('cleanup.json','report.json','build-command.json'):
            path=folder/name
            if not path.exists():continue
            data=json.loads(path.read_text());value=data.get('runner_scratch')
            if value and value not in seen:
                seen.add(value);assert not Path(value).exists(),value
                checks.append(dict(report=str(path.relative_to(BASE)),path=value,removed=True))
        for path in folder.iterdir():
            if path.is_file():
                with path.open('rb')as source:assert source.read(4)!=b'\x7fELF',str(path)
    for name in reports[:2]:
        value=json.loads((BASE/name).read_text())['runner_scratch']
        if value not in seen:
            seen.add(value);assert not Path(value).exists()
            checks.append(dict(report=name,path=value,removed=True))
    # Reconstruct every new original RAM input in anonymous memory; validate the
    # complete hashes after cleanup without retaining another raw bank.
    references=[];chunks={}
    for name in ('first-person-original-call-v1','first-person-original-fire-call-v2','first-person-original-update-fire-v1'):
        folder=BASE/name;index=json.loads((folder/'physical-memory-retention.json').read_text())
        for label,record in index['files'].items():
            with physical_ram(folder,label)as memory:assert hashlib.sha256(memory).hexdigest()==record['raw_sha256']
            recipe=Path(record['content_recipe']);data=json.loads(recipe.read_text())
            for chunk in data['chunks']:
                digest=chunk['sha256'];path=BASE/'retained-archive-content-v1/chunks'/digest[:2]/(digest+'.z')
                chunks[str(path)]=dict(path=str(path.relative_to(BASE)),compressed_bytes=path.stat().st_size,raw_sha256=digest)
            references.append(dict(case=name,label=label,**record))
        launch=json.loads((folder/'launch.json').read_text());cleanup=json.loads((folder/'cleanup.json').read_text())
        assert cleanup['all_processes_exited']and cleanup['original_session_unchanged']
        assert sha(Path(launch['original_session'])/'session-hdd.qcow2')==launch['original_session_sha256']
        for key in ('pid','xvfb_pid'):assert not Path('/proc',str(launch[key])).exists()
    retained=dict(passed=True,reason='Required original live API entry/exit reference inputs; exact reconstruction preserved',
        raw_reference_bytes=sum(r['raw_bytes']for r in references),references=references,shared_chunks=list(chunks.values()),
        referenced_unique_compressed_chunk_bytes=sum(r['compressed_bytes']for r in chunks.values()),
        new_persistent_raw_RAM_dumps=False,physical_xbox_verified=False,full_weapon_parity=False)
    write(BASE/'original-pose-reference-retention-v1.json',retained)
    previous=BASE/'weapon-completion-matrix-v39.json';matrix=json.loads(previous.read_text())
    assert matrix['weapon_paths']==63 and matrix['authored_rows']==1110 and matrix['maps']==38
    weapon=next(w for w in matrix['weapons']if w['name']=='objects\\weapons\\pistol\\needler\\needler')
    descriptions=[
        'Actual original168DED idle call versus native complete model-preparation owner:54 nodes,5722 instructions and4608 store bytes exact; raw reserved x87 bit6 difference retained',
        'Actual original168DED call after normal first Needler shot versus native complete model-preparation owner:54 nodes,5722 instructions and4608 store bytes exact',
        'Actual original168CA1 firing update on the host CPU versus native:27216 instructions,21748 store bytes,148 CPU estimate lanes and33584-byte state bank exact',
        'Original-emulator versus host CPU precision boundary:148 identical RSQRT inputs yield148 different estimates; native state remains different from the original emulator']
    for name,description in zip(reports,descriptions):
        weapon['bounded_evidence'].append(dict(report=name,sha256=sha(BASE/name),component=description,
            map='assets/disc/maps/lockout.map',certifies_entire_gate=False,certifies_all_map_variants=False,
            complete_current_root_runtime_verified=False,physical_xbox_verified=False))
    weapon['gates']['poses_models']['remaining']+=' Two actual original168DED calls now match the native model-preparation owner exactly for54 nodes each, with unchanged original cached animation state as entry input. Actual168CA1 updates match unchanged original code on the host CPU across all27216 instructions,21748 store bytes and the33584-byte state bank. Original emulation and host/native differ at148 reciprocal-sqrt estimate lanes,604 final state bytes and raw x87 status. This precision boundary remains open; no production arithmetic was fitted to the emulator. Full native animation-state origin, all transitions/species/hands/regions, complete current-root integration, physical Xbox arithmetic and final frames/audio remain required.'
    matrix['previous_matrix_sha256']=sha(previous);matrix['evidence_update_tool_sha256']=sha(Path(__file__))
    assert len(matrix['weapons'])==63 and sum(len(w['gates'])for w in matrix['weapons'])==630
    assert matrix['fully_certified_weapons']==0 and not matrix['full_weapon_parity']and matrix['cleanup_required_for_completion']
    write(BASE/'weapon-completion-matrix-v40.json',matrix)
    audit=dict(passed=True,timestamp_utc=now,goal_cleanup_contract='goal-completion-contract-20261018.json',
        scratch_checks=checks,new_persistent_compiler_objects_or_executables=0,new_persistent_CMake_build_trees=0,
        new_original_reference_retention='original-pose-reference-retention-v1.json',
        required_reference_unique_compressed_chunk_bytes=retained['referenced_unique_compressed_chunk_bytes'],
        exact_evidence_deduplication='original-pose-evidence-deduplication-v1.json',
        new_case_files_logical_bytes=sum(p.stat().st_size for f in folders for p in f.iterdir()if p.is_file()),
        unrelated_active_test_work_preserved=True,publisher_scratch=str(scratch),publisher_scratch_cleanup_pending_runner_exit=True,
        full_weapon_parity=False)
    write(BASE/'post-original-pose-storage-audit-v1.json',audit)
    checkpoint=dict(passed=True,timestamp_utc=now,full_weapon_parity=False,fully_certified_weapons=0,
        matrix='weapon-completion-matrix-v40.json',matrix_sha256=sha(BASE/'weapon-completion-matrix-v40.json'),
        evidence={name:sha(BASE/name)for name in reports},
        retention='original-pose-reference-retention-v1.json',retention_sha256=sha(BASE/'original-pose-reference-retention-v1.json'),
        storage_audit='post-original-pose-storage-audit-v1.json',cleanup_required_for_completion=True,
        production_game_code_changed=False,complete_current_root_runtime_verified=False,physical_xbox_verified=False)
    write(BASE/'checkpoint-original-pose-20261018.json',checkpoint)
    current_path=BASE/'storage-cleanup-current-20261017.json';current=json.loads(current_path.read_text())
    current.update(timestamp_utc=now,latest_weapon_completion_matrix='weapon-completion-matrix-v40.json',
        latest_storage_audit='post-original-pose-storage-audit-v1.json',
        latest_current_required_input_check='checkpoint-original-pose-20261018.json',
        current_original_pose_reference_retention='original-pose-reference-retention-v1.json',
        archive_content_store_allocated_bytes=allocated(BASE/'retained-archive-content-v1'),current_allocated_bytes=allocated(BASE))
    current['folder_allocated_size_reduction_bytes']=current['original_baseline_allocated_bytes']-current['current_allocated_bytes']
    current['folder_allocated_size_reduction_percent']=round(100*current['folder_allocated_size_reduction_bytes']/current['original_baseline_allocated_bytes'],3)
    write(current_path,current)
    print(json.dumps(dict(passed=True,matrix=checkpoint['matrix'],fully_certified_weapons=0,
        test_scratch_paths_removed=len(checks),duplicate_copies_coalesced=len(deduplicated),
        retained_reference_unique_compressed_chunk_bytes=retained['referenced_unique_compressed_chunk_bytes'],
        current_goal_allocated_bytes=current['current_allocated_bytes'],publisher_scratch=str(scratch))),flush=True)

if __name__=='__main__':main()
