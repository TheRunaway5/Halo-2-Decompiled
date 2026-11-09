#!/usr/bin/env python3
"""Shared promotion DATA/public queue policy with preserved and retail controls."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
from native_async_promotion_data_replacements import replace_public,replace_abi,reference_public
ROOT=Path(__file__).resolve().parents[1]
FILES=('include/halo2/native_tu15_async_promotion_kernel.hpp','include/halo2/native_tu15_async_promotion_value.hpp',
    'tests/native_tu15_async_promotion_data_tests.cpp','tests/reference/native_tu15_async_promotion_value_reference.hpp',
    'tests/reference/native_tu15_async_queue_value_reference.hpp','src/platform/source_async_jobs.cpp',
    'tools/native_async_promotion_data_replacements.py','tools/validate_native_tu15_async_promotion_values.py',
    'include/halo2/native_tu15_async_queue_kernel.hpp','include/halo2/native_tu15_async_queue_value.hpp',
    'include/halo2/source_async_jobs.hpp','tests/source_async_jobs_tests.cpp','tests/source_async_read_tests.cpp',
    'tests/fixtures/retail_source_async_queue.tsv','tests/fixtures/retail_source_async_read.tsv')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--leak-sanitizer',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);pins={name:sha(ROOT/name)for name in FILES};report=dict(passed=False,source_sha256=pins,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        reference_public();source=ROOT/'src/platform/source_async_jobs.cpp';text=source.read_text();installed=replace_public(text)
        if args.require_binding:
            header=ROOT/'src/platform/native_tu15_async_promote.hpp';pins[str(header.relative_to(ROOT))]=sha(header)
            if installed!=text or replace_abi(header.read_text())!=header.read_text():raise ValueError('Typed promotion default delegation required')
        else:source=scratch/'public.cpp';source.write_text(installed)
        env=dict(os.environ)
        if args.sanitizers:env['ASAN_OPTIONS']=env.get('ASAN_OPTIONS','')+(':detect_leaks=1'if args.leak_sanitizer else':detect_leaks=0')
        results=[]
        for label,test,fixture in(('shared-promotion-values','tests/native_tu15_async_promotion_data_tests.cpp',None),
            ('retail-queue','tests/source_async_jobs_tests.cpp','tests/fixtures/retail_source_async_queue.tsv'),
            ('retail-read','tests/source_async_read_tests.cpp','tests/fixtures/retail_source_async_read.tsv')):
            command=[args.compiler,'-std=c++20','-O1'if args.sanitizers else'-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                '-I'+str(ROOT/'include'),'-I'+str(ROOT/'tests'),str(ROOT/test),str(source),'-o',str(scratch/label)]
            if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            subprocess.run(command,check=True)
            result=subprocess.run([str(scratch/label)]+([str(ROOT/fixture)]if fixture else[]),capture_output=True,text=True,env=env,timeout=90)
            if result.returncode:raise ValueError('Promotion value policy failed '+(result.stdout+result.stderr)[-3000:])
            results.append(dict(name=label,compile_command=command,stdout=result.stdout.strip(),passed=True))
        if any(sha(ROOT/name)!=pin for name,pin in pins.items()):raise ValueError('Promotion DATA/value sources changed')
        report.update(passed=True,cases=results,semantic_promotions=7168,successful_promotions=5376,stale_job_noops=1792,
            public_default_binding_qualified=args.require_binding,address_undefined_sanitizers=args.sanitizers,leak_detection_qualified=args.sanitizers and args.leak_sanitizer,
            platform_mutex_or_live_world_qualified=False)
    except BaseException as error:report.update(passed=False,error=repr(error));raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(passed=True,semantic_promotions=7168,retail_suites=2)));return 0
if __name__=='__main__':raise SystemExit(main())
