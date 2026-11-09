#!/usr/bin/env python3
"""Private typed async comparator probe using the existing original-CPU oracle.

Only the candidate include/call and explicit literal observer witness change
in the scratch fixture. Original instructions and canonical controls do not.
The complete queue/default installation remains a separate later gate.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import runpy
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',default='c++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--insertion',action='store_true');args=parser.parse_args()
    kind='insert'if args.insertion else'order'
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);original=ROOT/f'tests/native_tu15_async_{kind}_abi_tests.cpp';text=original.read_text()
    include='#include "platform/native_tu15_async_order.hpp"'
    if text.count(include)!=1 or (not args.insertion and text.count('if(cursor_mode){')!=1):raise ValueError('Original async fixture structure changed')
    text=text.replace(include,'#include "platform/native_tu15_async_queue_abi.hpp"').replace('native_async_order_detail::','native_async_queue_abi_detail::')
    if not args.insertion:text=text.replace('if(cursor_mode){','if(true){')
    candidate=scratch/'typed-order.cpp';candidate.write_text(text);original_run=subprocess.run
    def run(command,*positional,**keywords):
        if isinstance(command,list)and str(original)in command:
            command=[str(candidate)if part==str(original)else part for part in command];command[0]=args.compiler
        return original_run(command,*positional,**keywords)
    subprocess.run=run
    report=args.report.resolve();report.parent.mkdir(parents=True,exist_ok=True)
    sys.argv=[str(ROOT/f'tools/validate_native_async_{kind}_tu15.py'),'--xbe',str(args.xbe),'--report',str(report)]
    if args.sanitizers:sys.argv.append('--sanitizers')
    files=('include/halo2/native_tu15_async_queue_kernel.hpp','src/platform/native_tu15_async_queue_abi.hpp',
        'tools/validate_native_tu15_async_queue_data.py',f'tools/validate_native_async_{kind}_tu15.py',f'tests/native_tu15_async_{kind}_abi_tests.cpp')
    pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in files}
    try:
        try:runpy.run_path(sys.argv[0],run_name='__main__')
        except SystemExit as stop:
            if stop.code:raise
        data=json.loads(report.read_text());data.update(candidate='Private typed async DATA/ABI; current public/default wrappers unchanged.',
            candidate_source_sha256=pins,generated_fixture_sha256=hashlib.sha256(text.encode()).hexdigest(),
            fixture_transformation='Only candidate include/compare namespace replaced; literal cursor witness supplied in every route. Original instructions and reference function bodies unchanged.',
            production_default_changed=False,typed_insertion_normal_abi_compared=args.insertion,scratch_owner=str(scratch))
        if any(hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=value for name,value in pins.items()):raise ValueError('Typed comparator source drift')
        report.write_text(json.dumps(data,indent=2)+'\n')
    except BaseException as error:
        report.write_text(json.dumps(dict(passed=False,error=repr(error),candidate_source_sha256=pins,scratch_owner=str(scratch)),indent=2)+'\n');raise
    finally:subprocess.run=original_run
    return 0
if __name__=='__main__':raise SystemExit(main())
