#!/usr/bin/env python3
"""Run a version-pinned ordinary-runtime performance/equivalence cohort.

Raw frames, profiler samples and shader caches belong to run_test.py storage.
Only bounded reports, commands, hashes and frame-time tables survive. Passing
this cohort is evidence for one map, not completion of the full performance goal.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import statistics
import subprocess
import sys
import time

ROOT=Path(__file__).resolve().parents[1]
REFERENCE='HALO2_REFERENCE_SCALAR_VECTOR_READS'


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def tail(path,limit=12000):
    with path.open('rb') as stream:
        stream.seek(max(0,path.stat().st_size-limit))
        return stream.read(limit).decode(errors='replace')


def validation_errors(path):
    """Scan the complete transient log, including errors before its retained tail."""
    if not path.exists():
        return 0
    with path.open(errors='replace') as stream:
        return sum('Validation Error' in line or 'VUID-' in line for line in stream)


def read_capture(image, timing, expected_size):
    payload=image.read_bytes()
    header=payload.split(b'\n',3)
    if len(header)!=4 or header[0]!=b'P6' or header[2]!=b'255':
        raise ValueError('Native capture did not produce its complete P6 output')
    dimensions=list(map(int,header[1].split()))
    if dimensions!=expected_size or len(header[3])!=dimensions[0]*dimensions[1]*3:
        raise ValueError('Capture is not the complete requested internal resolution')
    with timing.open() as stream:
        rows=list(csv.DictReader(stream,delimiter='\t'))
    for kind in ('render','present'):
        sizes={(int(row[kind+'_width']),int(row[kind+'_height'])) for row in rows}
        if sizes!={tuple(expected_size)}:
            raise ValueError('Capture timing does not prove full-size scene and presentation')
    return header[3],digest(image)


def worker(args):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True,exist_ok=True)
    report={'schema':'H2_PERFORMANCE_GOAL_COHORT_V1','runtime':str(args.runtime),
        'runtime_sha256':digest(args.runtime),'map':str(args.map),'map_sha256':digest(args.map),
        'helper_sha256':digest(__file__),'scratch':str(scratch),'stages':args.stage,
        'render_size':args.render_size,'present_window':[1280,720],
        'capture_window':args.render_size,
        'capture_provider':'Isolated X11 at full scene resolution; quality evidence only, never FPS acceptance',
        'steps':[],'captures':[],'benchmarks':[],'profiles':[],
        'cohort_gates_passed':False,
        'limits':'One map and this host; reference/optimized controls use one executable. '
                 'A/B framebuffer equality is not full OG Xbox parity. CPU sampling is '
                 'separate from unsampled FPS acceptance. No physical-display timing claim.'}
    def save():
        (args.output/'cohort.json').write_text(json.dumps(report,indent=2)+'\n')
    def environment(reference=False):
        env=dict(os.environ,SDL_VIDEODRIVER='x11',HALO2_MENU_FOG_SEED='1',
            SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT='0x0000/0x0000',
            SDL_WINDOW_NO_ACTIVATION_WHEN_SHOWN='1',SDL_WINDOW_ACTIVATE_WHEN_SHOWN='0',
            SDL_WINDOW_ACTIVATE_WHEN_RAISED='0',
            HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch/'vulkan-cache'),
            MESA_SHADER_CACHE_DIR=str(scratch/'mesa-cache'),
            __GL_SHADER_DISK_CACHE_PATH=str(scratch/'nvidia-cache'))
        env.pop(REFERENCE,None)
        if reference:env[REFERENCE]='1'
        return env
    def run(name,command,reference=False,timeout=None,extra_env=None):
        env=environment(reference);log=scratch/(name+'.log')
        env.update(extra_env or {})
        step={'name':name,'command':command,'reference_controls':
              {k:v for k,v in env.items() if k.startswith('HALO2_REFERENCE_')},
              'host_load_start':os.getloadavg(),'started':time.time(),'exit_code':None}
        report['steps'].append(step);save();print('Starting '+name,flush=True)
        # A timed capture can include xvfb-run and its native child. A nested
        # owned supervisor retires the entire child group after timeout or
        # cancellation, before this cohort advances to the next case.
        wrapped=command
        if timeout is not None:
            wrapped=[sys.executable,'-B',str(ROOT/'tools/run_test.py'),
                '--scratch-root',str(ROOT/'.local/test-scratch'),'--',sys.executable,'-B','-c',
                'import subprocess,sys;raise SystemExit(subprocess.run(sys.argv[2:],timeout=float(sys.argv[1])).returncode)',
                str(timeout),*command]
            step['owned_command']=wrapped
        try:
            with log.open('w') as stream:
                result=subprocess.run(wrapped,cwd=ROOT,env=env,stdout=stream,
                                      stderr=subprocess.STDOUT)
            step['exit_code']=result.returncode
        except OSError as error:
            step.update(exit_code=1,error=str(error))
        finally:
            step.update(finished=time.time(),host_load_end=os.getloadavg())
            step['log_tail']=tail(log) if log.exists() else ''
            step['validation_error_lines']=validation_errors(log)
            save();print(name+': exit '+str(step['exit_code']),flush=True)
        return step
    try:
        if 'capture' in args.stage:
            for players in args.players:
                for action in ('neutral','fire'):
                    tick=3 if action=='neutral' else 20
                    script=scratch/f'{action}-{players}.tsv'
                    script.write_text('# tick player fire reload\n'+''.join(
                        f'{t} {p} {int(action=="fire" and 5<=t<25)} 0\n'
                        for t in range(tick) for p in range(players)))
                    images=[];failures=[]
                    for reference in (True,False):
                        name=f'{action}-{players}p-'+('reference' if reference else 'optimized')
                        image=scratch/(name+'.ppm')
                        timing=scratch/(name+'-timing.tsv')
                        command=['xvfb-run','--auto-servernum','--server-args',
                            f'-screen 0 {args.render_size[0]}x{args.render_size[1]}x24',
                            str(args.runtime),'--multiplayer','--map',str(args.map),
                            '--shared-maps',str(args.shared_maps),'--local-players',str(players),
                            '--viewport',*map(str,args.render_size),'--render-size',*map(str,args.render_size),
                            '--mp-input-script',str(script),'--mp-capture-tick',str(tick),
                            '--mp-reference-tick-cadence','--mp-reference-render-fraction','0',
                            '--mp-reference-auxiliary-seed','1','--screenshot',str(image),
                            '--mute','--validation']
                        step=run(name,command,reference,timeout=args.timeout,
                                 extra_env={'HALO2_RENDER_TIMINGS':str(timing)})
                        if step['exit_code'] or not image.exists():
                            failures.append(name+': native capture failed or output missing')
                            images.append(None);continue
                        try:
                            rgb,sha256=read_capture(image,timing,args.render_size)
                            images.append((rgb,sha256,step['validation_error_lines']))
                        except (OSError,ValueError,KeyError) as error:
                            failures.append(name+': '+str(error));images.append(None)
                    result={'players':players,'action':action,'tick':tick,'passed':False,
                            'failures':failures}
                    if all(image is not None for image in images):
                        a,b=images
                        equal=a[0]==b[0]
                        result.update(equal_rgb=equal,reference_sha256=a[1],optimized_sha256=b[1],
                            validation_error_lines=a[2]+b[2],
                            max_channel_error=0 if equal else max(abs(x-y) for x,y in zip(a[0],b[0])),
                            passed=equal and not a[2] and not b[2])
                    report['captures'].append(result);save()
        if 'benchmark' in args.stage:
            for players in args.players:
                for pair in range(args.repetitions):
                    for reference in ((True,False) if pair%2==0 else (False,True)):
                        name=f'{players}p-pair{pair+1}-'+('reference' if reference else 'optimized')
                        output=scratch/name;summary=args.output/(name+'.json')
                        target=120 if players==1 else 60
                        command=[sys.executable,'-B',str(ROOT/'tools/benchmark_framerate.py'),
                            '--runtime',str(args.runtime),'--map',str(args.map),
                            '--shared-maps',str(args.shared_maps),'--scenario','split',
                            '--local-players',str(players),'--starting-weapon','0','--native-hud',
                            '--width','1280','--height','720','--render-size',*map(str,args.render_size),
                            '--frames',str(args.frames),'--warmup',str(args.warmup),
                            '--repetitions','1','--min-fps',str(target),
                            '--max-frame-ms',str(1000/target),'--timeout',str(args.timeout),
                            '--output',str(output),'--report-output',str(summary)]
                        step=run(name,command,reference)
                        entry={'players':players,'pair':pair,'reference':reference,
                               'report':str(summary),'complete':False,
                               'exit_code':step['exit_code'],'target_fps':target}
                        if summary.exists():
                            data=json.loads(summary.read_text());sample=data['runs'][0]
                            if data['runtime_sha256']!=report['runtime_sha256']:
                                raise RuntimeError('Benchmark executable identity changed')
                            entry.update(complete=True,fps=data['median_fps']['split'],
                                frame_budget=sample['frame_budget'],dimensions=sample['dimensions'])
                            for timing in output.glob('*.tsv'):
                                if timing.stat().st_size>1024*1024:raise RuntimeError('Timing report exceeds budget')
                                (args.output/(name+'-'+timing.name)).write_bytes(timing.read_bytes())
                        report['benchmarks'].append(entry);save()
        if 'profile' in args.stage:
            for players in (1,2,4):
                output=args.output/f'{players}p-profile.json'
                command=[sys.executable,'-B',str(ROOT/'tools/profile_multiplayer.py'),'--worker',
                    '--runtime',str(args.runtime),'--map',str(args.map),
                    '--shared-maps',str(args.shared_maps),'--players',str(players),
                    '--width','1280','--height','720','--frames',str(args.frames),
                    '--warmup',str(args.warmup),'--timeout',str(args.timeout),'--output',str(output)]
                step=run(f'{players}p-profile',command)
                report['profiles'].append({'players':players,'report':str(output),
                    'completed':step['exit_code']==0 and output.exists()});save()
        optimized=[r for r in report['benchmarks'] if not r['reference']]
        report['optimized_median_fps']={str(p):statistics.median(r['fps'] for r in optimized
            if r['players']==p and r['complete']) for p in args.players
            if any(r['players']==p and r['complete'] for r in optimized)}
        gates={
            'benchmark':bool(optimized) and all(r['complete'] and r['exit_code']==0 and
                math.isfinite(r['fps']) and r['fps']>=r['target_fps'] and
                r['frame_budget']['passed'] for r in optimized),
            'capture':bool(report['captures']) and all(r['passed'] for r in report['captures']),
            'profile':bool(report['profiles']) and all(r['completed'] for r in report['profiles'])}
        report['stage_gates']=gates
        report['requested_stage_gates_passed']=all(gates[stage] for stage in args.stage)
        report['cohort_gates_passed']=set(args.stage)==set(gates) and all(gates.values())
        report['runtime_identity_preserved']=digest(args.runtime)==report['runtime_sha256']
        if not report['runtime_identity_preserved']:raise RuntimeError('Runtime changed during cohort')
    finally:
        save()
        sizes=[p.stat().st_size for p in args.output.iterdir() if p.is_file()]
        if any(size>1024*1024 for size in sizes) or sum(sizes)>8*1024*1024:
            raise RuntimeError('Retained cohort exceeds small-report budget')
    return 0 if report.get('requested_stage_gates_passed',False) else 1


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime',type=Path,required=True)
    parser.add_argument('--map',type=Path,required=True)
    parser.add_argument('--shared-maps',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--players',type=int,choices=(1,4),action='append')
    parser.add_argument('--stage',choices=('capture','benchmark','profile'),action='append')
    parser.add_argument('--render-size',type=int,nargs=2,default=[3840,2160])
    parser.add_argument('--frames',type=int,default=210)
    parser.add_argument('--warmup',type=int,default=30)
    parser.add_argument('--repetitions',type=int,default=3)
    parser.add_argument('--timeout',type=float,default=300)
    parser.add_argument('--worker',action='store_true',help=argparse.SUPPRESS)
    args=parser.parse_args()
    if not (0<args.warmup<args.frames<=100000 and args.repetitions>0 and
            math.isfinite(args.timeout) and args.timeout>0 and
            all(64<=n<=4096 for n in args.render_size)):
        parser.error('Require bounded positive workload, warmup below frames and64..4096 scene dimensions')
    args.runtime=args.runtime.resolve(strict=True);args.map=args.map.resolve(strict=True)
    args.shared_maps=(args.shared_maps or args.map.parent).resolve(strict=True)
    args.output=args.output.resolve();args.players=list(dict.fromkeys(args.players or [1,4]))
    args.stage=list(dict.fromkeys(args.stage or ['capture','benchmark','profile']))
    if sys.platform!='linux':parser.error('This current native CPU/profile cohort requires Linux')
    if 'capture' in args.stage and not shutil.which('xvfb-run'):
        parser.error('Full-resolution capture requires the existing xvfb-run provider')
    if args.worker:
        return worker(args)
    return subprocess.call([sys.executable,'-B',str(ROOT/'tools/run_test.py'),
        '--scratch-root',str(ROOT/'.local/test-scratch'),'--',sys.executable,'-B',
        str(Path(__file__).resolve()),'--worker',*sys.argv[1:]])


if __name__=='__main__':
    raise SystemExit(main())
