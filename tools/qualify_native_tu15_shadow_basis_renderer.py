#!/usr/bin/env python3
"""Same frozen engine before/after native32340 numeric projection, owned captures."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re

from perf_4k60 import parity


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--candidate',type=Path,required=True)
    p.add_argument('--maps',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.reference=a.reference.resolve(strict=True);a.candidate=a.candidate.resolve(strict=True)
    pins=[sha(a.reference),sha(a.candidate)];entry=dict(case='1:150:10:110',runs=[])
    pattern=re.compile(r'Native32340 numeric stats: gates=(\d+) admitted=(\d+) basis=(\d+) tail=(\d+)')
    for role in ('reference','reference-repeat','candidate','candidate-repeat'):
        name='shadow-basis-'+role;runtime=a.reference if role.startswith('reference')else a.candidate
        env={key:''for key in ('HALO2_REFERENCE_LITERAL_INSTRUCTIONS','HALO2_REFERENCE_INSTRUCTION_CALLBACKS',
            'HALO2_REFERENCE_SMALL_MEMORY_TRANSFERS','HALO2_REFERENCE_SCALAR_VECTOR_READS','HALO2_REFERENCE_WRITE_DISPATCH',
            'HALO2_REFERENCE_FP_LIGHT_UP','HALO2_REFERENCE_NORMALIZE_VECTOR')}
        env['HALO2_NATIVE_SHADOW_BASIS_STATS']='1'
        run=parity.capture(runtime,name,1,150,10,110,env,scratch=scratch,maps=a.maps,size=(1920,1080),timeout_seconds=240)
        run['role']=role;log=scratch/(name+'.log')
        with log.open('rb')as stream:stream.seek(max(0,log.stat().st_size-4*1024*1024));text=stream.read().decode(errors='replace')
        run['native_numeric_census']=[dict(zip(('gates','admitted','basis','tail'),map(int,m)))for m in pattern.findall(text)]
        entry['runs'].append(run);print(role,run.get('valid'),run.get('sha256'),run['native_numeric_census'],flush=True)
    parity.compare_runs(entry)
    entry['native_numeric_exercised']=all(any(v['basis']>0 and v['tail']>0 and v['admitted']>0 for v in r['native_numeric_census'])for r in entry['runs']if r['role'].startswith('candidate'))
    entry['reference_has_no_native_numeric_kernel']=all(not r['native_numeric_census']for r in entry['runs']if r['role'].startswith('reference'))
    unchanged=pins==[sha(a.reference),sha(a.candidate)]
    passed=entry['passed']and entry['native_numeric_exercised']and entry['reference_has_no_native_numeric_kernel']and unchanged
    report=dict(passed=passed,reference=dict(path=str(a.reference),sha256=pins[0]),candidate=dict(path=str(a.candidate),sha256=pins[1]),
        inputs_unchanged=unchanged,case=entry,helper_sha256=sha(__file__),retained_large_artifacts=[],
        scope='Finite same-cohort native shadow arithmetic versus previous live host approximation, with positive native prefix/tail execution and1920x1080pixel equality. Original code arithmetic is qualified independently; no complete Xbox renderer visual parity claim.',
        whole_renderer_xbox_visual_parity_verified=False)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    return int(not passed)


if __name__=='__main__':raise SystemExit(main())
