#!/usr/bin/env python3
"""Advance authored Armory training through an ordinary player input recording.

The actual MCC scenario creates its player, AI and script state. The only
supplied actions are finite Controls events through the normal Simulation::step
path; no script/result/completion is supplied. Rendered training, full native
campaign ownership, co-op and launcher delivery are outside this observation.
All raw logs, open traces and copied input files belong to HALO2_TEST_SCRATCH.
"""
import argparse,json,os,re,shutil,subprocess,time
from pathlib import Path
from physics_test_cache import runtime_cache,runtime_map
from test_campaign_new_game_runtime import ARMORY,parse_campaign_lines,sha

CAIRO='scenarios\\solo\\01b_spacestation\\01b_spacestation'

def parse_training_lines(lines):
    report=parse_campaign_lines(lines)
    report['script_milestones']=[dict(line=n,text=line.removeprefix('Script: '))for n,line in enumerate(lines,1)if line.startswith('Script: ')]
    report['input_delivery']=[line for line in lines if line.startswith('Campaign input: ')]
    report['input_step_observations']=[]
    for n,line in enumerate(lines,1):
        if match:=re.match(r'^Campaign input: tick=(\d+) input_tick=(\d+)\b',line):
            report['input_step_observations'].append(dict(line=n,map_tick_before=int(match[1]),input_tick=int(match[2])))
    report['input_diagnostics']=[json.loads(line.removeprefix('Campaign input diagnostic: '))for line in lines if line.startswith('Campaign input diagnostic: ')]
    return report

def recorded_input_sequence_matches(report,ticks):
    events=report.get('input_schedule',[])
    expected=[event.get('tick')for event in events]
    if not expected or any(type(tick)is not int or not 0<=tick<ticks for tick in expected):return False
    if expected!=sorted(set(expected)):return False
    observed=[row.get('input_tick')for row in report.get('input_step_observations',[])]
    return expected==observed and len(report.get('input_delivery',[]))==len(expected)

def final_global_input_step_observed(report,ticks):
    if not recorded_input_sequence_matches(report,ticks):return False
    observed=report['input_step_observations'];diagnostics=report.get('input_diagnostics',[])
    final=(report.get('source_status')or[{}])[-1]
    # main.cpp prints this event only after Simulation::step succeeds, then
    # increments its process-wide campaign_input_ticks. Unlike game ticks,
    # that counter is preserved across a natural campaign map transition.
    return bool(observed[-1]['input_tick']==ticks-1 and diagnostics
        and diagnostics[-1].get('input_tick')==ticks-1
        and diagnostics[-1].get('tick')==final.get('tick')
        and observed[-1]['line']<final.get('line',0))

def clean_training_execution(report,ticks):
    initial=report.get('initial_scenarios',[]);statuses=report.get('source_status',[])
    milestones=report.get('required_milestones',[])
    return bool(report.get('returncode')==0 and not report.get('timeout')and initial and statuses
        and initial[0].get('scenario')==ARMORY and initial[0].get('tick')==0
        and initial[0].get('scripts',0)>0 and initial[0].get('level_finished')is False
        and initial[0].get('difficulty')==1
        and all(row.get('script_faults',1)==0 for row in statuses)
        and not report.get('script_faults',True)and not any(report.get('content_error_counts',[1]))
        and not report.get('reference_inputs_opened',True)and not report.get('cache_write_attempts',True)
        and report.get('runtime_unchanged')is True
        and milestones and all(row.get('observed')is True for row in milestones)
        and recorded_input_sequence_matches(report,ticks))

def training_progress_succeeded(report,ticks):
    final=(report.get('source_status')or[{}])[-1]
    return bool(clean_training_execution(report,ticks)
        and len(report['initial_scenarios'])==1 and not report.get('natural_next_scenarios')
        and all(row.get('scenario')==ARMORY for row in report['source_status'])
        and final.get('phase')=='status' and final.get('tick')==ticks)

def armory_to_cairo_succeeded(report,ticks):
    if not clean_training_execution(report,ticks)or not final_global_input_step_observed(report,ticks):return False
    initial=report['initial_scenarios'];transitions=report.get('natural_next_scenarios',[]);statuses=report['source_status']
    if len(initial)!=2 or len(transitions)!=1:return False
    start,fresh=initial;transition=transitions[0];final=statuses[-1]
    if not(start['line']<transition.get('line',0)<fresh['line']<final['line']):return False
    if not(transition.get('scenario')==CAIRO and transition.get('difficulty')==start['difficulty']
        and fresh.get('scenario')==CAIRO and fresh.get('difficulty')==start['difficulty']
        and fresh.get('tick')==0 and fresh.get('bsp')==0 and fresh.get('scripts',0)>0
        and fresh.get('level_finished')is False
        and final.get('scenario')==CAIRO and final.get('phase')=='status'
        and final.get('tick',0)>0 and final.get('level_finished')is False):return False
    completed=[row for row in statuses if row.get('scenario')==ARMORY and row.get('phase')=='status'
        and row.get('tick',0)>0 and row.get('level_finished')is True
        and start['line']<row['line']<transition['line']]
    if len(completed)!=1:return False
    if any(row.get('scenario')not in(ARMORY,CAIRO)or row.get('difficulty')!=start['difficulty']
        or(row['scenario']==ARMORY and row['line']>=transition['line'])
        or(row['scenario']==CAIRO and row['line']<fresh['line'])for row in statuses):return False
    # Completion is observed from the authored ScriptVm game_won and the
    # existing native catalog lookup handoff. It does not certify191750/191580
    # request execution, full native HS ownership or the Cairo mission.
    return report['input_step_observations'][-1]['line']>fresh['line']

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',type=Path,required=True);p.add_argument('--mcc-root',type=Path,required=True)
    p.add_argument('--schedule',type=Path,required=True);p.add_argument('--ticks',type=int,default=2400)
    p.add_argument('--require-line',action='append',default=[]);p.add_argument('--timeout',type=int,default=300)
    p.add_argument('--acceptance',choices=('training_progress','armory_to_cairo'),default='training_progress',
        help='Single-map progress or natural Armory completion and fresh Cairo startup; handoff requires a recorded event at ticks-1')
    p.add_argument('--report-output',type=Path,required=True);a=p.parse_args()
    if 'HALO2_TEST_SCRATCH'not in os.environ:p.error('Run through tools/run_test.py')
    if not 12<=a.ticks<=18000 or not 1<=a.timeout<=600:p.error('Require12..18000ticks and1..600seconds')
    if not a.require_line:p.error('Declare actual authored script milestones with --require-line')
    if not shutil.which('strace'):p.error('Runtime input audit requires strace')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();runtime=a.runtime.resolve(strict=True);cache=a.mcc_root.resolve(strict=True)
    maps,inputs=runtime_cache(cache);map_path=runtime_map(maps,maps/'01a_tutorial.map')
    schedule=scratch/'controls.json';schedule.write_bytes(a.schedule.read_bytes());events=json.loads(schedule.read_text())
    if a.acceptance=='armory_to_cairo'and(not events or events[-1].get('tick')!=a.ticks-1):
        p.error('Handoff acceptance requires a recorded event at ticks-1 to observe the final global accepted step')
    command=[str(runtime),'--campaign','--map',str(map_path),'--shared-maps',str(maps),'--simulate',str(a.ticks),
        '--campaign-input-script',str(schedule),'--mute']
    env={k:v for k,v in os.environ.items()if not k.startswith('HALO2_')or k=='HALO2_TEST_SCRATCH'}
    for key in('XDG_CONFIG_HOME','XDG_DATA_HOME','XDG_CACHE_HOME'):env[key]=str(scratch/key.lower())
    env['SDL_AUDIODRIVER']='dummy';trace=scratch/'opens.log';log=scratch/'runtime.log'
    report=dict(passed=False,scope=__doc__,command=command,runtime=str(runtime),runtime_sha256=sha(runtime),
        map_sha256=sha(map_path),mcc_inputs=inputs,input_schedule=events,input_sha256=sha(schedule),tool_sha256=sha(__file__),
        scratch=str(scratch),retained_large_artifacts=[],full_campaign_qualified=False,native_only_qualified=False,
        rendered_training_qualified=False,launcher_promoted=False)
    began=time.monotonic()
    try:
        with log.open('wb')as stream:
            r=subprocess.run(['strace','--kill-on-exit','-f','-e','trace=openat','-o',str(trace),'--',*command],
                cwd=scratch,env=env,stdout=stream,stderr=subprocess.STDOUT,timeout=a.timeout)
        report['returncode']=r.returncode
    except subprocess.TimeoutExpired:report['timeout']=True
    text=log.read_text(errors='replace');lines=text.splitlines();report.update(parse_training_lines(lines))
    report['required_milestones']=[dict(text=required,observed=any(required in row['text']for row in report['script_milestones']))for required in a.require_line]
    opens=trace.read_text(errors='replace');paths=re.findall(r'openat\([^,]+, "([^"]+)"',opens)
    report['reference_inputs_opened']=sorted({path for path in paths if path.lower().endswith('.xbe')or(path.lower().endswith('.map')and not Path(path).resolve().is_relative_to(cache))})
    report['cache_write_attempts']=[line[:800]for line in opens.splitlines()if str(cache)in line and re.search(r'O_WRONLY|O_RDWR|O_CREAT|O_TRUNC',line)]
    report['runtime_unchanged']=sha(runtime)==report['runtime_sha256']
    final=(report.get('source_status')or[{}])[-1]
    report['acceptance_criterion']=a.acceptance
    report['requested_global_steps']=a.ticks
    report['recorded_input_sequence_matched']=recorded_input_sequence_matches(report,a.ticks)
    report['final_global_step_observed']=final_global_input_step_observed(report,a.ticks)
    report['training_progress_passed']=training_progress_succeeded(report,a.ticks)
    report['armory_to_cairo_passed']=armory_to_cairo_succeeded(report,a.ticks)
    report['passed']=report['armory_to_cairo_passed']if a.acceptance=='armory_to_cairo'else report['training_progress_passed']
    report['armory_to_cairo_handoff_qualified']=a.acceptance=='armory_to_cairo'and report['passed']
    report['transition_scope']='Authored ScriptVm game_won, existing native catalog handoff, fresh Cairo startup; native191750/191580 request execution and Cairo gameplay are not established.'
    report.update(elapsed_seconds=round(time.monotonic()-began,3),log_tail=text[-32000:])
    a.report_output.parent.mkdir(parents=True,exist_ok=True);a.report_output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],required_milestones=report['required_milestones'],final_status=final,
        report=str(a.report_output),log_tail=text[-2000:])))
    return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
