#!/usr/bin/env python3
"""Replay a retained small live-glow input fixture under tools/run_test.py."""
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
    parser.add_argument('--inputs',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--receipt',type=Path,required=True)
    args=parser.parse_args();fixture=json.loads(args.inputs.read_text())
    raw=base64.b64decode(fixture['data_base64'],validate=True)
    assert fixture['format']=='LVG2' and raw[:4]==b'LVG2'
    assert len(raw)==fixture['bytes']<=700*1024
    assert hashlib.sha256(raw).hexdigest()==fixture['sha256']
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);capture=scratch/'restored-live-glow.lvg'
    capture.write_bytes(raw)
    oracle=Path(__file__).with_name('oracle_live_projectile_glow_tu15.py')
    command=[sys.executable,str(oracle),'--capture',str(capture),'--output',str(args.output)]
    result=subprocess.run(command)
    receipt=dict(passed=result.returncode==0,command=command,exit_code=result.returncode,
        inputs_path=str(args.inputs),inputs_sha256=sha(args.inputs),decoded_bytes=len(raw),
        decoded_sha256=sha(capture),original_tool_sha256=sha(oracle),tool_sha256=sha(Path(__file__)),
        runner_scratch=str(scratch),raw_capture_cleanup_owned_by_runner=True,large_artifacts_retained=[])
    if args.output.exists():receipt['original_report_sha256']=sha(args.output)
    args.receipt.write_text(json.dumps(receipt,indent=2)+'\n')
    return result.returncode if result.returncode>=0 else 128-result.returncode


if __name__=='__main__':raise SystemExit(main())
