#!/usr/bin/env python3
"""Qualify native bounds decoding with translated and unchanged-i386 original references."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from specialize_native_operands import LEAVES,digest,leaf_body,specialize
from validate_native_collision_matrix_leaves_tu15 import qualify_hardware


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py to own compiler outputs')
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    source=root/'src/physics/physics_live_support_tu15_generated.cpp';canonical=source.read_text()
    specialized,receipt=specialize(canonical);bodies=[]
    for namespace,text in (('reference',canonical),('production',specialized)):
        selected=[]
        for name in ('2ea050','2eb0c0'):
            original=leaf_body(canonical,name)
            assert digest(original[0].replace(original[1]+' &s','STATE &s'))==LEAVES[name][0]
            assert name in receipt['native_leaves']
            body=leaf_body(text,name)[0]
            if namespace=='reference' and name=='2ea050':body=body.replace('u(spill(s.fp.st(0)))',
                'halo2::native_tu15_leaves::portal_spill_word(s.fp.st(0),s.fp.status)')
            selected.append(body)
        bodies.append('namespace '+namespace+' {\n'+'\n'.join(selected)+'\n}')
    (scratch/'broadphase_bounds_leaf_bodies.inc').write_text('\n'.join(bodies))
    binary=scratch/'broadphase_bounds_leaves'
    command=['c++','-std=c++20','-O2','-ffp-contract=off','-fno-fast-math','-frounding-math',
        '-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(scratch),
        str(root/'tests/native_tu15_broadphase_bounds_leaves_tests.cpp'),'-o',str(binary)]
    subprocess.run(command,check=True);modes={}
    for mode in ('native','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_VERIFY_MATRIX_LEAVES'):
        environment=dict(os.environ)
        environment.pop('HALO2_REFERENCE_MATRIX_LEAVES',None);environment.pop('HALO2_VERIFY_MATRIX_LEAVES',None)
        if mode!='native':environment[mode]='1'
        result=subprocess.run([str(binary)]+([str(scratch)] if mode=='native' else []),
            env=environment,text=True,capture_output=True)
        if result.returncode:sys.stderr.write(result.stderr);result.check_returncode()
        modes[mode]=json.loads(result.stdout)
    hardware=qualify_hardware(scratch,(0x2ea050,0x2eb0c0),initial_x87_status=0x200)
    report=dict(passed=True,modes=modes,hardware_reference=hardware,command=command,
        source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        native_header_sha256=hashlib.sha256((root/'src/physics/runtime/native_tu15_broadphase_bounds_leaves.hpp').read_bytes()).hexdigest(),
        input_coverage=['IEEE scales and offsets','all x87 precision and rounding controls','SSE rounding and DAZ/FTZ',
            '16 stack alignments','output/node/broadphase/edge/stack aliases','incoming x87 depth and TOP'],
        comparisons=['all memory bytes','ordered reads and stores','all GP/SSE registers','all arithmetic flags',
            'MXCSR','x87 depth/TOP','x87 exception and C1 status'],live_frame_qualification=False,
        exceptional_partial_abi_qualification=False,scratch_directory=str(scratch),retained_large_artifacts=[])
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
