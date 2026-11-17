#!/usr/bin/env python3
"""Compact only an unfinished cohort's authenticated component JSON copies.

The CMake runner receipt remains byte-identical. Its retained paths become
logical paths readable by native_metadata; a separate manifest records the
encoded storage. All planning, originals and validation live in owned scratch.
"""
import gzip
import hashlib
import json
import os
from pathlib import Path
import tempfile

import native_metadata as metadata

SCHEMA='native-component-evidence-storage-v1'
MANIFEST=Path('cmake/component-evidence-storage.json')

def _sha(data):return hashlib.sha256(data).hexdigest()
def _json(data):return (json.dumps(data,indent=2,sort_keys=True)+'\n').encode()

def _read(path):
    if path.is_symlink() or not path.is_file():raise ValueError('Expected an owned regular file: '+str(path))
    data=path.read_bytes()
    if len(data)>metadata.FILE_LIMIT:raise ValueError('Component metadata exceeds1MiB: '+str(path))
    return data

def _paths(evidence):
    paths=[]
    if evidence.is_symlink() or not evidence.is_dir():raise ValueError('Expected owned component evidence directory')
    for directory,dirs,names in os.walk(evidence,followlinks=False):
        parent=Path(directory)
        if any((parent/name).is_symlink()for name in dirs+names):raise ValueError('Component evidence contains a symlink')
        for name in names:
            path=parent/name
            if name.endswith(('.json','.json.gz')):paths.append(path)
            elif '.json.gz.' in name:raise ValueError('Unexpected component storage part')
    return sorted(paths)

def _receipt(output,manifest_path,manifest_bytes,manifest,idempotent):
    return dict(schema=SCHEMA,manifest_path=str(manifest_path),manifest_sha256=_sha(manifest_bytes),
        manifest_bytes=len(manifest_bytes),runner_report_sha256=manifest['runner_report_sha256'],
        json_reports=len(manifest['files']),compressed_reports=sum(row['encoding']=='gzip'for row in manifest['files']),
        raw_component_bytes=manifest['raw_component_bytes'],stored_component_bytes=manifest['stored_component_bytes'],
        saved_bytes=manifest['raw_component_bytes']-manifest['stored_component_bytes'],
        retained_metadata_bytes=metadata.check_budget(output),idempotent=idempotent)

def _validate_existing(output,evidence,manifest_path,runner_bytes,rows):
    data=_read(manifest_path);manifest=json.loads(data)
    if manifest.get('schema')!=SCHEMA or manifest.get('runner_report_sha256')!=_sha(runner_bytes):raise ValueError('Component storage manifest/runner receipt differs')
    seen=set();raw_total=stored_total=0
    for entry in manifest['files']:
        index=entry['runner_index']
        if not isinstance(index,int) or not 0<=index<len(rows):raise ValueError('Component manifest runner index differs')
        row=rows[index];relative=Path(row['path'])
        logical=Path('cmake/evidence')/relative
        if relative.is_absolute() or '..'in relative.parts or relative.suffix!='.json' or row.get('status')!='retained':raise ValueError('Unsafe component receipt path')
        encoding=entry['encoding'];stored=Path(str(logical)+'.gz')if encoding=='gzip'else logical
        if encoding not in('gzip','identity') or entry['logical_path']!=logical.as_posix() or entry['stored_path']!=stored.as_posix():raise ValueError('Component manifest storage path differs')
        if entry['runner_record_sha256']!=_sha(_json(row)):raise ValueError('Component manifest original receipt differs')
        actual=output/stored;payload=_read(actual)
        if actual in seen or len(payload)!=entry['stored_bytes'] or _sha(payload)!=entry['stored_sha256']:raise ValueError('Component encoded size/hash differs')
        seen.add(actual)
        decoded=metadata.read_bytes(output/logical,max_bytes=metadata.FILE_LIMIT)
        if len(decoded)!=row['bytes'] or _sha(decoded)!=row['sha256'] or entry['raw_bytes']!=len(decoded) or entry['raw_sha256']!=_sha(decoded):raise ValueError('Component decoded size/hash differs')
        raw_total+=len(decoded);stored_total+=len(payload)
    expected={index for index,row in enumerate(rows)if row.get('status')=='retained'and Path(row['path']).suffix=='.json'}
    if {entry['runner_index']for entry in manifest['files']}!=expected or set(_paths(evidence))!=seen:raise ValueError('Component storage inventory differs')
    if (raw_total,stored_total)!=(manifest['raw_component_bytes'],manifest['stored_component_bytes']):raise ValueError('Component storage totals differ')
    return _receipt(output,manifest_path,data,manifest,True)

def _publish_file(source,target,created):
    # Exclusive creation handles scratch/output on different filesystems while
    # preventing an unrelated destination from being overwritten.
    payload=source.read_bytes()
    with target.open('xb')as outgoing:
        stat=os.fstat(outgoing.fileno())
        record=dict(path=target,identity=(stat.st_dev,stat.st_ino),sha256=_sha(payload));created.append(record)
        outgoing.write(payload);outgoing.flush();stat=os.fstat(outgoing.fileno())
        record['stamp']=(stat.st_size,stat.st_mtime_ns,stat.st_ctime_ns)

def _still_owned(record):
    path=record['path']
    try:
        if path.is_symlink() or not path.is_file():return False
        stat=path.stat()
        return (stat.st_dev,stat.st_ino)==record['identity'] and (stat.st_size,stat.st_mtime_ns,stat.st_ctime_ns)==record.get('stamp') and _sha(path.read_bytes())==record['sha256']
    except OSError:return False

def _rollback(output,removed,originals,created):
    conflicts=[]
    for original in removed:
        try:
            with original.open('xb')as outgoing:outgoing.write(originals[original])
        except FileExistsError:
            if original.is_symlink() or not original.is_file() or original.read_bytes()!=originals[original]:conflicts.append(original)
    changed=[record['path']for record in created if record['path'].exists() and not _still_owned(record)]
    recovery=None
    if conflicts or changed:
        recovery=Path(tempfile.mkdtemp(prefix='component-evidence-recovery-',dir=output/'cmake'))
        recovered=[]
        for number,original in enumerate(conflicts):
            raw=originals[original];encoded=gzip.compress(raw,compresslevel=9,mtime=0)
            name=str(number)+('.json.gz'if len(encoded)<len(raw)else'.json');payload=encoded if name.endswith('.gz')else raw
            if len(payload)>metadata.FILE_LIMIT:raise ValueError('Conflict recovery file exceeds1MiB: '+str(recovery))
            target=recovery/name
            with target.open('xb')as outgoing:outgoing.write(payload)
            if metadata.read_bytes(target,max_bytes=metadata.FILE_LIMIT)!=raw:raise ValueError('Conflict recovery did not round-trip')
            recovered.append(dict(logical_path=str(original.relative_to(output)),stored_path=str(target.relative_to(output)),raw_bytes=len(raw),raw_sha256=_sha(raw),stored_bytes=len(payload),stored_sha256=_sha(payload)))
        diagnostics=dict(schema='native-component-evidence-recovery-v1',originals=recovered,
            externally_replaced_targets=[str(path.relative_to(output))for path in changed],
            external_files_preserved=True,individual_file_limit_bytes=metadata.FILE_LIMIT,
            original_raw_receipts_still_authenticated=True)
        recovery_bytes=_json(diagnostics)
        if len(recovery_bytes)>metadata.FILE_LIMIT:raise ValueError('Conflict recovery manifest exceeds1MiB: '+str(recovery))
        (recovery/'recovery.json').write_bytes(recovery_bytes)
    for record in reversed(created):
        if _still_owned(record):record['path'].unlink()
    if recovery is not None:
        # Failure may restore the deliberately allowed initially-over-budget
        # raw state. Record it explicitly; it can never become a passing
        # compaction or silently expand the acceptance budget.
        other_bytes=metadata.size(output)-(recovery/'recovery.json').stat().st_size
        diagnostics['total_limit_bytes']=metadata.TOTAL_LIMIT;diagnostics['reserve_bytes']=metadata.RESERVE
        for _ in range(4):
            total=other_bytes+len(_json(diagnostics));diagnostics['failed_output_metadata_bytes']=total
            diagnostics['failed_output_within_budget']=total+metadata.RESERVE<=metadata.TOTAL_LIMIT
        recovery_bytes=_json(diagnostics)
        if len(recovery_bytes)>metadata.FILE_LIMIT:raise ValueError('Conflict recovery manifest exceeds1MiB: '+str(recovery))
        (recovery/'recovery.json').write_bytes(recovery_bytes)
    return recovery

def compact_component_evidence(output):
    """Return a bounded receipt; never edit completed historical cohorts.

    Call once after components finish, before the cohort-wide budget check.
    Source receipt hashes remain hashes of the exact decoded original bytes.
    """
    scratch_value=os.environ.get('HALO2_TEST_SCRATCH')
    if not scratch_value:raise RuntimeError('Component compaction requires HALO2_TEST_SCRATCH')
    scratch=Path(scratch_value).resolve()
    if not scratch.is_dir():raise ValueError('Owned scratch directory is absent')
    output=Path(output).absolute()
    if output.is_symlink() or (output/'cmake').is_symlink():raise ValueError('Component output must not be a symlink')
    output=output.resolve();cohort_path=output/'report.json';runner_path=output/'cmake/report.json';evidence=output/'cmake/evidence';manifest_path=output/MANIFEST
    cohort_bytes=_read(cohort_path);cohort=json.loads(cohort_bytes)
    if not isinstance(cohort,dict) or cohort.get('passed') is not False or cohort.get('status')!='components':raise ValueError('Historical/terminal cohort or non-component phase cannot be compacted')
    if not isinstance(cohort.get('owned_scratch'),str) or Path(cohort['owned_scratch']).resolve()!=scratch:raise ValueError('Cohort does not belong to the current scratch owner')
    runner_bytes=_read(runner_path);runner=json.loads(runner_bytes)
    if not isinstance(runner,dict) or not isinstance(runner.get('retained_small_reports'),list):raise ValueError('CMake runner receipt is absent')
    rows=runner['retained_small_reports']
    if manifest_path.exists():return _validate_existing(output,evidence,manifest_path,runner_bytes,rows)
    original_paths=_paths(evidence)
    if any(path.suffix!='.json'for path in original_paths):raise ValueError('Encoded component evidence needs its original storage manifest')
    originals={};entries=[];plans=[];seen=set()
    with tempfile.TemporaryDirectory(prefix='component-evidence-',dir=scratch)as stage_value:
        stage=Path(stage_value)
        for index,row in enumerate(rows):
            if row.get('status')!='retained':continue
            relative=Path(row['path'])
            if relative.is_absolute() or '..'in relative.parts:raise ValueError('Unsafe component receipt path')
            if relative.suffix!='.json':continue
            logical=Path('cmake/evidence')/relative;path=output/logical
            if path in seen:raise ValueError('Duplicate retained component receipt')
            seen.add(path);raw=_read(path)
            if len(raw)!=row['bytes'] or _sha(raw)!=row['sha256']:raise ValueError('Original component receipt size/hash differs')
            originals[path]=raw;encoded=gzip.compress(raw,compresslevel=9,mtime=0)
            encoding='gzip'if len(encoded)<len(raw)else'identity'
            payload=encoded if encoding=='gzip'else raw
            stored=Path(str(logical)+'.gz')if encoding=='gzip'else logical
            staged=stage/str(index);staged.write_bytes(payload)
            # Verify the exact staged bytes through the same bounded decoder
            # used by accounting readers before any output file is changed.
            check=stage/(str(index)+('.json.gz'if encoding=='gzip'else'.json'))
            check.write_bytes(staged.read_bytes())
            if metadata.read_bytes(check,max_bytes=metadata.FILE_LIMIT)!=raw:raise ValueError('Staged component bytes did not round-trip')
            entries.append(dict(runner_index=index,runner_record_sha256=_sha(_json(row)),logical_path=logical.as_posix(),stored_path=stored.as_posix(),encoding=encoding,
                raw_bytes=len(raw),raw_sha256=_sha(raw),stored_bytes=len(payload),stored_sha256=_sha(payload)))
            if encoding=='gzip':plans.append((path,output/stored,staged))
        if seen!=set(original_paths):raise ValueError('Unreceipted component JSON evidence')
        manifest=dict(schema=SCHEMA,runner_report_path='cmake/report.json',runner_report_sha256=_sha(runner_bytes),
            cohort_report_sha256_before_compaction=_sha(cohort_bytes),files=entries,
            raw_component_bytes=sum(row['raw_bytes']for row in entries),stored_component_bytes=sum(row['stored_bytes']for row in entries),
            limits=dict(file_bytes=metadata.FILE_LIMIT,total_bytes=metadata.TOTAL_LIMIT,reserve_bytes=metadata.RESERVE),
            original_runner_receipt_unchanged=True,decoded_bytes_and_hashes_unchanged=True)
        manifest_bytes=_json(manifest)
        if len(manifest_bytes)>metadata.FILE_LIMIT:raise ValueError('Component storage manifest exceeds1MiB')
        existing=metadata.files(output);replaced={path for path,_,_ in plans}
        projected=sum(path.stat().st_size for path in existing if path not in replaced)+sum(row['stored_bytes']for row in entries if row['encoding']=='gzip')+len(manifest_bytes)
        if any(path.stat().st_size>metadata.FILE_LIMIT for path in existing)or any(row['stored_bytes']>metadata.FILE_LIMIT for row in entries):raise metadata.MetadataBudgetError('Retained metadata file exceeds1MiB',projected)
        if projected+metadata.RESERVE>metadata.TOTAL_LIMIT:raise metadata.MetadataBudgetError('Accounting/component metadata exceeds8MiB retention budget',projected)
        if _read(cohort_path)!=cohort_bytes or _read(runner_path)!=runner_bytes or _paths(evidence)!=original_paths or any(_read(path)!=data for path,data in originals.items()):raise ValueError('Component inputs changed during storage planning')
        staged_manifest=stage/'manifest';staged_manifest.write_bytes(manifest_bytes)
        created=[];removed=[]
        try:
            for original,target,staged in plans:
                if target.exists():raise ValueError('Component encoded destination already exists')
                _publish_file(staged,target,created)
                original.unlink();removed.append(original)
            _publish_file(staged_manifest,manifest_path,created)
            if metadata.check_budget(output)!=projected:raise ValueError('Component committed budget differs')
            result=_validate_existing(output,evidence,manifest_path,runner_bytes,rows)
            result['idempotent']=False
            return result
        except BaseException as error:
            recovery=_rollback(output,removed,originals,created)
            if recovery is not None:raise RuntimeError('Component publication failed; external files preserved; original recovery: '+str(recovery/'recovery.json')+' sha256='+_sha((recovery/'recovery.json').read_bytes()))from error
            raise
