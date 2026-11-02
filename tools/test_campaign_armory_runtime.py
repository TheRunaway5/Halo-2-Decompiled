#!/usr/bin/env python3
"""Observe natural authored Armory startup and both native authored-area callbacks.

The ordinary MCC campaign CLI creates the real player and runs authored
scripts with neutral input. No actor, squad, script, completion or result is
supplied. This bounded headless observation does not qualify rendered training,
the Heretic handoff, complete native-only gameplay, co-op or launcher delivery.
Run through tools/run_test.py; traces and raw logs remain in its owned scratch.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

from physics_test_cache import runtime_cache, runtime_map
from test_campaign_new_game_runtime import ARMORY, parse_campaign_lines, sha


def armory_execution_succeeded(report,ticks):
    initial=report.get('initial_scenarios',[])
    final=(report.get('source_status')or[{}])[-1]
    return bool(report.get('returncode')==0 and not report.get('timeout')
                and initial and initial[0]['scenario']==ARMORY and initial[0]['tick']==0
                and initial[0]['scripts']>0 and final.get('scenario')==ARMORY
                and final.get('tick')==ticks and final.get('script_faults')==0
                and report.get('native_area_count_calls',0)>0
                and report.get('native_area_choice_calls',0)>0
                and not report.get('script_faults')
                and not any(report.get('content_error_counts',[]))
                and not report.get('reference_inputs_opened',True)
                and not report.get('cache_write_attempts',True)
                and report.get('runtime_unchanged') is True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime',type=Path,required=True)
    parser.add_argument('--mcc-root',type=Path,required=True)
    parser.add_argument('--ticks',type=int,default=120)
    parser.add_argument('--timeout',type=int,default=300)
    parser.add_argument('--report-output',type=Path,required=True)
    args=parser.parse_args()
    if 'HALO2_TEST_SCRATCH'not in os.environ:
        parser.error('Run through tools/run_test.py')
    if not 12<=args.ticks<=1800 or not 1<=args.timeout<=600:
        parser.error('Require12..1800ticks and1..600seconds')
    if shutil.which('strace')is None:
        parser.error('Runtime input audit requires strace')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    runtime=args.runtime.resolve(strict=True)
    cache=args.mcc_root.resolve(strict=True)
    maps,inputs=runtime_cache(cache)
    map_path=runtime_map(maps,maps/'01a_tutorial.map')
    command=[str(runtime),'--campaign','--map',str(map_path),'--shared-maps',str(maps),
             '--simulate',str(args.ticks),'--mute']
    env={key:value for key,value in os.environ.items()
         if not key.startswith('HALO2_')or key=='HALO2_TEST_SCRATCH'}
    for key in ('XDG_CONFIG_HOME','XDG_DATA_HOME','XDG_CACHE_HOME'):
        env[key]=str(scratch/key.lower())
    env['SDL_AUDIODRIVER']='dummy'
    trace,log=scratch/'opens.log',scratch/'runtime.log'
    report=dict(passed=False,scope=__doc__,command=command,runtime=str(runtime),
                runtime_sha256=sha(runtime),map_sha256=sha(map_path),mcc_inputs=inputs,
                tool_sha256=sha(__file__),scratch=str(scratch),retained_large_artifacts=[],
                full_campaign_qualified=False,native_only_qualified=False,
                rendered_training_qualified=False,launcher_promoted=False)
    started=time.monotonic()
    try:
        with log.open('wb')as stream:
            result=subprocess.run(['strace','--kill-on-exit','-f','-e','trace=openat',
                                   '-o',str(trace),'--',*command],cwd=scratch,env=env,
                                  stdout=stream,stderr=subprocess.STDOUT,timeout=args.timeout)
        report['returncode']=result.returncode
    except subprocess.TimeoutExpired:
        report['timeout']=True
    text=log.read_text(errors='replace')
    report.update(parse_campaign_lines(text.splitlines()))
    activity=re.findall(r'\bnative_ai_area_count=(\d+)',text)
    report['native_area_count_calls']=int(activity[-1])if activity else 0
    choice=re.findall(r'\bnative_ai_area_choice=(\d+)',text)
    report['native_area_choice_calls']=int(choice[-1])if choice else 0
    opens=trace.read_text(errors='replace')
    paths=re.findall(r'openat\([^,]+, "([^"]+)"',opens)
    report['reference_inputs_opened']=sorted({value for value in paths
        if value.lower().endswith('.xbe')or(value.lower().endswith('.map')
            and not Path(value).resolve().is_relative_to(cache))})
    report['cache_write_attempts']=[line[:800]for line in opens.splitlines()
        if str(cache)in line and re.search(r'O_WRONLY|O_RDWR|O_CREAT|O_TRUNC',line)]
    report['runtime_unchanged']=sha(runtime)==report['runtime_sha256']
    report.update(elapsed_seconds=round(time.monotonic()-started,3),log_tail=text[-32000:])
    report['passed']=armory_execution_succeeded(report,args.ticks)
    args.report_output.parent.mkdir(parents=True,exist_ok=True)
    args.report_output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],report=str(args.report_output),
                         final_status=(report['source_status']or[{}])[-1],
                         native_area_count_calls=report['native_area_count_calls'],
                         native_area_choice_calls=report['native_area_choice_calls'],
                         log_tail=text[-2000:])))
    return int(not report['passed'])


if __name__=='__main__':
    raise SystemExit(main())
