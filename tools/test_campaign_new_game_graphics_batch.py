#!/usr/bin/env python3
"""Explicit final CTest phase: real six-frame gate, then natural opening handoff.

Each actual engine invocation runs through run_test.py with separate owned raw
outputs. A failed short gate prevents the long run. No engine inputs or results
are substituted. This helper runs before the final cohort owner is cleaned up.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def run_cases(cases,report,save,invoke=subprocess.call):
    for case in cases:
        report['cases'].append(case)
        save()
        case['returncode']=invoke(case['command'])
        path=Path(case['report'])
        case['report_present']=path.is_file()
        result=json.loads(path.read_text())if path.is_file()else {}
        case['passed']=(case['returncode']==0 and result.get('passed') is True
                        and result.get('acceptance_criterion')==case['acceptance'])
        if path.is_file():case['report_sha256']=sha(path)
        save()
        if not case['passed']:
            return 1
    report['passed']=True
    save()
    return 0


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--runtime',type=Path,required=True)
    ap.add_argument('--mcc-root',type=Path,required=True)
    ap.add_argument('--frames',type=int,default=30000)
    ap.add_argument('--timeout',type=int,default=900)
    ap.add_argument('--cold-data-check',action='store_true',
                    help='Build-only focused ColdData target through the same owned official worker, then run it')
    ap.add_argument('--report-output',type=Path,required=True)
    args=ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        ap.error('Run through tools/run_test.py')
    if not 6<=args.frames<=30000 or not 1<=args.timeout<=900:
        ap.error('Require6..30000frames and1..900seconds for the long phase')
    runtime=args.runtime.resolve(strict=True)
    tools=Path(__file__).resolve().parent
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    helper=tools/'test_campaign_new_game_graphics.py'
    args.report_output.parent.mkdir(parents=True,exist_ok=True)
    report=dict(passed=False,scope=__doc__,tool_sha256=sha(__file__),helper_sha256=sha(helper),
                runtime_sha256=sha(runtime),cases=[],retained_large_artifacts=[],
                native_only_engine_qualified=False,full_campaign_qualified=False,
                full_armory_training_qualified=False,scratch=str(scratch))
    def save():args.report_output.write_text(json.dumps(report,indent=2)+'\n')
    if args.cold_data_check:
        source=tools.parent;owner=source.parent
        if runtime.parent!=owner/'build' or source.name!='current-source':
            raise ValueError('ColdData compilation requires this active cohort source/build owner')
        output=args.report_output.parent/'cold-data-validation'
        command=[sys.executable,str(tools/'run_tests.py'),'--build-worker','--build-only',
                 '--source',str(source),'--output',str(output),
                 '--target','native_tu15_cold_data_tests',
                 '--target','renderer_framebuffer_alias_tests','--jobs','6']
        env=dict(os.environ);env['HALO2_TEST_SCRATCH']=str(owner)
        build_case=dict(acceptance='cold_data_build_only',command=command,passed=False,
                        validation_not_run=True)
        report['cases'].append(build_case);save()
        try:
            build_case['returncode']=subprocess.call(command,env=env,stdout=subprocess.DEVNULL,timeout=300)
        except subprocess.TimeoutExpired:
            build_case.update(returncode=124,timeout=True)
        official=output/'report.json'
        build_case['official_report_sha256']=sha(official)if official.is_file()else None
        build_case['passed']=(build_case['returncode']==0 and official.is_file()
                              and json.loads(official.read_text()).get('status')=='built')
        save()
        if not build_case['passed']:
            return 1
        binary=owner/'build/native_tu15_cold_data_tests'
        command=[sys.executable,str(tools/'run_test.py'),'--scratch-root',str(scratch/'cold-data-case'),
                 '--',str(binary)]
        checked=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=120)
        test_case=dict(acceptance='cold_data_ownership',command=command,returncode=checked.returncode,
                       binary_sha256=sha(binary),log_tail=checked.stdout[-8192:],passed=checked.returncode==0)
        report['cases'].append(test_case);save()
        if not test_case['passed']:
            return 1
        result=args.report_output.parent/'renderer-framebuffer-alias.json'
        command=[sys.executable,str(tools/'run_test.py'),'--scratch-root',str(scratch/'renderer-case'),
                 '--',sys.executable,'-B',str(tools/'test_renderer_framebuffer_alias_case.py'),
                 '--binary',str(owner/'build/renderer_framebuffer_alias_tests'),
                 '--shaders',str(owner/'build/shaders'),'--report-output',str(result)]
        renderer_case=dict(acceptance='renderer_framebuffer_aliases',command=command,report=str(result),passed=False)
        report['cases'].append(renderer_case);save()
        renderer_case['returncode']=subprocess.call(command)
        if result.is_file():
            renderer_case['passed']=renderer_case['returncode']==0 and json.loads(result.read_text())['passed']
            renderer_case['report_sha256']=sha(result)
        save()
        if not renderer_case['passed']:return 1
    cases=[]
    for mode,frames,timeout,leaf in [('graphics_startup',6,300,'graphics-cold.json'),
                                    ('opening_handoff',args.frames,args.timeout,'graphics-progression.json')]:
        result=args.report_output.parent/leaf
        command=[sys.executable,str(tools/'run_test.py'),'--scratch-root',str(scratch/'gpu-cases'),'--',
                 sys.executable,'-B',str(helper),'--runtime',str(runtime),
                 '--mcc-root',str(args.mcc_root.resolve(strict=True)),'--frames',str(frames),
                 '--timeout',str(timeout),'--acceptance',mode,'--report-output',str(result)]
        cases.append(dict(acceptance=mode,frames=frames,timeout_seconds=timeout,
                          command=command,report=str(result),passed=False))
    code=run_cases(cases,report,save)
    report['runtime_unchanged']=sha(runtime)==report['runtime_sha256']
    report['passed']=report['passed']and report['runtime_unchanged']
    save()
    print(json.dumps(dict(passed=report['passed'],cases=len(report['cases']),report=str(args.report_output))))
    return code if code else (0 if report['passed']else 1)


if __name__=='__main__':
    raise SystemExit(main())
