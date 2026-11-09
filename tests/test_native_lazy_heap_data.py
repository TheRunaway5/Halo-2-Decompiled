#!/usr/bin/env python3
"""Pinned canonical lazy-controller comparison using independently backed test owners."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import re
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from specialize_native_operands import digest,leaf_body
BODY_PIN='f01d35461465d3460d347dfcb6a5cc3c3af7a438957fcbecf99856763183192a'


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=ROOT/'src/physics/physics_live_support_tu15_generated.cpp'
    canonical=leaf_body(source.read_text(),'149930');body,state_type=canonical[0],canonical[1]
    if digest(body.replace(state_type+' &s','STATE &s'))!=BODY_PIN:raise ValueError('Complete lazy DATA reference body drift')
    if 'selected('in body:raise ValueError('Unexpected selected operands require resolver authentication')
    files=(Path(__file__),source,ROOT/'tests/native_tu15_lazy_heap_data_tests.cpp',ROOT/'include/halo2/native_tu15_lazy_heap.hpp',
        ROOT/'src/physics/native_tu15_lazy_heap.cpp',ROOT/'src/physics/runtime/native_tu15_lazy_heap_abi.hpp',ROOT/'src/platform/native_tu15_tick_conversion.hpp',
        ROOT/'src/platform/native_tu15_millisecond_clock.hpp',ROOT/'include/halo2/native_tu15_millisecond_clock_kernel.hpp')
    pins={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest()for path in files}
    (scratch/'lazy_heap_data_reference.inc').write_text(body+'\n');binary=scratch/'lazy-heap-data'
    command=['c++','-std=c++20','-O1','-g','-fno-fast-math','-ffp-contract=off','-frounding-math',
        '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_lazy_heap_data_tests.cpp'),
        str(ROOT/'src/physics/native_tu15_lazy_heap.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    result=dict(passed=False,source_body_sha256=BODY_PIN,physical_original_compared=False,production_child_closure_qualified=False,
        test_services='Independently backed arena/reservation ledger, deterministic monotonic modular counters, context/yield/notification/trim state; no result lists.',
        timestamp_values_are_test_inputs=True,clock595_service_preserved=True,retained_large_artifacts=[],scratch_directory=str(scratch),
        compared='Complete pinned canonical parent versus shared native DATA plus retained ABI, using declared test-owned service algorithms; complete banks, ordered reads/stores, return and child-entry ABI/FP/cursor.',
        read_fault_partial_state_qualified=False,production_parent_bound=False,read_pass_observer_feedback_enabled=False)
    try:
        compiled=subprocess.run(command,text=True,capture_output=True)
        if compiled.returncode:raise RuntimeError(compiled.stderr[-4000:])
        environment=dict(os.environ)
        for key in ('HALO2_REFERENCE_LAZY_HEAP','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_REFERENCE_PARTITION_TEMPORARIES','HALO2_REFERENCE_TICK_CONVERSION','HALO2_REFERENCE_MILLISECOND_CLOCK'):environment.pop(key,None)
        run=subprocess.run([str(binary)],text=True,capture_output=True,env=environment)
        result.update(passed=run.returncode==0,stdout=run.stdout,errors=run.stderr)
        if run.stdout:
            result['comparison']=json.loads(run.stdout)
            sites={int(value,16)for value in re.findall(r's\.services\.instruction\((0x[0-9a-f]+)u\)',body)}
            executed=set(result['comparison']['executed_parent_sites']);result['parent_body_coverage']=dict(total=len(sites),executed=len(sites&executed),unexecuted=sorted(sites-executed))
            if run.returncode==0 and sites-executed:raise ValueError('Lazy DATA seam lost a complete parent source-site coverage family')
        print(json.dumps({key:result.get('comparison',{}).get(key)for key in ('passed','transactions','fixtures','residual_boundaries','child_entry_packets','ordered_reads','ordered_stores')}));print(run.stderr,end='',file=sys.stderr)
    except BaseException as error:
        result['errors']=repr(error);raise
    finally:
        result['source_sha256']=pins
        result['source_drift_during_run']=[name for name,value in pins.items()if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=value]
        if result['source_drift_during_run']:result['passed']=False
        result['sanitizers']=args.sanitizers;result['commands']=[command,[str(binary)]]
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(result,indent=2)+'\n')
    return 0 if result['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
