#!/usr/bin/env python3
"""Finite same-engine GPU frames for ordered native44710 and shared-mask entry wiring.

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
    for role in ('reference','reference-repeat','candidate','candidate-repeat','entry-oracle'):
        name='cull-walk-'+role
        env={'HALO2_NATIVE_CULL_WALK_STATS':'1',
            'HALO2_REFERENCE_MAIN_CULL':'1'if role.startswith('reference')else'',
            'HALO2_MAIN_CULL_ORACLE':'1'if role=='entry-oracle'else''}
        run=parity.capture(runtime,name,1,150,10,110,env,scratch=scratch,map_name=a.map,maps=maps,size=(1920,1080),timeout_seconds=240)
        run['role']=role;run['environment']=env
        log=scratch/(name+'.log')
        with log.open('rb')as stream:stream.seek(max(0,log.stat().st_size-4*1024*1024));text=stream.read().decode(errors='replace')
        counts=re.findall(r'Native44710 stats: calls=(\d+) frustum_calls=(\d+) sphere_calls=(\d+) native_entries=(\d+) residual_entries=(\d+)',text)
        run['native_calls']=sum(int(c[0])for c in counts);run['frustum_calls']=sum(int(c[1])for c in counts)
        run['native_entries']=sum(int(c[3])for c in counts);run['residual_entries']=sum(int(c[4])for c in counts)
        mismatches=re.findall(r'unit_mismatches=(\d+) mask_mismatches=(\d+) fp_mismatches=(\d+)',text)
        run['entry_mismatches']=sum(sum(map(int,c))for c in mismatches)
        entry['runs'].append(run);print(role,run['valid'],run.get('sha256'),run['native_calls'],'native walks',run['native_entries'],'native BSP entries',flush=True)
    parity.compare_runs(entry)
    entry['native_exercised']=all(r['native_calls']>0 and r['native_entries']>0
        for r in entry['runs']if r['role'].startswith('candidate'))
    entry['mixed_services_exercised']=all(r['residual_entries']>0 for r in entry['runs']if r['role'].startswith('candidate'))
    entry['reference_preserved']=all(r['native_calls']==0 for r in entry['runs']if r['role'].startswith('reference'))
    entry['entry_oracle_passed']=all(r['valid']and r['native_entries']>0 and r['entry_mismatches']==0 and r['sha256']==entry['runs'][0]['sha256']for r in entry['runs']if r['role']=='entry-oracle')
    unchanged=all(sha(v)==pin for v,pin in pins.items());entry['passed']=entry['passed']and entry['native_exercised']and entry['reference_preserved']and entry['entry_oracle_passed']and unchanged
    report=dict(passed=entry['passed'],case=entry,source_runtime_map_sha256=pins,inputs_unchanged=unchanged,
        helper_sha256=sha(__file__),retained_large_artifacts=[],whole_renderer_xbox_visual_parity_verified=False,
        scope='Five finite one-player frames from the same frozen engine, complete original/native44710 selection, positive native service counts, an explicit residual-coverage census and synchronous actual44E80 unit/mask/FP comparisons. No Xbox framebuffer or full renderer readiness claim.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
