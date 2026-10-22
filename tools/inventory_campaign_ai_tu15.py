#!/usr/bin/env python3
"""Inventory prepared MCC campaign content through the native cache consumer.

Retain authored squad names/counts/starts/BSP ownership, script definitions and
builtin call counts. This is a content inventory for engine acceptance, never
NPC birth, original HS scheduling, navigation/combat or co-op qualification.
The selected inspection runtime is preserved; generated logs are disposable.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from physics_test_cache import runtime_cache,runtime_map


def sha(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--runtime',type=Path,required=True);ap.add_argument('--mcc-root',type=Path,required=True)
    ap.add_argument('--map',action='append');ap.add_argument('--report-output',type=Path,required=True)
    args=ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:ap.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();runtime=args.runtime.resolve(strict=True)
    maps,inputs=runtime_cache(args.mcc_root);cache=args.mcc_root.resolve(strict=True)
    selected=args.map or [p.name for p in sorted(maps.glob('*.map')) if re.fullmatch(r'\d\d[a-z]_.*\.map',p.name)]
    assert selected and len(set(selected))==len(selected)
    report=dict(scope=__doc__,runtime=str(runtime),runtime_sha256=sha(runtime),mcc_inputs=inputs,
        tool_sha256=sha(__file__),maps=[],passed=False,engine_behavior_qualified=False,
        original_hs_scheduling_qualified=False,cooperative_session_qualified=False,retained_large_artifacts=[])
    args.report_output.parent.mkdir(parents=True,exist_ok=True)
    def save():args.report_output.write_text(json.dumps(report,indent=2)+'\n')
    env={k:v for k,v in os.environ.items() if not k.startswith('HALO2_') or k=='HALO2_TEST_SCRATCH'}
    for key in ('XDG_CONFIG_HOME','XDG_DATA_HOME','XDG_CACHE_HOME'):env[key]=str(scratch/key.lower())
    for ordinal,name in enumerate(selected):
        path=runtime_map(maps,maps/name);trace=scratch/f'{ordinal}-opens.log';log=scratch/f'{ordinal}-content.log'
        command=[str(runtime),'--inspect-systems',str(path),'--bsp','0']
        item=dict(map=name,map_sha256=sha(path),command=command,passed=False)
        report['maps'].append(item);save()
        try:
            with log.open('w') as stream:
                result=subprocess.run(['strace','--kill-on-exit','-f','-e','trace=openat','-o',str(trace),'--',*command],
                    cwd=scratch,env=env,stdout=stream,stderr=subprocess.STDOUT,timeout=180)
            item['returncode']=result.returncode
        except subprocess.TimeoutExpired:item['timeout']=True
        text=log.read_text(errors='replace');opened=trace.read_text(errors='replace')
        item['description']=text.split('Squad:',1)[0][:8000]
        try:item['scenario']=json.loads(text.splitlines()[0])
        except (ValueError,IndexError):item['scenario']={}
        totals=re.search(r'\bsquads=(\d+) scripts=(\d+)\b',text)
        item['declared_counts']=dict(zip(('squads','scripts'),map(int,totals.groups()))) if totals else {}
        item['inspected_bsp']=0
        item['other_bsps_behavior_qualified']=False
        item['squads']=[dict(name=m[0],count=int(m[1]),starts=int(m[2]),
            team=ord(m[3]) if len(m[3])==1 and not m[3].isdigit() else int(m[3]),bsp=int(m[4]))
            for m in re.findall(r'^Squad: (.*?) count=(-?\d+) starts=(\d+) team=(.*?) bsp=(-?\d+)$',text,re.M)]
        item['scripts']=[dict(name=n,type=int(t)) for n,t in re.findall(r'^Script: (.*?) type=(\d+)$',text,re.M)]
        item['builtins']=[dict(name=n,calls=int(c)) for n,c in re.findall(r'^Builtin: (.*?) calls=(\d+)$',text,re.M)]
        item['limitations']=re.findall(r'^Unresolved: (.*)$',text,re.M)
        item['diagnostic_tail']=text[-4000:] if item.get('returncode')!=0 else ''
        paths=re.findall(r'openat\([^,]+, "([^"]+)"',opened)
        item['reference_inputs_opened']=sorted({p for p in paths if p.lower().endswith('.xbe') or
            (p.lower().endswith('.map') and not Path(p).resolve().is_relative_to(cache))})
        item['cache_write_attempts']=[line[:800] for line in opened.splitlines() if str(cache) in line and
            re.search(r'O_WRONLY|O_RDWR|O_CREAT|O_TRUNC',line)]
        item['inventory_counts_match']=(item['declared_counts']==dict(squads=len(item['squads']),scripts=len(item['scripts']))
            and isinstance(item['scenario'].get('bsps'),int) and item['scenario']['bsps']>0)
        item['passed']=(item.get('returncode')==0 and item['inventory_counts_match'] and
            not item['reference_inputs_opened'] and not item['cache_write_attempts'])
        save();print(json.dumps(dict(map=name,passed=item['passed'],squads=len(item['squads']),scripts=len(item['scripts']))),flush=True)
    report['runtime_unchanged']=sha(runtime)==report['runtime_sha256'];assert report['runtime_unchanged']
    report['totals']=dict(maps=len(report['maps']),bsps=sum(m['scenario'].get('bsps',0) for m in report['maps']),
        squads=sum(len(m['squads']) for m in report['maps']),scripts=sum(len(m['scripts']) for m in report['maps']),
        distinct_builtins=len({b['name'] for m in report['maps'] for b in m['builtins']}))
    report['passed']=all(m['passed'] for m in report['maps']);save()
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
