#!/usr/bin/env python3
"""Compare both actual pending-response parents with fresh original TU1.5 cases.

The existing unchanged-code vitality oracle owns all inputs and outcomes. Its
72 declared pending/region cases use original Lockout tags and explicit object,
clock and recorded metadata overlays. Stop at the original first unowned callee;
no eligibility/effect result is supplied. This is not natural gameplay or audio.
"""
import argparse,hashlib,json,os,subprocess,tempfile
from pathlib import Path
from inspect_multiplayer import Cache
from oracle_smg_firing_1_1 import ROOT,CACHE_PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import measure_source_vitality_runtime as original


def sha(path):
    with path.open('rb') as file:return hashlib.file_digest(file,'sha256').hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--vitality',type=Path,required=True);p.add_argument('--delivery',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    normalized=original.BASE/'generated/normalized-correspondence.json'
    delivery_map=ROOT/'analysis/damage-shields-goal-20261011/delivery-runtime/generated/normalized-correspondence.json'
    cache_path=ROOT/'assets/disc/maps/lockout.map'
    if not all(path.exists() for path in (normalized,delivery_map,cache_path,NEW_DEFAULT)):return 77
    inputs={str(path):sha(path) for path in (normalized,delivery_map,cache_path,NEW_DEFAULT,Path(original.__file__),a.vitality,a.delivery)}
    assert sha(NEW_DEFAULT)==NEW_PIN and sha(cache_path)==CACHE_PIN
    paired=json.loads(normalized.read_text());assert paired['authority_sha256']=='e0f135ff479f39a091589fa7393d3a72654b8d91ffe8aa334a4a874c6a17c3d1'
    declared=[row for row in original.rows() if row.get('regions')]
    assert len(declared)==72
    cache=Cache(cache_path)
    try:cases=original.execute(NEW_DEFAULT,True,paired,cache,declared)
    finally:cache.data.close();cache.stream.close()
    reference=dict(schema=1,image='1.5',cases=cases,instruction_patches=[],native_expected_data_used=False)
    steps=sum(c['instructions'].count(0xd74a0) for c in cases)
    assert steps>0 and any(row['declared_input'].get('pending')==8 for row in cases)
    inverse={value:int(key,16) for key,value in json.loads(delivery_map.read_text())['entries'].items()}
    result=dict(passed=False,scope=__doc__,original_xbe_sha256=NEW_PIN,original_case_count=len(cases),original_pending_step_entries=steps,
        original_natural_returns=sum(c['complete'] for c in cases),original_callee_boundaries=sum(not c['complete'] for c in cases),
        original_instructions=sum(len(c['instructions']) for c in cases),ordered_original_writes=sum(len(c['writes']) for c in cases),
        recorded_metadata_overlays=sum(len(c['metadata_overlays']) for c in cases),
        metadata_overlays=[dict(case=c['id'],overlays=c['metadata_overlays']) for c in cases],declared_inputs=declared,inputs=inputs,
        reference_tool_sha256=sha(Path(original.__file__)),tool_sha256=sha(Path(__file__)),native_runs=[],
        supplied_callee_results=[],original_code_patches=[],live_MCC_gameplay_or_audible_delivery_verified=False,retained_large_artifacts=[])
    with tempfile.TemporaryDirectory(prefix='pending-parents-',dir=scratch) as owned:
        work=Path(owned);fixture=work/'u5.json'
        for name,binary in [('vitality',a.vitality),('delivery',a.delivery)]:
            if name=='delivery':
                # The delivery library now owns some callees beyond the
                # original oracle's boundary. Stop observation at that same
                # entry with its real pending frame, before executing it.
                for case in cases:
                    if not case['complete'] and case['exit_entry'] in inverse:
                        case['original_prefix_exit_site']=inverse[case['exit_entry']]
            data=json.dumps(reference,sort_keys=True,separators=(',',':'),allow_nan=False)+'\n';fixture.write_text(data)
            argv=[str(binary.resolve()),str(fixture)];process=subprocess.run(argv,capture_output=True,text=True,timeout=120)
            result['native_runs'].append(dict(parent=name,argv=argv,exit_code=process.returncode,fixture_sha256=sha(fixture),
                fixture_bytes=fixture.stat().st_size,original_entry_prefix_stops=sum('original_prefix_exit_site' in c for c in cases),
                output=(process.stdout+process.stderr)[-8192:]))
    result.update(fixture_scratch_removed=not work.exists(),inputs_unchanged=all(sha(Path(path))==pin for path,pin in inputs.items()))
    result['passed']=result['inputs_unchanged'] and all(row['exit_code']==0 for row in result['native_runs'])
    text=json.dumps(result,indent=2)+'\n';assert len(text)<1024*1024
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(text)
    print(json.dumps(dict(passed=result['passed'],cases=len(cases),pending_step_entries=steps,native_runs=result['native_runs'])),flush=True)
    return 0 if result['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
