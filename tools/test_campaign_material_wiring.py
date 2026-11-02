#!/usr/bin/env python3
"""Bounded native campaign startup regression.

Use tools/run_test.py --scratch-root .local/test-scratch so native snapshots
have disk storage. All logs and native cache/config outputs stay in scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time

from run_tests import validation_environment


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine',type=Path,required=True)
    parser.add_argument('--cache',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--map',action='append')
    parser.add_argument('--frames',type=int,default=2)
    parser.add_argument('--timeout',type=float,default=100)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    if args.frames<1 or args.timeout<=0:parser.error('Frames and timeout must be positive')
    root=Path(__file__).resolve().parents[1]
    scratch=Path(tempfile.mkdtemp(prefix='campaign-startup-',dir=os.environ['HALO2_TEST_SCRATCH']))
    engine=args.engine.resolve(strict=True);cache=args.cache.resolve(strict=True)
    maps=cache/'halo2/h2_maps_win64_dx11'
    names=args.map or ['01b_spacestation','03a_oldmombasa','03b_newmombasa','04b_floodlab',
        '05a_deltaapproach','05b_deltatowers','06b_floodzone','07a_highcharity','07b_forerunnership']
    report={'engine':str(engine),'engine_sha256':hashlib.sha256(engine.read_bytes()).hexdigest(),
        'cache':str(cache),'scratch':str(scratch),'scope':'Bounded native campaign startup, not full playthrough or visual parity',
        'cases':[],'passed':False}
    args.report.parent.mkdir(parents=True,exist_ok=True)
    for ordinal,name in enumerate(names):
        if Path(name).name!=name:parser.error('Map must be a basename without extension')
        owned=scratch/str(ordinal);owned.mkdir()
        env={key:value for key,value in os.environ.items() if not key.startswith('HALO2_')}
        env.update(HALO2_TEST_SCRATCH=str(owned),XDG_CONFIG_HOME=str(owned/'config'),
            XDG_CACHE_HOME=str(owned/'cache'),SDL_AUDIODRIVER='dummy')
        env,validation=validation_environment(root,env)
        command=[str(engine),'--campaign','--map',str(maps/(name+'.map')),'--shared-maps',str(maps),
            '--frames',str(args.frames),'--hidden','--mute','--validation','--viewport','640','360','--graphics-mode','Original']
        log=owned/'native.log';start=time.monotonic();error=None
        try:
            with log.open('wb')as stream:
                result=subprocess.run(command,cwd=root,env=env,stdout=stream,stderr=subprocess.STDOUT,timeout=args.timeout)
            code=result.returncode
        except subprocess.TimeoutExpired:
            code=None;error='timeout'
        with log.open('rb')as stream:
            stream.seek(0,2);size=stream.tell();stream.seek(max(0,size-12000));tail=stream.read().decode(errors='replace')
        with log.open(errors='replace')as stream:
            validation_errors=sum('Vulkan validation:' in line for line in stream)
        row={'map':name,'command':command,'exit_code':code,'seconds':round(time.monotonic()-start,2),
            'passed':code==0 and not validation_errors and f'Rendered {args.frames} frames' in tail,'log_tail':tail,
            'validation_errors':validation_errors,
            'validation_environment':validation}
        if error:row['error']=error
        report['cases'].append(row)
        report['passed']=all(case['passed']for case in report['cases']) and len(report['cases'])==len(names)
        args.report.write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps({key:row[key]for key in ('map','exit_code','seconds','passed')}),flush=True)
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
