#!/usr/bin/env python3
"""Prove the World fixture rejects loss of the model caller's nested arena.

Mutates only the enclosing run_test.py owner's private source, restores it in
finally, then rebuilds/retests the restored source before returning.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--jobs',type=int,default=6);a=p.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=a.source.resolve(strict=True)
    if not source.is_relative_to(owner):raise ValueError('Model mutation must stay in the current private owner')
    a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=True)
    path=source/'src/physics/physics_live_world_tu15.cpp';before=path.read_bytes()
    marker=b'++stats.bank_model;\n            CallerFrame nested(*this,sp);'
    if before.count(marker)!=1:raise ValueError('Model frame mutation does not select exactly one owner')
    base=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--jobs',str(a.jobs),'--test-jobs','1','--cmake-arg=-DHALO2_STAGE_BUILD_OUTPUT=OFF','--cmake-arg=-DHALO2_BUILD_WINDOWS=OFF']
    negative=[*base,'--output',str(a.output/'missing-frame'),'--target','native_tu15_model_draw_bridge_tests',
        '--filter','^original_tu15_model_draw_bridge$']
    result=dict(passed=False,before_sha256=hashlib.sha256(before).hexdigest(),owner=str(owner),
        mutation='Remove only the136CE0 owner CallerFrame; original code and declared input cases stay unchanged.',
        negative_command=negative,retained_large_artifacts=[])
    try:
        changed=before.replace(marker,b'++stats.bank_model;\n            // Deliberate missing-frame mutation.',1)
        path.write_bytes(changed);os.utime(path,None)
        result['mutated_sha256']=hashlib.sha256(changed).hexdigest()
        result['negative_exit_code']=subprocess.run(negative).returncode
    finally:
        path.write_bytes(before);os.utime(path,None)
        result['restored_sha256']=hashlib.sha256(path.read_bytes()).hexdigest()
    receipt=json.loads((a.output/'missing-frame/report.json').read_text());steps={s['name']:s['exit_code']for s in receipt['steps']}
    log=(a.output/'missing-frame/tests.log').read_text(errors='replace')
    result['negative_steps']=steps
    result['caller_stack_failure_observed']='native-original-resource: model resource resident None bank differs at 0x600f'in log
    result['mutation_killed']=result['negative_exit_code']!=0 and steps.get('build')==0 and steps.get('tests',0)!=0 and result['caller_stack_failure_observed']
    positive=[*base,'--output',str(a.output/'restored'),'--target','source_native_draw_units_tests',
        '--target','pstencil_native_incident_tests','--target','native_tu15_model_draw_bridge_tests','--target','halo2_native',
        '--filter','^(native_bank_draw_units_world_tu15|pstencil_native_cinematic_incident|original_tu15_model_draw_bridge)$',
        '--retain-report','native-tu15-model-draw-bridge.json','--retain-report','Testing/Temporary/LastTest.log']
    result['positive_command']=positive;result['positive_exit_code']=subprocess.run(positive).returncode
    result['passed']=result['mutation_killed']and result['positive_exit_code']==0 and path.read_bytes()==before
    (a.output/'validation.json').write_text(json.dumps(result,indent=2)+'\n');print('Model nested-frame qualification:',result['passed'])
    return int(not result['passed'])


if __name__=='__main__':raise SystemExit(main())
