#!/usr/bin/env python3
"""Capture real SDL glow inputs, compare original consumers, preserve native failure.

Run through tools/run_test.py. Raw GPU inputs live only in HALO2_TEST_SCRATCH.
The bounded input JSON is a required reproducible comparison fixture, not a frame
capture or executable. Full native firing failures keep their original verdict.
"""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--inputs-report',type=Path,required=True)
    parser.add_argument('--original-report',type=Path,required=True)
    parser.add_argument('command',nargs=argparse.REMAINDER)
    args=parser.parse_args();command=args.command
    if command and command[0]=='--':command=command[1:]
    assert command
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);capture=scratch/'weapon-live-glow.lvg'
    environment=os.environ.copy();environment['HALO2_TEST_GLOW_RECORDS']=str(capture)
    report=dict(component='Live native glow inputs versus independently executed original consumers',
        passed=False,native_command=command,native_exit_code=None,original_exit_code=None,
        runner_scratch=str(scratch),raw_capture=str(capture),
        raw_capture_cleanup_owned_by_runner=True,full_weapon_parity=False,
        framebuffer_parity_verified=False,large_artifacts_retained=[],tool_sha256=sha(Path(__file__)))
    def save():args.report.write_text(json.dumps(report,indent=2)+'\n')
    save()
    native=subprocess.run(command,env=environment);report['native_exit_code']=native.returncode;save()
    original_exit=1
    if capture.exists() and capture.stat().st_size>4:
        raw=capture.read_bytes();assert raw[:4]==b'LVG2'
        assert len(raw)<=700*1024,'Live glow input exceeded bounded fixture retention'
        args.inputs_report.write_text(json.dumps(dict(format='LVG2',bytes=len(raw),sha256=sha(capture),
            data_base64=base64.b64encode(raw).decode('ascii'),
            retention_reason='Required exact live GPU inputs for independent original sprite/projector replay',
            executable_content=False,framebuffer_capture=False),indent=2)+'\n')
        oracle=Path(__file__).with_name('oracle_live_projectile_glow_tu15.py')
        original_command=[sys.executable,str(oracle),'--capture',str(capture),'--output',str(args.original_report)]
        report.update(original_command=original_command,original_tool_sha256=sha(oracle),
            capture_bytes=len(raw),capture_sha256=sha(capture),inputs_report_sha256=sha(args.inputs_report));save()
        try:original_exit=subprocess.run(original_command,timeout=300).returncode
        except subprocess.TimeoutExpired:report['original_timed_out']=True;original_exit=124
        report['original_exit_code']=original_exit
        if args.original_report.exists():
            result=json.loads(args.original_report.read_text())
            report.update(passed=original_exit==0 and result['passed'],
                original_report_sha256=sha(args.original_report),actual_gpu_records=result['actual_gpu_records'],
                compared_versions=[dict(label=row['label'],vertices=row['compared_vertices'],
                    bit_identical=row['bit_identical'])for row in result['versions']])
    else:report['capture_missing_or_empty']=True
    report['native_failure_preserved']=native.returncode!=0
    save()
    print(json.dumps({key:report[key]for key in ('passed','native_exit_code','original_exit_code',
        'native_failure_preserved','framebuffer_parity_verified')}),flush=True)
    code=native.returncode or original_exit
    return code if code>=0 else 128-code


if __name__=='__main__':raise SystemExit(main())
