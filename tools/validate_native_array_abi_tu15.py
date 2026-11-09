#!/usr/bin/env python3
"""Qualify retained array ABI adapters with declared native allocator contracts."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
from specialize_native_operands import ARRAY_ABI_ENTRIES,digest,leaf_body,specialize,CALL,CASE,MAPPING
from validate_native_collision_matrix_leaves_tu15 import qualify_hardware

SERVICES=r'''
.code32
.section .bridge,"awx"
.org 0x20000
declared_allocate:
 mov 4(%esp),%edx
 mov 8(%ecx),%eax
 add %edx,8(%ecx)
 add %edx,36(%ecx)
 incl 28(%ecx)
 ret $8
.org 0x20100
declared_release:
 mov 4(%esp),%eax
 mov 8(%esp),%edx
 sub %edx,36(%ecx)
 incl 32(%ecx)
 ret $12
'''


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py to own compiler outputs')
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH']);cohort=[]
    for number,path in enumerate(('src/physics/runtime/physics_world_runtime_generated.cpp',
        'src/physics/runtime/physics_contact_runtime_generated.cpp',
        'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp')):
        directory=scratch/f'copy-{number}';directory.mkdir()
        source=root/path;canonical=source.read_text();specialized,receipt=specialize(canonical)
        mapping={int(a,16):int(b,16) for a,b in CASE.findall(MAPPING.search(canonical)['cases'])}
        names=('2df360','2df710','2df780') if number<2 else ('2e2e30','2e3250')
        bodies=[]
        for namespace,text in (('reference',canonical),('production',specialized)):
            selected=[]
            for name in names:
                original=leaf_body(canonical,name)
                assert digest(original[0].replace(original[1]+' &s','STATE &s')) in ARRAY_ABI_ENTRIES[name][0]
                assert ARRAY_ABI_ENTRIES[name][2] in receipt['native_array_abi_entries']
                body=leaf_body(text,name)[0]
                body=CALL.sub(lambda match:f'0x{mapping[int(match[1],16)]:x}u',body)
                body=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',body)
                body=body.replace(original[1]+' &s','ObservedWorldState &s')
                selected.append(body)
            bodies.append('namespace '+namespace+' {\n'+'\n'.join(selected)+'\n}')
        if number<2:
            body_names=('2df710','2df780','2df360')
        else:body_names=('2e3250','2e3250','2e2e30')
        for namespace in ('reference','production'):
            body='switch(entry){'+''.join(f'case {i}: {namespace}::run_{name}(s);return;' for i,name in enumerate(body_names))+'}'
            bodies.append(f'void {namespace}_entry(ObservedWorldState &s,unsigned entry){{{body}}}')
        (directory/'array_abi_bodies.inc').write_text('\n'.join(bodies))
        binary=directory/'array_abi'
        command=['c++','-std=c++20','-O2','-ffp-contract=off','-fno-fast-math','-frounding-math',
            '-DARRAY_ABI_ENTRY_COUNT=3','-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(directory),
            str(root/'tests/native_tu15_array_abi_tests.cpp'),str(root/'src/physics/native_tu15_array_growth.cpp'),
            str(root/'src/physics/native_tu15_array_reserve.cpp'),'-o',str(binary)]
        subprocess.run(command,check=True);modes={}
        for mode in ('native','HALO2_REFERENCE_ARRAY_OWNERS'):
            environment=dict(os.environ)
            for variable in ('HALO2_REFERENCE_ARRAY_OWNERS','HALO2_REFERENCE_PARTITION_TEMPORARIES','HALO2_REFERENCE_MATRIX_LEAVES'):environment.pop(variable,None)
            if mode!='native':environment[mode]='1'
            result=subprocess.run([str(binary)]+([str(directory)] if mode=='native' and number==0 else []),
                env=environment,text=True,capture_output=True)
            if result.returncode:sys.stderr.write(result.stderr);result.check_returncode()
            modes[mode]=json.loads(result.stdout)
        cohort.append(dict(source=path,source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),modes=modes))
    hardware=qualify_hardware(scratch/'copy-0',(0x2e31e0,0x2e3250,0x2e2e30),
        ((0x48c938,struct.pack('<I',0x1001000)),),initial_x87_status=0x200,extra_assembly=SERVICES)
    hardware['declared_native_services']=['caller-owned backing cursor allocation','caller-owned active-byte release accounting']
    report=dict(passed=True,cohort=cohort,hardware_reference=hardware,
        allocator_results_supplied=False,production_allocator_abi_closure=False,
        actual_owner_cases='Exact call-boundary prefixes only; no actual owner outcome supplied',
        declared_allocator_cases='Full normal-return composition with independently executed native backing allocation/release',
        comparisons=['all memory bytes','ordered reads and stores','all GP/SSE registers','flags/DF','call ABI',
            'FP depth/TOP/controls/status','x87 exceptions/C1'],exceptional_partial_abi_qualification=False,
        live_frame_qualification=False,scratch_directory=str(scratch),retained_large_artifacts=[])
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
