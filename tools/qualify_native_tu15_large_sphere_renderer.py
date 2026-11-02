#!/usr/bin/env python3
"""Finite live native/original44710 large-sphere render qualification.

Use run_test.py. Captures, previews and complete command histories remain in
HALO2_TEST_SCRATCH; only hashes, coverage and bounded diagnostics survive.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
from perf_4k60 import parity


def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',type=Path,required=True);p.add_argument('--maps',type=Path,required=True)
    p.add_argument('--map',default='coagulation');p.add_argument('--case',default='1:170:10:110')
    p.add_argument('--starting-weapon',type=int,choices=(0,*range(3,21)),default=8)
    p.add_argument('--look',type=float,nargs=2,default=(1.35,-.6))
    p.add_argument('--probe-only',action='store_true');p.add_argument('--report',type=Path,required=True)
    a=p.parse_args();players,tick,first,last=map(int,a.case.split(':'))
    if not(1<=players<=4 and tick>0 and 0<=first<=last and -1.5<=a.look[1]<=1.5):p.error('Invalid finite input case')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    runtime=a.runtime.resolve(strict=True);maps=a.maps.resolve(strict=True)
    pins={str(path):sha(path)for path in(runtime,maps/(a.map+'.map'),Path(parity.__file__))}
    script='# tick player fire reload grenade yaw pitch\n'+''.join(
        f'{t} {n} {int(first<=t<last and last>0)} 0 0 {a.look[0]} {a.look[1]}\n'
        for t in range(tick)for n in range(players))
    entry=dict(case=a.case,runs=[]);banks=[]
    roles=('reference',)if a.probe_only else('reference','reference-repeat','candidate','candidate-repeat')
    for role in roles:
        name='large-sphere-'+role;bank=scratch/(name+'.banks');reference=role.startswith('reference')
        env={'HALO2_REFERENCE_LIGHT_FRUSTUM_CULL':'reference'if reference else'',
            'HALO2_NATIVE_LARGE_SPHERE_STATS':'1','HALO2_COMMAND_BANKS_PATH':str(bank),
            'HALO2_REFERENCE_LIGHT_FLAGS_31650':'','HALO2_REFERENCE_BANK_DRAW_UNITS':'',
            'HALO2_REFERENCE_MODEL_DRAW_UNITS':'','HALO2_REFERENCE_LIGHT_PARENT_BIPED':''}
        run=parity.capture(runtime,name,players,tick,first,last,env,scratch=scratch,map_name=a.map,
            maps=maps,size=(1920,1080),timeout_seconds=300,
            extra_arguments=['--mp-starting-weapon',str(a.starting_weapon)]if a.starting_weapon else[],script_text=script)
        run.update(role=role,environment=env)
        rgb=run.get('_payload',b'').split(b'\n',3)[-1]
        run['nonuniform_image']=bool(rgb)and any(rgb[i:i+3]!=rgb[:3]for i in range(3,len(rgb),3))
        if run.get('_payload')and role in('reference','candidate'):
            preview=scratch/(name+'-preview.ppm');preview.write_bytes(run['_payload']);run['scratch_preview']=str(preview)
        log=scratch/(name+'.log')
        with log.open('rb')as stream:
            stream.seek(max(0,log.stat().st_size-4*1024*1024));text=stream.read().decode(errors='replace')
        rows=re.findall(r'Sphere mode0 stats: receivers=(\d+) entries=(\d+) model_entries=(\d+) '
            r'exhausted=(\d+) reference=(\d+) mismatches=(\d+)',text)
        for n,key in enumerate(('receivers','entries','model_entries','exhausted','reference_calls','mismatches')):
            run[key]=sum(int(row[n])for row in rows)
        run['diagnostic_errors']=[line[:1200]for line in text.splitlines()if
            'Large-sphere' in line or 'Source large-sphere' in line][-12:]
        data=bank.read_bytes()if bank.is_file()else b'';banks.append(data)
        run.update(bank_bytes=len(data),bank_sha256=hashlib.sha256(data).hexdigest());entry['runs'].append(run)
        print(role,run['valid'],run.get('sha256'),run['receivers'],'mode0 receivers',run['entries'],
            'entries',run['reference_calls'],'original comparisons',run['mismatches'],'mismatches',flush=True)
    positive=all(r['receivers']>0 and r['entries']>0 for r in entry['runs'])
    reference_selected=all(r['reference_calls']==r['receivers']and r['reference_calls']>0
        for r in entry['runs']if r['role'].startswith('reference'))
    clean=all(r['mismatches']==0 and not r['diagnostic_errors']for r in entry['runs'])
    if a.probe_only:
        for run in entry['runs']:run.pop('_payload',None)
        passed=False;probe_completed=entry['runs'][0]['valid']and positive and reference_selected and clean
    else:
        parity.compare_runs(entry);probe_completed=None
        entry.update(positive_mode0_work=positive,reference_selected=reference_selected,diagnostics_clean=clean,
            native_selected=all(r['reference_calls']==0 for r in entry['runs']if r['role'].startswith('candidate')),
            all_images_nonuniform=all(r['nonuniform_image']for r in entry['runs']),
            complete_banks_and_fp_equal=bool(banks[0])and all(b==banks[0]for b in banks))
        entry['bank_first_differences']=[]
        for run,data in zip(entry['runs'],banks,strict=True):
            if data==banks[0]:continue
            at=next((i for i,(x,y)in enumerate(zip(banks[0],data))if x!=y),min(len(banks[0]),len(data)))
            entry['bank_first_differences'].append(dict(role=run['role'],offset=at,
                reference=banks[0][max(0,at-16):at+32].hex(),actual=data[max(0,at-16):at+32].hex()))
        passed=entry['passed']and all(entry[k]for k in('positive_mode0_work','reference_selected',
            'diagnostics_clean','native_selected','all_images_nonuniform','complete_banks_and_fp_equal'))
    unchanged=all(sha(path)==digest for path,digest in pins.items());passed=passed and unchanged
    report=dict(passed=passed,validation_not_run=a.probe_only,probe_completed=probe_completed,case=entry,
        inputs_unchanged=unchanged,source_runtime_map_sha256=pins,helper_sha256=sha(__file__),
        starting_weapon=a.starting_weapon,look=a.look,input_script=script,retained_large_artifacts=[],
        whole_renderer_xbox_visual_parity_verified=False,
        scope='Finite same-engine large-sphere native/original cull comparison. Original44710 uses actual resident '
            'geometry and poses with the same pre-cull draw/visibility inputs, checks complete units/masks/FP and '
            'restores its writes without replacing World leases. Four-run acceptance requires positive mode0 '
            'work, nonuniform repeatable images and identical complete command histories plus main FP. '
            'Model cull work is counted separately. No physical Xbox framebuffer or whole-renderer parity claim.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    return int(not(probe_completed if a.probe_only else passed))


if __name__=='__main__':raise SystemExit(main())
