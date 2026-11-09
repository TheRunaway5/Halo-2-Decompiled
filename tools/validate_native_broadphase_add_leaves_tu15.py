#!/usr/bin/env python3
"""Qualify native sweep insertion and packed-index increment against TU1.5."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
from specialize_native_operands import BROADPHASE_INSERT_PIN,LEAVES,digest,leaf_body,specialize,CALL,CASE,MAPPING
from validate_native_array_abi_tu15 import SERVICES
from validate_native_collision_matrix_leaves_tu15 import qualify_hardware


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py to own compiler outputs')
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    source=root/'src/physics/runtime/physics_world_runtime_generated.cpp';canonical=source.read_text();specialized,receipt=specialize(canonical)
    assert '2e6240' in receipt['native_leaves'] and receipt['native_broadphase_insertion_entries']==['2eafb0']
    assert digest(leaf_body(canonical,'2e6240')[0].replace('ObservedWorldState &s','STATE &s'))==LEAVES['2e6240'][0]
    assert digest(leaf_body(canonical,'2e7500')[0].replace('ObservedWorldState &s','STATE &s'))==BROADPHASE_INSERT_PIN
    mapping={int(a,16):int(b,16) for a,b in CASE.findall(MAPPING.search(canonical)['cases'])}
    helpers=[re.search(r'std::uint64_t '+name+r'\(.*?\n\}',canonical,re.S)[0] for name in ('r64','packed_words')]
    names=('2df360','2df710','2e6240','2e7500');bodies=helpers
    for namespace,text in [('reference',canonical),('production',specialized)]:
        selected=[]
        for name in names:
            body=leaf_body(text,name)[0]
            body=CALL.sub(lambda m:f'0x{mapping[int(m[1],16)]:x}u',body)
            body=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',body)
            if namespace=='production':
                body,count=re.subn(r'(    if\(.*?return;\n)',r'\1    ++translated_fallbacks;\n',body,count=1,flags=re.S)
                assert count==1,name
            selected.append(body)
        declarations='\n'.join('static void run_'+name+'(ObservedWorldState &);' for name in names)
        dispatcher='void dispatch(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,bool){declared_service(s,target,site);}'
        bodies.append('namespace '+namespace+' {\n'+declarations+'\n'+dispatcher+'\n'+'\n'.join(selected)+'\n}')
    (scratch/'broadphase_add_leaf_bodies.inc').write_text('\n'.join(bodies))
    binary=scratch/'broadphase_add_leaves'
    command=['c++','-std=c++20','-O2','-ffp-contract=off','-fno-fast-math','-frounding-math',
        '-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(scratch),str(root/'tests/native_tu15_broadphase_add_leaves_tests.cpp'),
        str(root/'src/physics/native_tu15_array_growth.cpp'),str(root/'src/physics/native_tu15_array_reserve.cpp'),'-o',str(binary)]
    subprocess.run(command,check=True);modes={}
    for mode in ('native','reference'):
        env=dict(os.environ)
        for key in ('HALO2_REFERENCE_BROADPHASE_ADD','HALO2_REFERENCE_ARRAY_OWNERS','HALO2_REFERENCE_MATRIX_LEAVES',
            'HALO2_REFERENCE_PARTITION_TEMPORARIES'):env.pop(key,None)
        if mode=='reference':env['HALO2_REFERENCE_BROADPHASE_ADD']='1';env['HALO2_REFERENCE_MATRIX_LEAVES']='1'
        result=subprocess.run([str(binary)]+([str(scratch)] if mode=='native' else []),env=env,text=True,capture_output=True)
        if result.returncode:sys.stderr.write(result.stderr);result.check_returncode()
        modes[mode]=json.loads(result.stdout)
    hardware=qualify_hardware(scratch,(0x2e9cf0,0x2eafb0),
        ((0x48c938,struct.pack('<I',0x1001000)),),initial_x87_status=0x200,extra_assembly=SERVICES)
    paths=[source,root/'src/physics/runtime/native_tu15_broadphase_add.hpp',root/'src/physics/runtime/native_tu15_broadphase_leaves.hpp',
        root/'src/physics/runtime/native_tu15_array_abi.hpp',root/'tools/specialize_native_operands.py',
        root/'tests/native_tu15_broadphase_add_leaves_tests.cpp',Path(__file__).resolve()]
    report=dict(passed=True,modes=modes,hardware_reference=hardware,
        source_sha256={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
        production_allocator_abi_closure=False,exceptional_partial_abi_qualification=False,live_frame_qualification=False,
        declared_allocator_cases='Independent caller-owned allocator/release backing; original .text unchanged',
        actual_allocator_cases='Exact actual-owner allocate/release call-boundary prefixes; no actual owner outcomes supplied',
        comparisons=['bank bytes','ordered reads/stores','all GP/SSE registers','flags/DF','MXCSR','x87 depth/TOP/exception/C1','call ABI','cursor'],
        scratch_directory=str(scratch),retained_large_artifacts=[])
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
