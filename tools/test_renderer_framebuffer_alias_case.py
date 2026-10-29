#!/usr/bin/env python3
"""Run the actual GPU fixture, retain bounded JSON, dispose of all raw PPMs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time


def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--binary',type=Path,required=True)
    ap.add_argument('--shaders',type=Path,required=True)
    ap.add_argument('--report-output',type=Path,required=True)
    args=ap.parse_args()
    if 'HALO2_TEST_SCRATCH'not in os.environ:ap.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);output=scratch/'native-alias';output.mkdir()
    binary=args.binary.resolve(strict=True);command=[str(binary),str(output)]
    env=dict(os.environ);env['HALO2_NATIVE_GPU_SHADER_DIR']=str(args.shaders.resolve(strict=True))
    for key in ('XDG_CONFIG_HOME','XDG_DATA_HOME','XDG_CACHE_HOME'):env[key]=str(scratch/key.lower())
    if not env.get('DISPLAY'):
        if not shutil.which('xvfb-run'):ap.error('Display or xvfb-run required')
        command=['xvfb-run','-a',*command]
    log=scratch/'native-alias.log';report=dict(passed=False,scope=__doc__,command=command,
        binary_sha256=sha(binary),tool_sha256=sha(__file__),scratch=str(scratch),retained_large_artifacts=[])
    started=time.monotonic()
    try:
        with log.open('w')as stream:
            report['returncode']=subprocess.run(command,env=env,stdout=stream,stderr=subprocess.STDOUT,timeout=240).returncode
    except subprocess.TimeoutExpired:report.update(returncode=124,timeout=True)
    report['elapsed_seconds']=round(time.monotonic()-started,3)
    with log.open('rb')as stream:
        stream.seek(max(0,log.stat().st_size-8192));report['log_tail']=stream.read().decode(errors='replace')
    native=output/'report.json'
    if native.is_file()and native.stat().st_size<=1024*1024:
        report['native_report']=json.loads(native.read_text());report['native_report_sha256']=sha(native)
    report['raw_ppm_count']=len(list(output.glob('*.ppm')))
    report['raw_ppm_bytes']=sum(p.stat().st_size for p in output.glob('*.ppm'))
    report['passed']=report.get('returncode')==0 and'native_report'in report
    args.report_output.parent.mkdir(parents=True,exist_ok=True)
    args.report_output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],returncode=report.get('returncode'),report=str(args.report_output))))
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
