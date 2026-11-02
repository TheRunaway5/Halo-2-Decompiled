#!/usr/bin/env python3
"""Same frozen engine: native30BD0 versus its retained renderer reference.

Requires run_test.py; raw images, scripts, driver caches and full logs remain
in HALO2_TEST_SCRATCH. A bounded receipt requires positive renderer admissions.
"""
import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import re
import statistics

from perf_4k60 import parity

ROOT=Path(__file__).resolve().parents[1]
PATTERN=re.compile(r'Native30BD0 stats: admitted=(\d+) renderer=(\d+) last=0x([0-9a-fA-F]+) refused=\[([0-9,]+)\](?: attempts=(\d+))?')
SITE_PATTERN=re.compile(r'Native30BD0 renderer site: pc=0x([0-9a-fA-F]+) admitted=(\d+) vector=0x([0-9a-fA-F]+) stack=0x([0-9a-fA-F]+) reason=(\d+) refused=\[([0-9,]+)\] first=(\d+) last=(\d+)')
POLICY_PATTERN=re.compile(r'Native30BD0 policy refusals: \[([0-9,]+)\] owner=(\d+)')
REQUIRED_PATTERN=re.compile(r'Native30BD0 required site: pc=0x([0-9a-fA-F]+) entered=(\d+)')
POLICY_REASONS=('none','range','reference_dispatch','recursion','policy_hold','policy_change','observer','guard','unavailable')


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def census(log):
    found=[]
    # Runtime diagnostics are scratch-owned. Bound a malformed log before
    # reading; normal capture logs are much smaller than this ceiling.
    with log.open('rb')as stream:
        stream.seek(max(0,log.stat().st_size-4*1024*1024));text=stream.read().decode(errors='replace')
    for line in text.splitlines():
        if match:=PATTERN.search(line):
            admitted,renderer,last,refused,attempts=match.groups();values=list(map(int,refused.split(',')))
            if len(values)!=9:raise ValueError('Unexpected native normalizer refusal schema')
            found.append(dict(admitted=int(admitted),renderer_admitted=int(renderer),last_admitted_site=int(last,16),refused=values,
                attempts=int(attempts)if attempts is not None else None,renderer_sites=[],required_sites=[],policy_refusals=None,owner_refusals=0))
        elif found and (match:=POLICY_PATTERN.search(line)):
            values=list(map(int,match[1].split(',')))
            if len(values)!=len(POLICY_REASONS):raise ValueError('Unexpected native write-policy refusal schema')
            found[-1].update(policy_refusals=dict(zip(POLICY_REASONS,values)),owner_refusals=int(match[2]))
        elif found and (match:=SITE_PATTERN.search(line)):
            pc,admitted,vector,stack,reason,refused,first,last=match.groups();values=list(map(int,refused.split(',')))
            if len(values)!=9:raise ValueError('Unexpected per-site refusal schema')
            found[-1]['renderer_sites'].append(dict(pc=int(pc,16),admitted=int(admitted),last_vector=int(vector,16),
                last_stack=int(stack,16),last_refusal=int(reason),refused=values,first_ordinal=int(first),last_ordinal=int(last)))
        elif found and (match:=REQUIRED_PATTERN.search(line)):
            found[-1]['required_sites'].append(dict(pc=int(match[1],16),entered=int(match[2])))
    consistent=bool(found)and all(w['attempts']==w['admitted']+sum(w['refused'])and
        w['renderer_admitted']==sum(s['admitted']for s in w['renderer_sites'])and w['policy_refusals']is not None and
        sum(w['policy_refusals'].values())+w['owner_refusals']==w['refused'][8]for w in found)
    return dict(worlds=found,admitted=sum(w['admitted']for w in found),
        renderer_admitted=sum(w['renderer_admitted']for w in found),
        refused=[sum(w['refused'][n]for w in found)for n in range(9)],census_consistent=consistent)


def timings(path,size):
    # These finite, fixed-cadence runs can expose an obvious regression. They
    # do not isolate the normalizer or establish quiet-host throughput.
    if not path.exists()or path.stat().st_size>8*1024*1024:
        return dict(valid=False,error='Missing or oversized diagnostic timing table')
    try:
        with path.open()as stream:rows=list(csv.DictReader(stream,delimiter='\t'))
        expected=tuple(size)*2
        if not rows or any(tuple(int(r[k])for k in ('render_width','render_height','present_width','present_height'))!=expected for r in rows):
            raise ValueError('Diagnostic dimensions differ from the requested capture')
        measured=rows[30:-1]
        if not measured:raise ValueError('No timing samples after warmup and before screenshot')
        fields=['interval_ms','renderer_ms','world_native_prepare_ms','host_pre_render_ms']
        fields += [f'host_view{v}_{p}_ms'for v in range(4)for p in ('camera_lighting','objects')]
        summary={}
        for field in fields:
            if field not in measured[0]:continue
            values=sorted(float(r[field])for r in measured)
            if any(not math.isfinite(v)or v<0 for v in values):raise ValueError('Invalid diagnostic timing')
            summary[field]=dict(mean=statistics.mean(values),median=statistics.median(values),p95=values[int(.95*(len(values)-1))])
        return dict(valid=True,total_rows=len(rows),measured_rows=len(measured),milliseconds=summary,
            scope='Fixed-cadence capture diagnostics on a concurrently used host; no isolated kernel speed or FPS claim.')
    except (ValueError,KeyError,TypeError)as error:return dict(valid=False,error=str(error))


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--maps',type=Path,required=True);p.add_argument('--map',default='coagulation')
    p.add_argument('--report',type=Path,required=True);p.add_argument('--size',type=int,nargs=2,default=[1920,1080])
    p.add_argument('--case',action='append');a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.runtime=a.runtime.resolve(strict=True);a.maps=a.maps.resolve(strict=True)
    a.report=a.report.resolve();a.report.parent.mkdir(parents=True,exist_ok=True)
    runtime_pin=sha(a.runtime);map_path=a.maps/(a.map+'.map');map_pin=sha(map_path)
    # TU1.5 disables dynamic object shadows for2/3/4local players. Those
    # frame comparisons cannot establish positive shadow-renderer coverage.
    cases=a.case or ['1:150:10:110']
    result=dict(passed=False,runtime=str(a.runtime),runtime_sha256=runtime_pin,map=str(map_path),map_sha256=map_pin,
        size=a.size,cases=[],whole_renderer_xbox_visual_parity_verified=False,
        scope='Finite same-engine frame equality with positive native admissions from audited renderer call sites; physical scalar-code authority is qualified separately.',
        helper_sha256=sha(__file__),parity_helper_sha256=sha(parity.__file__),retained_large_artifacts=[])
    for n,case in enumerate(cases):
        values=tuple(map(int,case.split(':')))
        if len(values)!=4 or not 1<=values[0]<=4 or values[1]<=0 or values[2]<0 or values[3]<values[2]:raise ValueError('Invalid finite renderer case')
        entry=dict(case=case,runs=[])
        for role in ('reference','reference-repeat','candidate','candidate-repeat'):
            name=f'normalize-{n}-{role}'
            env={k:''for k in ('HALO2_REFERENCE_LITERAL_INSTRUCTIONS','HALO2_REFERENCE_INSTRUCTION_CALLBACKS',
                'HALO2_REFERENCE_SMALL_MEMORY_TRANSFERS','HALO2_REFERENCE_SCALAR_VECTOR_READS',
                'HALO2_REFERENCE_WRITE_DISPATCH','HALO2_REFERENCE_FP_LIGHT_UP')}
            timing_path=scratch/(name+'-timings.tsv')
            env.update(HALO2_NATIVE_NORMALIZE_STATS='1',HALO2_REFERENCE_NORMALIZE_VECTOR='1'if role.startswith('reference')else'',
                HALO2_RENDER_TIMINGS=str(timing_path))
            run=parity.capture(a.runtime,name,*values,env,scratch=scratch,map_name=a.map,maps=a.maps,size=tuple(a.size),timeout_seconds=240)
            run['role']=role;run['environment']=env;run['native_census']=census(scratch/(name+'.log'));entry['runs'].append(run)
            run['diagnostic_timings']=timings(timing_path,a.size)
            print(case,role,run.get('valid'),run.get('sha256'),run['native_census']['renderer_admitted'],'renderer admissions',flush=True)
        parity.compare_runs(entry)
        entry['reference_path_exercised']=all(r['native_census']['worlds'] and r['native_census']['admitted']==0 and
            r['native_census']['refused'][0]>0 for r in entry['runs']if r['role'].startswith('reference'))
        entry['native_renderer_exercised']=all(r['native_census']['renderer_admitted']>0 for r in entry['runs']if r['role'].startswith('candidate'))
        entry['native_constant_owners_admitted']=all(r['native_census']['refused'][7]==0 for r in entry['runs']if r['role'].startswith('candidate'))
        entry['census_consistent']=all(r['native_census']['census_consistent']for r in entry['runs'])
        entry['passed']=entry['passed']and entry['reference_path_exercised']and entry['native_renderer_exercised']and entry['native_constant_owners_admitted']and entry['census_consistent']
        result['cases'].append(entry);a.report.write_text(json.dumps(result,indent=2)+'\n')
    result['inputs_unchanged']=sha(a.runtime)==runtime_pin and sha(map_path)==map_pin
    result['passed']=result['inputs_unchanged']and all(c['passed']for c in result['cases'])
    result['scratch_owner']=str(scratch);a.report.write_text(json.dumps(result,indent=2)+'\n')
    print('PASS'if result['passed']else'FAIL','native normalizer renderer qualification')
    return int(not result['passed'])


if __name__=='__main__':raise SystemExit(main())
