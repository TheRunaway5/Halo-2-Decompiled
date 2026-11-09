#!/usr/bin/env python3
"""Qualify packed-edge removal kernels with translated and unchanged i386 references."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from specialize_native_operands import LEAVES, digest, leaf_body, specialize
from validate_native_collision_matrix_leaves_tu15 import qualify_hardware


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py to own compiler outputs')
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    source=root/'src/physics/physics_live_support_tu15_generated.cpp';canonical=source.read_text()
    specialized,receipt=specialize(canonical)
    helpers=[]
    for name in ('native_read64','native_mmx_words'):
        helpers.append(re.search(r'std::uint64_t '+name+r'\(.*?\n\}',canonical,re.S)[0])
    bodies=[]
    for namespace,text in (('reference',canonical),('production',specialized)):
        selected=[]
        for name in ('2e9df0','2e9fd0'):
            original=leaf_body(canonical,name)
            assert digest(original[0].replace(original[1]+' &s','STATE &s'))==LEAVES[name][0]
            assert name in receipt['native_leaves']
            selected.append(leaf_body(text,name)[0])
        bodies.append('namespace '+namespace+' {\n'+'\n'.join(selected)+'\n}')
    (scratch/'broadphase_leaf_bodies.inc').write_text('\n'.join(helpers+bodies))
    binary=scratch/'broadphase_leaves'
    command=['c++','-std=c++20','-O2','-ffp-contract=off','-fno-fast-math','-frounding-math',
        '-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(scratch),
        str(root/'tests/native_tu15_broadphase_leaves_tests.cpp'),'-o',str(binary)]
    subprocess.run(command,check=True)
    modes={}
    for mode in ('native','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_VERIFY_MATRIX_LEAVES'):
        environment=dict(os.environ)
        environment.pop('HALO2_REFERENCE_MATRIX_LEAVES',None);environment.pop('HALO2_VERIFY_MATRIX_LEAVES',None)
        if mode!='native':environment[mode]='1'
        result=subprocess.run([str(binary)]+([str(scratch)] if mode=='native' else []),
            env=environment,text=True,capture_output=True)
        if result.returncode:sys.stderr.write(result.stderr);result.check_returncode()
        modes[mode]=json.loads(result.stdout)
    hardware=qualify_hardware(scratch,(0x2e9df0,0x2e9fd0))
    report=dict(passed=True,modes=modes,hardware_reference=hardware,command=command,
        source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        native_header_sha256=hashlib.sha256((root/'src/physics/runtime/native_tu15_broadphase_leaves.hpp').read_bytes()).hexdigest(),
        input_coverage=['0..63 records','signed word boundaries','node/bounds aliases',
            '16 stack alignments','forward/backward direction flag','incoming x87 depth and TOP'],
        comparisons=['all memory bytes','ordered reads and stores','all GP/SSE registers',
            'all arithmetic flags','MXCSR','x87 logical depth/TOP','MMX lifetime'],live_frame_qualification=False,
        exceptional_partial_abi_qualification=False,scratch_directory=str(scratch),retained_large_artifacts=[])
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
