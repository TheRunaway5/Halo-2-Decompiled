#!/usr/bin/env python3
"""Finite same-engine original/native bank-native secondary BSP admission, command banks, FP and GPU comparison.

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
    p.add_argument('--maps',type=Path,required=True);p.add_argument('--map',default='coagulation');p.add_argument('--report',type=Path,required=True)
    p.add_argument('--require-sphere-parent',action='store_true',help='Require live bit22 light queries and parent-biped appends in every capture')
    p.add_argument('--require-model',action='store_true',help='Require accepted native/genuine model appends in every capture')
    p.add_argument('--case',default='1:150:10:110',help='players:capture_tick:fire_first:fire_last');a=p.parse_args()
    players,tick,first,last=map(int,a.case.split(':'))
    if not(1<=players<=4 and tick>0 and 0<=first<=last):p.error('Invalid finite capture case')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);runtime=a.runtime.resolve(strict=True);maps=a.maps.resolve(strict=True)
    pins={str(v):sha(v)for v in (runtime,maps/(a.map+'.map'))};entry=dict(case=a.case,runs=[])
    payloads=[]
    for role in ('reference','reference-repeat','candidate','candidate-repeat'):
        name='bsp-admission-'+role;bank=scratch/(name+'.banks')
        env={'HALO2_NATIVE_DRAW_UNIT_STATS':'1','HALO2_REFERENCE_BANK_DRAW_UNITS':'1'if role.startswith('reference')else'',
            'HALO2_COMMAND_BANKS_PATH':str(bank),'HALO2_NATIVE_LIGHT_PARENT_STATS':'1','HALO2_NATIVE_SPHERE_RECTANGLE_STATS':'1',
            'HALO2_REFERENCE_MODEL_DRAW_UNITS':'1'if role.startswith('reference')else'',
            'HALO2_REFERENCE_LIGHT_PARENT_BIPED':'1'if role.startswith('reference')else''}
        run=parity.capture(runtime,name,players,tick,first,last,env,scratch=scratch,map_name=a.map,maps=maps,size=(1920,1080),timeout_seconds=240)
        run['role']=role;run['environment']=env
        rgb=run.get('_payload',b'').split(b'\n',3)[-1]
        run['nonuniform_image']=bool(rgb)and any(rgb[i:i+3]!=rgb[:3]for i in range(3,len(rgb),3))
        run['rgb_range']=[min(rgb),max(rgb)]if rgb else None
        if a.require_model and role=='candidate'and run.get('_payload'):
            preview=scratch/'model-draw-candidate-preview.ppm';preview.write_bytes(run['_payload'])
            run['scratch_preview']=str(preview)
        log=scratch/(name+'.log')
        with log.open('rb')as stream:stream.seek(max(0,log.stat().st_size-4*1024*1024));text=stream.read().decode(errors='replace')
        counts=re.findall(r'Native bank BSP stats: native=(\d+) reference=(\d+) native_units=(\d+) reference_units=(\d+)',text)
        for n,key in enumerate(('native_bank_batches','reference_bank_batches','native_bank_units','reference_bank_units')):run[key]=sum(int(c[n])for c in counts)
        model=re.findall(r'Native bank model stats: native=(\d+) reference=(\d+) native_units=(\d+) reference_units=(\d+) early=(\d+) geometry=(\d+) sections=(\d+) overrides=(\d+) rectangles=(\d+) stack_records=(\d+) requests=(\d+)',text)
        for n,key in enumerate(('native_model_calls','reference_model_calls','native_model_units','reference_model_units',
            'model_early_refusals','model_geometry_refusals','model_sections','model_overrides','native_model_rectangles','model_stack_records','native_model_requests')):
            run[key]=sum(int(c[n])for c in model)
        parent=re.findall(r'Sphere parent-biped stats: queries=(\d+) appended=(\d+)',text)
        run['sphere_parent_queries']=sum(int(c[0])for c in parent);run['sphere_parent_appends']=sum(int(c[1])for c in parent)
        rectangle=re.findall(r'Native30270 stats: calls=(\d+) x87_triples=(\d+)',text)
        run['native_rectangle_calls']=sum(int(c[0])for c in rectangle);run['native_rectangle_x87_triples']=sum(int(c[1])for c in rectangle)
        run['projected_parent_counters']=[list(map(int,c))for c in re.findall(r'Native31C10 stats: tested=(\d+) taken=(\d+) none=(\d+) walk=(\d+) appended=(\d+) refused=(\d+)',text)]
        payload=bank.read_bytes()if bank.is_file()else b'';payloads.append(payload)
        run['bank_bytes']=len(payload);run['bank_sha256']=hashlib.sha256(payload).hexdigest()
        entry['runs'].append(run);print(role,run['valid'],run.get('sha256'),run['native_bank_batches'],'native bank batches',len(payload),'command-bank bytes',flush=True)
    parity.compare_runs(entry)
    entry['all_images_nonuniform']=all(r['nonuniform_image']for r in entry['runs'])
    entry['native_exercised']=all(r['native_bank_batches']>0 and r['native_bank_units']>0 and r['reference_bank_batches']==0 for r in entry['runs']if r['role'].startswith('candidate'))
    entry['reference_preserved']=all(r['reference_bank_batches']>0 and r['reference_bank_units']>0 and r['native_bank_batches']==0 for r in entry['runs']if r['role'].startswith('reference'))
    entry['sphere_parent_exercised']=all(r['sphere_parent_queries']>0 and r['sphere_parent_appends']>0 for r in entry['runs'])
    entry['native_model_exercised']=all(r['native_model_calls']>0 and r['native_model_units']>0 and not r['reference_model_calls']for r in entry['runs']if r['role'].startswith('candidate'))
    entry['genuine_model_reference_exercised']=all(r['reference_model_calls']>0 and r['reference_model_units']>0 and not r['native_model_calls']for r in entry['runs']if r['role'].startswith('reference'))
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
    entry['passed']=entry['passed']and entry['all_images_nonuniform']and entry['native_exercised']and entry['reference_preserved']and entry['complete_banks_and_fp_equal']and unchanged and(not a.require_sphere_parent or entry['sphere_parent_exercised'])and(not a.require_model or entry['native_model_exercised']and entry['genuine_model_reference_exercised'])
    report=dict(passed=entry['passed'],case=entry,source_runtime_map_sha256=pins,inputs_unchanged=unchanged,
        helper_sha256=sha(__file__),retained_large_artifacts=[],whole_renderer_xbox_visual_parity_verified=False,
        sphere_parent_required=a.require_sphere_parent,
        model_required=a.require_model,
        scope='Four finite same-engine GPU captures. Reference selects genuine2CB10/135EA0/136030, model2CBD0 and projected parent166B60; candidate selects bank-native BSP/model and projected parent kernels. Sphere parent policy is active in both roles, so image equality is integration evidence, not an independent original-code proof of that policy. Dynamic136230 remains genuine above its2CBD0 site;30270 is native in both roles and its live count is reported without requiring it. Requires positive bank batch counters, complete-capacity command histories plus main FP and identical images. Optional parent and model gates require actual accepted work in every capture. Physical Xbox framebuffer, full renderer and all-map parity remain separate.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
