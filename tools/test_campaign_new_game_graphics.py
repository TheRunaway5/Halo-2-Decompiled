#!/usr/bin/env python3
"""Observe the ordinary finite campaign render loop with actual frame consumers.

No --simulate, completion calls, placed squads, reference captures, injected
input, screenshots or success callbacks are used. The hidden SDL/Vulkan window
still renders actual frames. An absent display uses an owned Xvfb display; the
reported Vulkan device distinguishes hardware from software implementations.
Raw process logs/traces remain in tools/run_test.py scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

from physics_test_cache import runtime_cache,runtime_map
from test_campaign_new_game_runtime import parse_campaign_lines,opening_handoff_succeeded


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--runtime',type=Path,required=True)
    ap.add_argument('--mcc-root',type=Path,required=True)
    ap.add_argument('--frames',type=int,default=6)
    ap.add_argument('--timeout',type=int,default=300)
    ap.add_argument('--acceptance',choices=('graphics_startup','opening_handoff'),required=True)
    ap.add_argument('--report-output',type=Path,required=True)
    args=ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        ap.error('Run through tools/run_test.py')
    if not 1<=args.frames<=30000 or not 1<=args.timeout<=1800:
        ap.error('Require1..30000actualframes and1..1800seconds')
    if shutil.which('strace') is None:
        ap.error('strace is required to audit actual runtime inputs')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    runtime=args.runtime.resolve(strict=True)
    maps,inputs=runtime_cache(args.mcc_root)
    cache=args.mcc_root.resolve(strict=True)
    intro=runtime_map(maps,maps/'00a_introduction.map')
    command=[str(runtime),'--campaign','--map',str(intro),'--shared-maps',str(maps),
             '--frames',str(args.frames),'--hidden','--mute','--viewport','640','360']
    env={key:value for key,value in os.environ.items()
         if not key.startswith('HALO2_') or key=='HALO2_TEST_SCRATCH'}
    for key in ('XDG_CONFIG_HOME','XDG_DATA_HOME','XDG_CACHE_HOME'):
        env[key]=str(scratch/key.lower())
    env['SDL_AUDIODRIVER']='dummy'
    log,trace=scratch/'campaign-graphics.log',scratch/'campaign-graphics-opens.log'
    invocation=['strace','--kill-on-exit','-f','-e','trace=openat','-o',str(trace),'--',*command]
    owned_display=not bool(env.get('DISPLAY'))
    if owned_display:
        if shutil.which('xvfb-run') is None:
            ap.error('A real display or xvfb-run is required')
        invocation=['xvfb-run','-a','-s','-screen 0 1280x720x24',*invocation]
    report=dict(passed=False,scope=__doc__,acceptance_criterion=args.acceptance,
        command=command,invocation=invocation,tool_sha256=sha(__file__),
        runtime_sha256=sha(runtime),mcc_inputs=inputs,map_sha256=sha(intro),
        requested_frames=args.frames,timeout_seconds=args.timeout,owned_xvfb=owned_display,
        scratch=str(scratch),retained_large_artifacts=[],full_campaign_qualified=False,
        native_only_engine_qualified=False,full_armory_training_qualified=False,
        framebuffer_parity_qualified=False,rendered_or_cooperative_acceptance=False,
        script_calls_supplied=[],squads_placed=[],input_or_completion_injected=False)
    started=time.monotonic()
    try:
        with log.open('w') as stream:
            completed=subprocess.run(invocation,cwd=scratch,env=env,stdout=stream,
                stderr=subprocess.STDOUT,timeout=args.timeout)
        report['returncode']=completed.returncode
    except subprocess.TimeoutExpired:
        report['timeout']=True
    report['elapsed_seconds']=round(time.monotonic()-started,3)
    with log.open('rb') as stream:
        stream.seek(max(0,log.stat().st_size-32768))
        text=stream.read().decode('utf-8',errors='replace')
    report['log_tail']=text
    report['log_sha256']=sha(log);report['log_bytes']=log.stat().st_size
    with log.open(errors='replace') as stream:
        report.update(parse_campaign_lines(stream))
    report['vulkan_devices']=re.findall(r'^Vulkan device: (.*)$',text,re.MULTILINE)
    report['rendered_frames']=list(map(int,re.findall(r'^Rendered (\d+) frames$',text,re.MULTILINE)))
    report['runtime_sha256_after']=sha(runtime)
    report['runtime_unchanged']=report['runtime_sha256_after']==report['runtime_sha256']
    report['trace_unavailable']=not trace.is_file()
    report['reference_inputs_opened']=[];report['cache_write_attempts']=[]
    if trace.is_file():
        opens=trace.read_text(errors='replace')
        paths=re.findall(r'openat\([^,]+, "([^"]+)"',opens)
        report['reference_inputs_opened']=sorted({p for p in paths if
            p.lower().endswith(('.xbe','.exe')) or
            (p.lower().endswith('.map') and not Path(p).resolve().is_relative_to(cache))})
        report['cache_write_attempts']=[line[:800]for line in opens.splitlines()
            if str(cache)in line and re.search(r'O_WRONLY|O_RDWR|O_CREAT|O_TRUNC',line)]
    clean=(report.get('returncode')==0 and not report.get('timeout')
           and report['runtime_unchanged'] and not report['trace_unavailable']
           and not report['reference_inputs_opened'] and not report['cache_write_attempts']
           and not report['script_faults'] and not any(report['content_error_counts'])
           and not any(row['script_faults']for row in report['source_status']))
    report['graphics_startup_passed']=bool(clean and report['vulkan_devices']
        and report['rendered_frames'] and sum(report['rendered_frames'])==args.frames)
    report['opening_handoff_passed']=bool(clean and report['vulkan_devices']
        and report['rendered_frames'] and opening_handoff_succeeded(report))
    report['passed']=(report['graphics_startup_passed']if args.acceptance=='graphics_startup'
                      else report['opening_handoff_passed'])
    args.report_output.parent.mkdir(parents=True,exist_ok=True)
    args.report_output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],returncode=report.get('returncode'),
        vulkan_devices=report['vulkan_devices'],source_status=report['source_status'],
        report=str(args.report_output),log_tail=text[-2200:])))
    return 0 if report['passed']else 1


if __name__=='__main__':
    raise SystemExit(main())
