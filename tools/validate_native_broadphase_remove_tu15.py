#!/usr/bin/env python3
"""Qualify the native broadphase remove parent against its unchanged owners."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
from specialize_native_operands import BROADPHASE_REMOVE_PIN,digest,leaf_body,specialize,CALL,CASE,MAPPING
from validate_native_array_abi_tu15 import SERVICES
from validate_native_collision_matrix_leaves_tu15 import qualify_hardware


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path)
    parser.add_argument('--skip-hardware',action='store_true',help='Development comparison only; report remains unqualified')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py to own outputs')
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    sources=[root/'src/physics/physics_live_support_tu15_generated.cpp',root/'src/physics/runtime/physics_world_runtime_generated.cpp']
    canonical=[path.read_text() for path in sources];specialized=[specialize(text) for text in canonical]
    original=leaf_body(canonical[0],'2ec9b0')
    assert digest(original[0].replace(original[1]+' &s','STATE &s'))==BROADPHASE_REMOVE_PIN
    assert specialized[0][1]['native_broadphase_removal_entries']==['2ec9b0']
    names=[('2e9df0','2e9fd0','2ec9b0'),('2df360','2df710','2df780','2e7da0','2e8090')]
    helpers=[]
    for name in ('native_read64','native_mmx_words'):
        match=re.search(r'(?:std::uint64_t|static std::uint64_t) '+name+r'\(.*?\n\}',canonical[0],re.S)
        assert match,name
        helpers.append(match[0])
    bodies=['\n'.join(helpers)]
    for namespace,production in [('reference',False),('production',True)]:
        selected=[]
        for number,nameset in enumerate(names):
            mapping={int(a,16):int(b,16) for a,b in CASE.findall(MAPPING.search(canonical[number])['cases'])}
            for name in nameset:
                text=specialized[number][0] if production else canonical[number]
                body=leaf_body(text,name)[0]
                body=CALL.sub(lambda m:f'0x{mapping[int(m[1],16)]:x}u',body)
                body=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',body)
                body=body.replace(leaf_body(text,name)[1]+' &s','ObservedWorldState &s')
                if production:
                    body,count=re.subn(r'(    if\(halo2::.*?return;\n)',r'\1    ++translated_fallbacks;\n',body,count=1,flags=re.S)
                    assert count==1,name
                selected.append(body)
        declarations='\n'.join('static void run_'+name+'(ObservedWorldState &);' for group in names for name in group)
        dispatch='''
void dispatch(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,bool) {
    switch(target) {
    case 0x2eba20u:run_2e7da0(s);return;
    case 0x2eb850u:run_2e8090(s);return;
    case 0x2e31e0u:run_2df710(s);return;
    case 0x2e3250u:run_2df780(s);return;
    default:declared_service(s,target,site);return;
    }
}
'''
        bodies.append('namespace '+namespace+' {\n'+declarations+'\n'+dispatch+'\n'+'\n'.join(selected)+'\n}')
    (scratch/'broadphase_remove_bodies.inc').write_text('\n'.join(bodies))
    binary=scratch/'broadphase_remove'
    command=['c++','-std=c++20','-O2','-ffp-contract=off','-fno-fast-math','-frounding-math',
        '-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(scratch),str(root/'tests/native_tu15_broadphase_remove_tests.cpp'),
        str(root/'src/physics/native_tu15_array_growth.cpp'),str(root/'src/physics/native_tu15_array_reserve.cpp'),
        str(root/'src/physics/native_tu15_island_partition_temporary.cpp'),'-o',str(binary)]
    subprocess.run(command,check=True);modes={}
    for mode in ('native','HALO2_REFERENCE_BROADPHASE_REMOVE'):
        env=dict(os.environ)
        for key in ('HALO2_REFERENCE_BROADPHASE_REMOVE','HALO2_REFERENCE_ARRAY_OWNERS','HALO2_REFERENCE_MATRIX_LEAVES',
            'HALO2_REFERENCE_PARTITION_TEMPORARIES'):env.pop(key,None)
        if mode!='native':env[mode]='1'
        result=subprocess.run([str(binary)]+([str(scratch)] if mode=='native' else []),env=env,text=True,capture_output=True)
        if result.returncode:sys.stderr.write(result.stderr);result.check_returncode()
        modes[mode]=json.loads(result.stdout)
    hardware=None if args.skip_hardware else qualify_hardware(scratch,(0x2ec9b0,),
        ((0x48c938,struct.pack('<I',0x1001000)),),initial_x87_status=0x200,fs_base=0x100ff00,extra_assembly=SERVICES)
    report=dict(passed=not args.skip_hardware,modes=modes,hardware_reference=hardware,
        source_sha256={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},
        native_source_sha256={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (
            root/'src/physics/runtime/native_tu15_broadphase_remove.hpp',root/'tools/specialize_native_operands.py',
            root/'src/physics/runtime/native_tu15_array_abi.hpp',root/'src/physics/runtime/native_tu15_partition_temporary_abi.hpp',
            root/'src/physics/native_tu15_array_growth.cpp',root/'src/physics/native_tu15_array_reserve.cpp',
            root/'src/physics/native_tu15_island_partition_temporary.cpp',
            root/'tests/native_tu15_broadphase_remove_tests.cpp',Path(__file__).resolve())},
        production_allocator_abi_closure=False,exceptional_partial_abi_qualification=False,live_frame_qualification=False,
        declared_allocator_cases='Independent caller-owned backing allocator/release; original .text unchanged',
        actual_allocator_cases='Exact allocate/release call-boundary prefixes only; no actual owner outcome supplied',
        coverage=['2–192 nodes','16 caller alignments','both DF directions','partition-cell seed/removal/backlink repair',
            'native temporary lifetime','pair-array growth','node-array reserve','dense move and final-node removal',
            'active observation-only cursor with zero translated closure fallbacks'],
        comparisons=['full bank','ordered reads/stores','all GP/SSE registers','arithmetic/DF flags','x87 status/TOP','MXCSR','service ABI'],
        scratch_directory=str(scratch),retained_large_artifacts=[])
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
