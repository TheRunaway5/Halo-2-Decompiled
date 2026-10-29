#!/usr/bin/env python3
"""Observe bounded natural campaign startup from the actual MCC introduction.

The separate linked frontend test establishes menu/map/difficulty handoff.
This probe starts the ordinary campaign CLI with neutral input. It supplies no
script calls, placed squads, player births, completion requests or service
results. The longer budget separately checks the observed Heretic-to-Armory
handoff. Neither check qualifies full cinematics, Armory training responses,
rendering, co-op, or complete native campaign execution. Run through tools/run_test.py.
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

from physics_test_cache import runtime_cache, runtime_map


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def startup_succeeded(report, ticks):
    final = (report.get('source_status') or [{}])[-1]
    return (report.get('returncode') == 0 and final.get('tick') == ticks
            and final.get('script_faults') == 0
            and not report.get('script_faults')
            and not report.get('reference_inputs_opened', True)
            and not report.get('cache_write_attempts', True)
            and not report.get('trace_unavailable', True))


INTRODUCTION = 'scenarios\\solo\\00a_introduction\\00a_introduction'
ARMORY = 'scenarios\\solo\\01a_tutorial\\01a_tutorial'


def parse_campaign_lines(lines):
    """Preserve actual ordered per-map observations, including the initial clock."""
    report = dict(source_status=[], initial_scenarios=[], natural_next_scenarios=[],
                  script_faults=[], content_error_counts=[])
    scenario, difficulty = INTRODUCTION, None
    for ordinal,line in enumerate(lines,1):
        initial = re.search(r'^Campaign scenario started: (.*?) difficulty=(\d+) ',line)
        if initial:
            scenario,difficulty=initial.group(1),int(initial.group(2))
        transition = re.search(r'^Campaign next scenario: (.*?) difficulty=(\d+)\s*$',line)
        if transition:
            scenario,difficulty=transition.group(1),int(transition.group(2))
            report['natural_next_scenarios'].append(dict(scenario=scenario,difficulty=difficulty,line=ordinal))
        status = re.search(r'tick=(\d+) bsp=(\d+).*?actors=(\d+).*?scripts=(\d+).*?script_faults=(\d+)',line)
        if status:
            row=dict(zip(('tick','bsp','actors','scripts','script_faults'),map(int,status.groups())))
            finished=re.search(r'\blevel_finished=([01])\b',line)
            row.update(scenario=scenario,difficulty=difficulty,line=ordinal,
                       phase='initial' if initial else 'status',
                       level_finished=bool(int(finished.group(1))) if finished else None)
            report['source_status'].append(row)
            if initial:report['initial_scenarios'].append(row)
        if line.startswith('Script fault: '):
            report['script_faults'].append(line.removeprefix('Script fault: ').rstrip())
        report['content_error_counts'].extend(map(int,re.findall(r'\bcontent_errors=(\d+)\b',line)))
    return report


def opening_handoff_succeeded(report):
    """Bounded natural Heretic completion and fresh authored Armory activation."""
    if (report.get('returncode') != 0 or report.get('timeout') or report.get('script_faults')
            or any(report.get('content_error_counts',[]))
            or report.get('reference_inputs_opened',True)
            or report.get('cache_write_attempts',True) or report.get('trace_unavailable',True)
            or any(row['script_faults'] for row in report.get('source_status',[]))):
        return False
    initial=report.get('initial_scenarios',[])
    transitions=report.get('natural_next_scenarios',[])
    if not initial or not transitions:
        return False
    heretic=initial[0]
    next_map=transitions[0]
    if (heretic['scenario'] != INTRODUCTION or heretic['tick'] != 0 or heretic['bsp'] != 0
            or heretic['scripts'] <= 0 or heretic['level_finished'] is not False
            or heretic['difficulty'] != 1 or next_map['scenario'] != ARMORY
            or next_map['difficulty'] != heretic['difficulty']):
        return False
    completed=[row for row in report['source_status'] if row['scenario']==INTRODUCTION
               and row['phase']=='status' and row['level_finished'] is True and row['tick']>0
               and heretic['line']<row['line']<next_map['line']]
    if len(initial)<2:
        return False
    armory=initial[1]
    fresh=(armory['scenario']==ARMORY and armory['tick']==0 and armory['bsp']==0
           and armory['scripts']>0 and armory['level_finished'] is False
           and armory['difficulty']==heretic['difficulty'] and armory['line']>next_map['line'])
    return bool(completed and fresh and any(row['scenario']==ARMORY and row['phase']=='status'
        and row['tick']>0 and row['line']>armory['line'] for row in report['source_status']))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--runtime', type=Path, required=True)
    ap.add_argument('--mcc-root', type=Path, required=True)
    ap.add_argument('--ticks', type=int, default=120,help='Explicit bounded budget1..18000')
    ap.add_argument('--acceptance',choices=('cold_startup','opening_handoff'),default='cold_startup',
                    help='Select the acceptance purpose independently of the tick budget')
    ap.add_argument('--timeout', type=int, default=300)
    ap.add_argument('--report-output', type=Path, required=True)
    args = ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        ap.error('Run through tools/run_test.py')
    if not 1 <= args.ticks <= 18000 or not 1 <= args.timeout <= 1800:
        ap.error('Require1..18000ticks and1..1800seconds')
    if shutil.which('strace') is None:
        ap.error('Natural startup requires strace to audit actual runtime inputs')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    runtime = args.runtime.resolve(strict=True)
    maps, inputs = runtime_cache(args.mcc_root)
    map_path = runtime_map(maps, maps / '00a_introduction.map')
    cache = args.mcc_root.resolve(strict=True)
    command = [str(runtime), '--campaign', '--map', str(map_path),
               '--shared-maps', str(maps), '--simulate', str(args.ticks), '--mute']
    env = {key: value for key, value in os.environ.items()
           if not key.startswith('HALO2_') or key == 'HALO2_TEST_SCRATCH'}
    for key in ('XDG_CONFIG_HOME', 'XDG_DATA_HOME', 'XDG_CACHE_HOME'):
        env[key] = str(scratch / key.lower())
    env['SDL_AUDIODRIVER'] = 'dummy'
    log, trace = scratch / 'new-game-startup.log', scratch / 'new-game-opens.log'
    report = dict(passed=False, scope=__doc__, command=command,
                  runtime_sha256=sha(runtime), map_sha256=sha(map_path),
                  mcc_inputs=inputs, tool_sha256=sha(__file__), scratch=str(scratch),
                  requested_ticks=args.ticks, retained_large_artifacts=[],
                  source_status=[], script_calls_supplied=[], squads_placed=[],
                  reference_inputs_opened=[], cache_write_attempts=[],
                  trace_unavailable=True, full_campaign_qualified=False,
                  native_only_engine_qualified=False, rendered_or_cooperative_acceptance=False,
                  frontend_menu_executed_by_this_probe=False,
                  opening_progression_qualified=False,
                  armory_training_response_qualified=False,
                  cli_difficulty='ordinary CLI default Normal; four menu choices qualified separately')
    started = time.monotonic()
    try:
        with log.open('w') as stream:
            result = subprocess.run(['strace', '--kill-on-exit', '-f', '-e',
                'trace=openat', '-o', str(trace), '--', *command], cwd=scratch,
                env=env, stdout=stream, stderr=subprocess.STDOUT, timeout=args.timeout)
        report['returncode'] = result.returncode
    except subprocess.TimeoutExpired:
        report['timeout'] = True
    report['elapsed_seconds'] = round(time.monotonic() - started, 3)
    with log.open('rb') as stream:
        stream.seek(max(0, log.stat().st_size - 32768))
        text = stream.read().decode('utf-8', errors='replace')
    report['log_tail'] = text
    report['log_sha256'] = sha(log)
    report['log_bytes'] = log.stat().st_size
    with log.open(errors='replace') as stream:
        report.update(parse_campaign_lines(stream))
    if trace.is_file():
        opens = trace.read_text(errors='replace')
        report['trace_unavailable'] = False
        paths = re.findall(r'openat\([^,]+, "([^"]+)"', opens)
        report['reference_inputs_opened'] = sorted({path for path in paths
            if path.lower().endswith(('.xbe', '.exe')) or
            (path.lower().endswith('.map') and not Path(path).resolve().is_relative_to(cache))})
        report['cache_write_attempts'] = [line[:800] for line in opens.splitlines()
            if str(cache) in line and re.search(r'O_WRONLY|O_RDWR|O_CREAT|O_TRUNC', line)]
    report['strict_single_map_startup_budget_matched'] = startup_succeeded(report,args.ticks)
    report['opening_handoff_passed'] = opening_handoff_succeeded(report)
    report['acceptance_criterion'] = args.acceptance
    report['passed'] = (report['opening_handoff_passed'] if args.acceptance=='opening_handoff'
                        else report['strict_single_map_startup_budget_matched'])
    report['heretic_to_armory_handoff_qualified'] = args.acceptance=='opening_handoff' and report['passed']
    report['startup_criterion_scope'] = 'Exact final game tick equals requested budget; natural multi-map progression is reported separately and is not qualified by this predicate'
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], returncode=report.get('returncode'),
        source_status=report['source_status'][-1] if report['source_status'] else {},
        report=str(args.report_output), log_tail=text[-1800:])))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
