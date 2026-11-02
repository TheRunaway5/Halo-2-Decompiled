#!/usr/bin/env python3
"""Finite same-engine original/native resource-policy/publication command banks, FP and GPU comparison.

Use run_test.py. Full-capacity command banks and frames stay scratch-owned;
only small hashes, sizes, positive native counters and diagnostics are kept.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
from perf_4k60 import parity


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--maps',type=Path,required=True);p.add_argument('--map',default='coagulation');p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);runtime=a.runtime.resolve(strict=True);maps=a.maps.resolve(strict=True)
    pins={str(v):sha(v)for v in (runtime,maps/(a.map+'.map'))};entry=dict(case='1:150:10:110',runs=[])
    payloads=[]
    for role in ('reference','reference-repeat','candidate','candidate-repeat'):
        name='bsp-admission-'+role;bank=scratch/(name+'.banks')
        env={'HALO2_NATIVE_COMMAND_WALK_STATS':'1','HALO2_NATIVE_RENDER_PREPARE_STATS':'1','HALO2_NATIVE_GEOMETRY_RESOURCE_STATS':'1','HALO2_NATIVE_RENDER_ADMISSION_STATS':'1','HALO2_REFERENCE_RENDER_LIGHTING':'1'if role.startswith('reference')else'','HALO2_REFERENCE_GEOMETRY_RESOURCE':'1'if role.startswith('reference')else'','HALO2_REFERENCE_MAIN_COMMANDS':'1'if role.startswith('reference')else'',
            'HALO2_COMMAND_BANKS_PATH':str(bank)}
        run=parity.capture(runtime,name,1,150,10,110,env,scratch=scratch,map_name=a.map,maps=maps,size=(1920,1080),timeout_seconds=240)
        run['role']=role;run['environment']=env
        log=scratch/(name+'.log')
        with log.open('rb')as stream:stream.seek(max(0,log.stat().st_size-4*1024*1024));text=stream.read().decode(errors='replace')
        counts=re.findall(r'Native40E80 stats: frames=(\d+) geometry_calls=(\d+) part_calls=(\d+) emit_calls=(\d+)',text)
        for n,key in enumerate(('native_frames','geometry_calls','part_calls','emit_calls')):run[key]=sum(int(c[n])for c in counts)
        prepare=re.findall(r'Native render prepare stats: queues=(\d+) objects=(\d+) geometry=(\d+) resource_calls=(\d+)',text)
        for n,key in enumerate(('native_prepare_queues','native_prepare_objects','native_geometry','native_geometry_requests')):run[key]=sum(int(c[n])for c in prepare)
        resources=re.findall(r'Native130980 stats: calls=(\d+) acquire=(\d+) promote=(\d+) yield=(\d+) pump=(\d+)',text)
        for n,key in enumerate(('native_resource_calls','resource_acquire_calls','resource_promote_calls','resource_yield_calls','resource_pump_calls')):run[key]=sum(int(c[n])for c in resources)
        admission=re.findall(r'Native render admission stats: sections=(\d+) lighting=(\d+) geometry=(\d+) requests=(\d+)',text)
        for n,key in enumerate(('section_services','lighting_services','geometry_services','admission_geometry_requests')):run[key]=sum(int(c[n])for c in admission)
        lighting=re.findall(r'Native2CFE0 stats: native=(\d+) reference=(\d+)',text)
        for n,key in enumerate(('native_lighting_calls','reference_lighting_calls')):run[key]=sum(int(c[n])for c in lighting)
        payload=bank.read_bytes()if bank.is_file()else b'';payloads.append(payload)
        run['bank_bytes']=len(payload);run['bank_sha256']=hashlib.sha256(payload).hexdigest()
        entry['runs'].append(run);print(role,run['valid'],run.get('sha256'),run['native_frames'],'native frames',len(payload),'command-bank bytes',flush=True)
    parity.compare_runs(entry)
    entry['native_exercised']=all(r['native_frames']>0 and r['geometry_calls']>0 and r['part_calls']>0 and r['emit_calls']>0
        for r in entry['runs']if r['role'].startswith('candidate'))
    entry['reference_preserved']=all(r['native_lighting_calls']==0 and r['reference_lighting_calls']>0 and r['native_resource_calls']==0 and r['native_frames']==0 and r['native_prepare_queues']==0 and r['native_geometry']==0 for r in entry['runs']if r['role'].startswith('reference'))
    entry['preparation_exercised']=all(r['native_prepare_queues']>0 and r['native_geometry']>0 for r in entry['runs']if r['role'].startswith('candidate'))
    entry['hierarchy_objects_exercised']=all(r['native_prepare_objects']>0 for r in entry['runs']if r['role'].startswith('candidate'))
    entry['resource_policy_exercised']=all(r['native_resource_calls']>0 for r in entry['runs']if r['role'].startswith('candidate'))
    entry['live_admission_exercised']=all(r['native_lighting_calls']>0 and r['reference_lighting_calls']==0 and r['section_services']>0 and r['geometry_services']>0 and r['admission_geometry_requests']>0 for r in entry['runs']if r['role'].startswith('candidate'))
    entry['real_acquisition_exercised']=all(r['resource_acquire_calls']>0 for r in entry['runs']if r['role'].startswith('candidate'))
    entry['blocking_services_exercised']=any(r['resource_yield_calls']>0 or r['resource_pump_calls']>0 or r['resource_promote_calls']>0 for r in entry['runs']if r['role'].startswith('candidate'))
    entry['complete_banks_and_fp_equal']=bool(payloads[0])and all(v==payloads[0]for v in payloads)
    entry['bank_first_differences']=[]
    for role,data in zip(('reference','reference-repeat','candidate','candidate-repeat'),payloads,strict=True):
        if data==payloads[0]:continue
        first=next((i for i,(left,right)in enumerate(zip(payloads[0],data))if left!=right),min(len(payloads[0]),len(data)))
        entry['bank_first_differences'].append(dict(role=role,offset=first,
            reference=payloads[0][max(0,first-16):first+32].hex(),actual=data[max(0,first-16):first+32].hex()))
    if len(payloads[0])>=96:
        record_bytes=180+24*sum(struct.unpack_from('<I',payloads[0],24+24*b)[0]for b in range(3))
        entry['first_record_bytes']=record_bytes
        entry['reference_records_if_fixed_capacity']=len(payloads[0])//record_bytes
    unchanged=all(sha(v)==pin for v,pin in pins.items())
    entry['passed']=entry['passed']and entry['native_exercised']and entry['preparation_exercised']and entry['resource_policy_exercised']and entry['live_admission_exercised']and entry['reference_preserved']and entry['complete_banks_and_fp_equal']and unchanged
    report=dict(passed=entry['passed'],case=entry,source_runtime_map_sha256=pins,inputs_unchanged=unchanged,
        helper_sha256=sha(__file__),retained_large_artifacts=[],whole_renderer_xbox_visual_parity_verified=False,
        scope='Four finite one-player GPU captures from one frozen engine. Reference selects genuine2CFE0,130980 and40E80; candidate selects native2CFE0/resource policy and40E80. Both use live44930/449D0 at native append sites. Positive service/native-core and acquisition census; complete-capacity command histories plus main x87 control/status/MXCSR. Whole2CBD0 guest FP, Xbox framebuffer and full renderer readiness remain separate.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
