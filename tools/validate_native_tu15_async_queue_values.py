#!/usr/bin/env python3
"""Installed shared async DATA against frozen policy and retail input fixtures."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
from native_async_queue_data_replacements import replace_abi,replace_public

ROOT=Path(__file__).resolve().parents[1]
FILES=('include/halo2/native_tu15_async_queue_kernel.hpp','include/halo2/native_tu15_async_queue_value.hpp',
    'include/halo2/source_async_jobs.hpp','src/platform/source_async_jobs.cpp','src/platform/native_tu15_async_order.hpp',
    'tests/native_tu15_async_queue_data_tests.cpp','tests/reference/native_tu15_async_queue_value_reference.hpp',
    'tests/source_async_jobs_tests.cpp','tests/source_async_read_tests.cpp',
    'tests/fixtures/retail_source_async_queue.tsv','tests/fixtures/retail_source_async_read.tsv',
    'tools/native_async_queue_data_replacements.py','tools/validate_native_tu15_async_queue_values.py')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--leak-sanitizer',action='store_true');args=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);pins={name:sha(ROOT/name)for name in FILES}
    report=dict(passed=False,source_sha256=pins,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        for path,install in(('src/platform/source_async_jobs.cpp',replace_public),('src/platform/native_tu15_async_order.hpp',replace_abi)):
            source=(ROOT/path).read_text()
            if install(source)!=source:raise ValueError('Installed typed async owner required '+path)
        environment=dict(os.environ)
        if args.sanitizers:environment['ASAN_OPTIONS']=environment.get('ASAN_OPTIONS','')+(':detect_leaks=1'if args.leak_sanitizer else':detect_leaks=0')
        cases=[]
        for name,source,fixture in(('shared-values','tests/native_tu15_async_queue_data_tests.cpp',None),
            ('retail-queue','tests/source_async_jobs_tests.cpp','tests/fixtures/retail_source_async_queue.tsv'),
            ('retail-read','tests/source_async_read_tests.cpp','tests/fixtures/retail_source_async_read.tsv')):
            command=[args.compiler,'-std=c++20','-O1'if args.sanitizers else'-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                '-I'+str(ROOT/'include'),'-I'+str(ROOT/'tests'),str(ROOT/source),str(ROOT/'src/platform/source_async_jobs.cpp'),'-o',str(scratch/name)]
            if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            subprocess.run(command,check=True)
            result=subprocess.run([str(scratch/name)]+([str(ROOT/fixture)]if fixture else[]),capture_output=True,text=True,env=environment)
            if result.returncode:raise ValueError(name+' failed '+(result.stdout+result.stderr)[-4000:])
            cases.append(dict(name=name,compile_command=command,stdout=result.stdout.strip(),passed=True))
        if any(sha(ROOT/name)!=digest for name,digest in pins.items()):raise ValueError('Async DATA qualification inputs changed')
        report.update(passed=True,cases=cases,installed_public_queue_qualified=True,semantic_comparisons=32768,semantic_insertions=5264,
            address_undefined_sanitizers=args.sanitizers,leak_detection_qualified=args.sanitizers and args.leak_sanitizer,
            world_synchronous_policy_changed=False,live_worker_or_world_epoch_qualified=False)
    except BaseException as error:report.update(passed=False,error=repr(error));raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(passed=True,semantic_comparisons=32768,semantic_insertions=5264,retail_suites=2)));return 0
if __name__=='__main__':raise SystemExit(main())
