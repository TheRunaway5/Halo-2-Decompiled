#!/usr/bin/env python3
"""Qualify the native complete broadphase addObject parent and service closure."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
from specialize_native_operands import BROADPHASE_ADDITION_PIN,digest,leaf_body,specialize,CALL,CASE,MAPPING
from validate_native_array_abi_tu15 import SERVICES
from validate_native_collision_matrix_leaves_tu15 import qualify_hardware


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py to own compiler outputs')
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    source=root/'src/physics/runtime/physics_world_runtime_generated.cpp';canonical=source.read_text();specialized,receipt=specialize(canonical)
    assert receipt['native_broadphase_addition_entries']==['2ec4b0']
    assert digest(leaf_body(canonical,'2e8a00')[0].replace('ObservedWorldState &s','STATE &s'))==BROADPHASE_ADDITION_PIN
    mapping={int(a,16):int(b,16) for a,b in CASE.findall(MAPPING.search(canonical)['cases'])}
    helpers=[re.search(r'std::uint64_t '+name+r'\(.*?\n\}',canonical,re.S)[0] for name in ('r64','packed_words')]
    names=('2df360','2df710','2df780','2e6240','2e7500','2e7da0','2e8090','2e8a00');bodies=helpers
    for namespace,text in [('reference',canonical),('production',specialized)]:
        selected=[]
        for name in names:
            body=leaf_body(text,name)[0]
            body=CALL.sub(lambda m:f'0x{mapping[int(m[1],16)]:x}u',body)
            body=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',body)
            if name=='2e8a00' and namespace=='reference':
                # Preserve the recovered SSE destination for dual-NaN payloads
                # and the actual MOVAPS alignment in this compiled oracle.
                body=body.replace('_mm_add_ps(','halo2::native_tu15_leaves::ordered_add_ps(')
                body=body.replace('_mm_mul_ps(','halo2::native_tu15_leaves::ordered_mul_ps(')
                body=body.replace('vw(s.memory,','halo2::native_tu15_leaves::aligned_vw(s.memory,')
                body=body.replace('v(s.memory,','halo2::native_tu15_leaves::aligned_v(s.memory,')
            if namespace=='production':
                body,count=re.subn(r'(    if\(.*?return;\n)',r'\1    ++translated_fallbacks;\n',body,count=1,flags=re.S)
                assert count==1,name
            selected.append(body)
        declarations='\n'.join('static void run_'+name+'(ObservedWorldState &);' for name in names)
        dispatcher='void dispatch(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,bool){declared_service(s,target,site);}'
        bodies.append('namespace '+namespace+' {\n'+declarations+'\n'+dispatcher+'\n'+'\n'.join(selected)+'\n}')
    (scratch/'broadphase_add_object_bodies.inc').write_text('\n'.join(bodies))
    binary=scratch/'broadphase_add_object'
    command=['c++','-std=c++20','-O2','-ffp-contract=off','-fno-fast-math','-frounding-math',
        '-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(scratch),str(root/'tests/native_tu15_broadphase_add_object_tests.cpp'),
        str(root/'src/physics/native_tu15_array_growth.cpp'),str(root/'src/physics/native_tu15_array_reserve.cpp'),
        str(root/'src/physics/native_tu15_island_partition_temporary.cpp'),'-o',str(binary)]
    subprocess.run(command,check=True);modes={}
    for mode in ('native','reference'):
        env=dict(os.environ)
        for key in ('HALO2_REFERENCE_BROADPHASE_ADD','HALO2_REFERENCE_ARRAY_OWNERS','HALO2_REFERENCE_MATRIX_LEAVES',
            'HALO2_REFERENCE_PARTITION_TEMPORARIES'):env.pop(key,None)
        if mode=='reference':env['HALO2_REFERENCE_BROADPHASE_ADD']='1';env['HALO2_REFERENCE_MATRIX_LEAVES']='1'
        result=subprocess.run([str(binary)]+([str(scratch)] if mode=='native' else []),env=env,text=True,capture_output=True)
        if result.returncode:sys.stderr.write(result.stderr);result.check_returncode()
        modes[mode]=json.loads(result.stdout)
    hardware=qualify_hardware(scratch,(0x2ec4b0,),((0x48c938,struct.pack('<I',0x1001000)),),
        initial_x87_status=0x200,fs_base=0x100ff00,extra_assembly=SERVICES)
    paths=[source,root/'src/physics/runtime/native_tu15_broadphase_add_object.hpp',root/'src/physics/runtime/native_tu15_broadphase_add.hpp',
        root/'src/physics/runtime/native_tu15_broadphase_leaves.hpp',root/'src/physics/runtime/native_tu15_array_abi.hpp',
        root/'src/physics/runtime/native_tu15_partition_temporary_abi.hpp',root/'tools/specialize_native_operands.py',
        root/'tests/native_tu15_broadphase_add_object_tests.cpp',Path(__file__).resolve()]
    report=dict(passed=True,modes=modes,hardware_reference=hardware,
        source_sha256={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
        compiled_reference_pins_actual_sse_destination_order_and_movaps_alignment=True,
        production_allocator_abi_closure=False,exceptional_partial_abi_qualification=False,live_frame_qualification=False,
        declared_allocator_cases='Independent caller-owned allocator/release backing; original .text unchanged',
        actual_allocator_cases='Exact actual-owner allocate/release call-boundary prefixes; no actual owner outcomes supplied',
        comparisons=['bank bytes','ordered reads/stores','all GP/SSE registers','flags/DF','MXCSR','x87 depth/TOP/exception/C1','call ABI','cursor'],
        scratch_directory=str(scratch),retained_large_artifacts=[])
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
