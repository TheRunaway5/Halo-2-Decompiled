#!/usr/bin/env python3
"""Qualify the shared typed temporary owner and its retained caller ABI bridge."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
from specialize_native_operands import TEMPORARY_ABI_ENTRIES,digest,leaf_body,specialize,CALL,CASE,MAPPING
from validate_native_collision_matrix_leaves_tu15 import qualify_hardware


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py to own compiler outputs')
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    source=root/'src/physics/runtime/physics_world_runtime_generated.cpp';canonical=source.read_text()
    specialized,receipt=specialize(canonical);bodies=[]
    mapping={int(a,16):int(b,16) for a,b in CASE.findall(MAPPING.search(canonical)['cases'])}
    for namespace,text in (('reference',canonical),('production',specialized)):
        selected=[]
        for name in TEMPORARY_ABI_ENTRIES:
            original=leaf_body(canonical,name)
            assert digest(original[0].replace(original[1]+' &s','STATE &s'))==TEMPORARY_ABI_ENTRIES[name][0]
            assert TEMPORARY_ABI_ENTRIES[name][2] in receipt['native_temporary_abi_entries']
            body=leaf_body(text,name)[0]
            body=CALL.sub(lambda match:f'0x{mapping[int(match[1],16)]:x}u',body)
            body=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',body)
            selected.append(body)
        bodies.append('namespace '+namespace+' {\n'+'\n'.join(selected)+'\n}')
    (scratch/'partition_temporary_abi_bodies.inc').write_text('\n'.join(bodies))
    binary=scratch/'partition_temporary_abi'
    command=['c++','-std=c++20','-O2','-ffp-contract=off','-fno-fast-math','-frounding-math',
        '-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(scratch),
        str(root/'tests/native_tu15_partition_temporary_abi_tests.cpp'),
        str(root/'src/physics/native_tu15_island_partition_temporary.cpp'),'-o',str(binary)]
    subprocess.run(command,check=True);modes={}
    for mode in ('native','HALO2_REFERENCE_PARTITION_TEMPORARIES'):
        environment=dict(os.environ)
        environment.pop('HALO2_REFERENCE_PARTITION_TEMPORARIES',None);environment.pop('HALO2_REFERENCE_MATRIX_LEAVES',None)
        if mode!='native':environment[mode]='1'
        result=subprocess.run([str(binary)]+([str(scratch)] if mode=='native' else []),
            env=environment,text=True,capture_output=True)
        if result.returncode:sys.stderr.write(result.stderr);result.check_returncode()
        modes[mode]=json.loads(result.stdout)
    hardware=qualify_hardware(scratch,(0x2eba20,0x2eb850),
        ((0x48c938,struct.pack('<I',0x1001000)),),initial_x87_status=0x200,fs_base=0x100f000)
    report=dict(passed=True,modes=modes,hardware_reference=hardware,command=command,
        source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        input_coverage=['zero/wrapping/count inputs','16 stack alignments','delayed count/header aliases',
            'fast arena allocation and release','exact slow arena and owned-release call boundaries',
            'incoming x87 depth/TOP and rounding controls'],
        comparisons=['all memory bytes','ordered reads and stores','all GP/SSE registers','all arithmetic flags',
            'full residual call ABI','MXCSR','x87 depth/TOP/exception/C1 status'],
        live_frame_qualification=False,exceptional_partial_abi_qualification=False,
        residual_allocation_implementations_native=False,scratch_directory=str(scratch),retained_large_artifacts=[])
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
