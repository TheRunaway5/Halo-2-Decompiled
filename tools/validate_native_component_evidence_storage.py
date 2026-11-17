#!/usr/bin/env python3
"""Replay completed R21 metadata in owned scratch, without new runtime claims."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys

from native_component_evidence_storage import compact_component_evidence,MANIFEST
from native_metadata import files,check_budget,FILE_LIMIT,TOTAL_LIMIT,RESERVE,read_bytes

ROOT=Path(__file__).resolve().parents[1]
def sha(data):return hashlib.sha256(data).hexdigest()
def hashes(root):return {str(p.relative_to(root)):sha(p.read_bytes())for p in files(root)}
def semantic(data):return {k:v for k,v in data.items()if k not in('report_path','report_sha256')}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--evidence-root',type=Path,default=ROOT)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    original=args.evidence_root.resolve()/'build/test-results/native-execution-engine-r21-20261110'
    before=hashes(original);report_bytes=(original/'report.json').read_bytes();runner_bytes=(original/'cmake/report.json').read_bytes()
    report=json.loads(report_bytes)
    if not report.get('passed') or len(report.get('accounting_runs',[]))!=8:raise ValueError('Completed eight-run R21 evidence is required')
    replay=scratch/'replay-r21';replay.mkdir()
    for path in files(original):
        target=replay/path.relative_to(original);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target)
    shutil.copyfile(original/'cmake/tests.log',replay/'cmake/tests.log')
    original_bytes=check_budget(replay)
    # This is a new unfinished metadata-only fixture. Restore the exact original
    # cohort report afterward to exercise both completed-cohort readers.
    (replay/'report.json').write_text(json.dumps(dict(passed=False,status='components',owned_scratch=str(scratch),scope='metadata-only replay fixture'))+'\n')
    storage=compact_component_evidence(replay)
    if (replay/'cmake/report.json').read_bytes()!=runner_bytes:raise ValueError('Original runner receipt changed')
    for row in json.loads(runner_bytes)['retained_small_reports']:
        if row.get('status')!='retained' or Path(row['path']).suffix!='.json':continue
        data=read_bytes(replay/'cmake/evidence'/row['path'],max_bytes=FILE_LIMIT)
        if len(data)!=row['bytes'] or sha(data)!=row['sha256']:raise ValueError('Original receipt identity changed')
    second=compact_component_evidence(replay)
    if not second['idempotent']:raise ValueError('Storage manifest idempotence failed')
    (replay/'report.json').write_bytes(report_bytes)
    compressed_bytes=check_budget(replay)
    from compare_native_execution_checkpoints import checked_cohort
    if semantic(checked_cohort(original/'report.json'))!=semantic(checked_cohort(replay/'report.json')):raise ValueError('Completed checkpoint reader changed eight-run semantics')
    from aggregate_native_execution import aggregate
    if aggregate(original/'report.json',ROOT,top=0)!=aggregate(replay/'report.json',ROOT,top=0):raise ValueError('Completed aggregate reader changed eight-run semantics')
    if before!=hashes(original):raise ValueError('Historical R21 evidence changed')
    result=dict(passed=True,schema='native-component-evidence-storage-replay-v1',historical_r21_unchanged=True,
        historical_r21_report_sha256=sha(report_bytes),historical_r21_runner_sha256=sha(runner_bytes),
        original_metadata_bytes=original_bytes,compacted_metadata_bytes=compressed_bytes,
        reserve_bytes=RESERVE,total_limit_bytes=TOTAL_LIMIT,headroom_after_reserve=TOTAL_LIMIT-RESERVE-compressed_bytes,
        storage=storage,manifest_sha256=sha((replay/MANIFEST).read_bytes()),
        actual_run_count=8,all_decoded_component_receipt_hashes_unchanged=True,runner_receipt_bytes_unchanged=True,
        completed_checkpoint_reader_identical=True,completed_aggregate_reader_identical=True,
        maximum_retained_file_bytes=max(p.stat().st_size for p in files(replay)),
        owned_scratch=str(scratch),retained_large_artifacts=[],
        scope='Metadata-only replay of original R21, not a new engine/runtime cohort or R22 execution acceptance.')
    data=(json.dumps(result,indent=2,sort_keys=True)+'\n').encode()
    if len(data)>FILE_LIMIT:raise ValueError('Replay report exceeds1MiB')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_bytes(data)
    print(json.dumps({key:result[key]for key in('passed','original_metadata_bytes','compacted_metadata_bytes','headroom_after_reserve')}))
    return 0

if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            path=Path(sys.argv[sys.argv.index('--report')+1]);path.parent.mkdir(parents=True,exist_ok=True)
            path.write_text(json.dumps(dict(passed=False,error=str(error),owned_scratch=os.environ.get('HALO2_TEST_SCRATCH'),retained_large_artifacts=[]),indent=2)+'\n')
        raise
