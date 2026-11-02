#!/usr/bin/env python3
"""Finite same-engine GPU frames for native receiver-volume wiring.

Run under run_test.py. Original TU1.5 disables these shadows in split screen;
only the actual one-player workload can qualify positive native kernel use.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
from perf_4k60 import parity


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--maps',type=Path,required=True);p.add_argument('--map',default='coagulation');p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);runtime=a.runtime.resolve(strict=True);maps=a.maps.resolve(strict=True)
    pins={str(v):sha(v)for v in (runtime,maps/(a.map+'.map'))};entry=dict(case='1:150:10:110',runs=[])
    for role in ('reference','reference-repeat','candidate','candidate-repeat'):
        name='shadow-view-'+role
        env={'HALO2_NATIVE_SHADOW_VIEW_STATS':'1','HALO2_NATIVE_SHADOW_BASIS_STATS':'1',
            'HALO2_REFERENCE_SHADOW_VIEW':'1'if role.startswith('reference')else''}
        run=parity.capture(runtime,name,1,150,10,110,env,scratch=scratch,map_name=a.map,maps=maps,size=(1920,1080),timeout_seconds=240)
        run['role']=role;run['environment']=env
        log=scratch/(name+'.log')
        with log.open('rb')as stream:stream.seek(max(0,log.stat().st_size-4*1024*1024));text=stream.read().decode(errors='replace')
        counts=re.findall(r'Native320A0 stats: calls=(\d+) admitted=(\d+)',text)
        run['native_view_calls']=sum(int(c[0])for c in counts);run['native_view_admitted']=sum(int(c[1])for c in counts)
        entry['runs'].append(run);print(role,run['valid'],run.get('sha256'),run['native_view_calls'],'native receiver volumes',flush=True)
    parity.compare_runs(entry)
    entry['native_exercised']=all(r['native_view_calls']>0 and r['native_view_admitted']>0
        for r in entry['runs']if r['role'].startswith('candidate'))
    entry['reference_preserved']=all(r['native_view_calls']==0 for r in entry['runs']if r['role'].startswith('reference'))
    unchanged=all(sha(v)==pin for v,pin in pins.items());entry['passed']=entry['passed']and entry['native_exercised']and entry['reference_preserved']and unchanged
    report=dict(passed=entry['passed'],case=entry,source_runtime_map_sha256=pins,inputs_unchanged=unchanged,
        helper_sha256=sha(__file__),retained_large_artifacts=[],whole_renderer_xbox_visual_parity_verified=False,
        scope='Four finite one-player frames from the same frozen engine with explicit reference/native receiver-volume selection and positive native invocation/admission census; no Xbox framebuffer or full renderer readiness claim.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
