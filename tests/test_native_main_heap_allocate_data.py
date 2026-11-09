#!/usr/bin/env python3
"""Owned canonical-reference comparison for the shared heap allocation DATA core."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from generated_routine_ownership import function_bodies
PINS={'22e1d0':'131f98861fe122829a2bf5770b5a862dafacd14e8f66ade753c8051fef8fdf1f',
      '22e430':'410b9cfcfd7d00fde25b7208235594b4cc2a81fe473a127678cabd6725dd5e17'}


def main():
    if not os.environ.get('HALO2_TEST_SCRATCH'):raise RuntimeError('Run through tools/run_test.py')
    source=ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp'
    text=source.read_text()
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',text,re.S)
    if not resolver or hashlib.sha256(resolver[0].encode()).hexdigest()!='20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2':
        raise RuntimeError('Complete selected resolver changed before allocation reference extraction')
    mapping={int(a,16):int(b,16)for a,b in re.findall(r'case (0x[0-9a-f]+)u:return (0x[0-9a-f]+)u;',resolver[0])}
    bodies=[]
    for match,body in function_bodies(text):
        if match[1] not in ('22e1d0','22e430'):continue
        if hashlib.sha256(body.encode()).hexdigest()!=PINS[match[1]]:raise RuntimeError('Canonical allocation reference body drift: '+match[1])
        body=body.replace('ObservedWorldState &s','WorldState &s')
        body=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',body)
        body=re.sub(r'selected\((0x[0-9a-f]+)u,s\.image\)',lambda m:hex(mapping[int(m[1],16)])+'u',body)
        bodies.append(body)
    if len(bodies)!=2:raise RuntimeError('Both unchanged canonical heap bodies are required')
    with tempfile.TemporaryDirectory(dir=os.environ['HALO2_TEST_SCRATCH'])as directory:
        scratch=Path(directory);(scratch/'main_heap_allocate_reference.inc').write_text('\n'.join(bodies))
        binary=scratch/'heap-allocation-data'
        command=['c++','-std=c++20','-O1','-g','-fno-fast-math','-ffp-contract=off','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),
            str(ROOT/'tests/native_tu15_main_heap_allocate_tests.cpp'),str(ROOT/'src/physics/native_tu15_main_heap_allocate.cpp'),'-o',str(binary)]
        if os.environ.get('HALO2_HEAP_ALLOCATE_SANITIZERS'):command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        subprocess.run(command,check=True)
        result=subprocess.run([str(binary)],capture_output=True,text=True)
        print(result.stdout,end='');print(result.stderr,end='',file=sys.stderr)
        requested=os.environ.get('HALO2_MAIN_HEAP_ALLOCATION_DATA_REPORT')
        if requested:
            report=dict(schema='native-tu15-main-heap-allocation-data-v1',passed=result.returncode==0,
                source_body_sha256=PINS,selected_resolver_sha256=hashlib.sha256(resolver[0].encode()).hexdigest(),
                sanitizers=bool(os.environ.get('HALO2_HEAP_ALLOCATE_SANITIZERS')),output=result.stdout,errors=result.stderr,
                source_hashes={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()for p in
                    (Path(__file__),ROOT/'tests/native_tu15_main_heap_allocate_tests.cpp',
                     ROOT/'include/halo2/native_tu15_main_heap_allocate.hpp',ROOT/'src/physics/native_tu15_main_heap_allocate.cpp')},
                commands=[command,[str(binary)]],physical_original_compared=False,full_caller_abi_compared=False,
                compared='Full banks and ordered reads/stores against pinned canonical source, using a test caller transport; typed value callers compare heap DATA and allocation result.',
                retained_large_artifacts=[])
            output=Path(requested);output.parent.mkdir(parents=True,exist_ok=True);output.write_text(json.dumps(report,indent=2)+'\n')
        return result.returncode


if __name__=='__main__':raise SystemExit(main())
