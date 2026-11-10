#!/usr/bin/env python3
"""Replay authentic retained metadata; never rerun or relabel engine captures."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from native_metadata import retain_file,read_bytes,load,digest,files,size,check_budget,FILE_LIMIT,RESERVE,TOTAL_LIMIT
ROOT=Path(__file__).resolve().parents[1]
def hashes(root):return{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()for p in files(root)}
def copied_metadata(source,destination):
    for path in files(source):
        target=destination/path.relative_to(source);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target)
def compress_selected(root):
    receipts=[]
    for path in(root/'source-hashes.json',*sorted(root.glob('*-execution*.json'))):
        before=path.read_bytes();result=retain_file(path,path,root,max_bytes=64*FILE_LIMIT if path.name=='source-hashes.json'else FILE_LIMIT)
        if read_bytes(path)!=before or digest(path)!=hashlib.sha256(before).hexdigest():raise ValueError('Metadata byte/hash round trip failed')
        receipts.append(result)
    return receipts
def semantic_checked(summary):
    return{k:v for k,v in summary.items()if k not in('report_path','report_sha256')}
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--evidence-root',type=Path,default=ROOT,help='Original owner of historical metadata; frozen source borrows it read-only')
    args=parser.parse_args();evidence_root=args.evidence_root.resolve()
    R17=evidence_root/'build/test-results/native-execution-engine-r17-20261109';R14=evidence_root/'build/test-results/native-execution-engine-r14-20261109'
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);before17=hashes(R17);before14=hashes(R14);r17_report=load(R17/'report.json')
    if r17_report.get('passed'):raise ValueError('Historical R17 must retain its failed metadata-budget status')
    replay=scratch/'r17';copied_metadata(R17,replay);raw_size=size(replay);receipts=compress_selected(replay)
    if digest(replay/'source-hashes.json')!=r17_report['source_inventory_sha256']:raise ValueError('Original R17 source inventory identity changed')
    r17_runtime=list(replay.glob('*-execution.json.gz'))
    if len(r17_runtime)!=6:raise ValueError('Expected exactly six authentic retained R17 runtime reports')
    # The removed R17 owner cannot supply its missing final counters. Exercise
    # the eight-run retention envelope using actual earlier counters, labelled
    # as metadata-only input; never mix them into a claimed R17 runtime cohort.
    fill=[]
    for label in('zanzibar-4p-static','zanzibar-4p-moving'):
        for suffix in('-execution.json','-execution-summary.json'):
            original=R14/(label+suffix);result=retain_file(original,replay/original.name,replay,max_bytes=FILE_LIMIT);fill.append(dict(source=str(original),sha256=digest(original),retained=result))
    eight_size=check_budget(replay)
    # A fully completed historical eight-run cohort independently checks both
    # production readers and their original raw identity checks transparently.
    completed=scratch/'completed-r14';copied_metadata(R14,completed);completed_receipts=compress_selected(completed)
    shutil.copyfile(R14/'cmake/tests.log',completed/'cmake/tests.log')
    report=load(completed/'report.json')
    for run in report['accounting_runs']:
        # Existing logical paths deliberately remain unchanged here: reader
        # fallback must also handle a losslessly compressed old plain cohort.
        raw=completed/Path(run['runtime_report']).name;summary=completed/Path(run['analysis']).name
        if digest(raw)!=run['runtime_report_sha256']or digest(summary)!=run['analysis_sha256']:raise ValueError('Raw run/summary identity changed')
    from compare_native_execution_checkpoints import checked_cohort
    original_comparison=checked_cohort(R14/'report.json');compressed_comparison=checked_cohort(completed/'report.json')
    if semantic_checked(original_comparison)!=semantic_checked(compressed_comparison):raise ValueError('Transparent checkpoint reader changed semantic results')
    from aggregate_native_execution import aggregate
    original_queue=aggregate(R14/'report.json',ROOT,top=20);compressed_queue=aggregate(completed/'report.json',ROOT,top=20)
    if original_queue!=compressed_queue:raise ValueError('Transparent aggregate reader changed results')
    # New cohorts record physical compressed paths and storage receipts. Keep
    # validating those routes as well as the legacy logical-path fallback.
    by_logical={Path(row['logical_path']).name:row for row in completed_receipts}
    report['source_inventory_storage']=by_logical['source-hashes.json']
    for run in report['accounting_runs']:
        for field in('runtime_report','analysis'):
            stored=by_logical[Path(run[field]).name];run[field]=stored['path'];run[field+'_storage']=stored
    (completed/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    if semantic_checked(original_comparison)!=semantic_checked(checked_cohort(completed/'report.json')):raise ValueError('New compressed-path checkpoint reader changed semantics')
    new_queue=aggregate(completed/'report.json',ROOT,top=20)
    for key in original_queue:
        if key not in('cohort_report_sha256','workloads')and original_queue[key]!=new_queue[key]:raise ValueError('New compressed-path aggregate reader changed '+key)
    for original,new in zip(original_queue['workloads'],new_queue['workloads'],strict=True):
        if {k:v for k,v in original.items()if k!='runtime_report'}!={k:v for k,v in new.items()if k!='runtime_report'}:raise ValueError('New compressed-path workload counters changed')
    # The direct runtime CLI must produce byte-identical semantic analysis and
    # the decoded raw hash for a compressed actual runtime report.
    runtime=r17_runtime[0];analysis=scratch/'analysis.json';command=[sys.executable,'-B',str(ROOT/'tools/native_execution_report.py'),'--runtime',str(runtime),'--output',str(analysis)];tested=subprocess.run(command,capture_output=True,text=True)
    if tested.returncode:raise ValueError('Compressed runtime CLI failed: '+tested.stderr[-3000:])
    from native_execution_report import analyze
    actual=load(analysis);expected=analyze(load(runtime,max_bytes=FILE_LIMIT))
    if any(actual[k]!=v for k,v in expected.items())or actual['runtime_report_sha256']!=digest(runtime):raise ValueError('Compressed runtime analysis changed')
    if before17!=hashes(R17)or before14!=hashes(R14):raise ValueError('Historical metadata was modified')
    result=dict(passed=True,owned_scratch=str(scratch),historical_r17_unchanged=True,historical_r14_unchanged=True,r17_still_failed=True,actual_r17_retained_runs=6,r17_original_metadata_bytes=raw_size,r17_compressed_metadata_bytes=eight_size-sum(r['retained']['stored_bytes']for r in fill),eight_run_metadata_fixture_bytes=eight_size,aggregate_limit=TOTAL_LIMIT,reserved_headroom=RESERVE,remaining_headroom=TOTAL_LIMIT-RESERVE-eight_size,maximum_new_storage_file=max(p.stat().st_size for p in files(replay)),source_inventory_raw_sha256=r17_report['source_inventory_sha256'],r17_storage_receipts=receipts,missing_r17_rows_not_fabricated=True,eight_run_fixture_fill_from_completed_r14=fill,completed_eight_run_readers_identical=True,completed_r14_receipts=completed_receipts,scope='Metadata-only replay and decoder/reader regression; no engine or captures executed, and no R17 completion inferred',retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k]for k in('passed','eight_run_metadata_fixture_bytes','remaining_headroom','maximum_new_storage_file')}));return 0
if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            path=Path(sys.argv[sys.argv.index('--report')+1]);path.parent.mkdir(parents=True,exist_ok=True);path.write_text(json.dumps(dict(passed=False,owned_scratch=os.environ.get('HALO2_TEST_SCRATCH'),error=str(error),retained_large_artifacts=[]),indent=2)+'\n')
        raise
