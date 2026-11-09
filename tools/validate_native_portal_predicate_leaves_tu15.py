#!/usr/bin/env python3
"""Qualify the complete native portal/sphere predicate across all three source copies."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
from specialize_native_operands import LEAVES, digest, leaf_body, specialize, CALL
from validate_native_collision_matrix_leaves_tu15 import qualify_hardware


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py to own compiler outputs')
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    names=('123210','243450','14cea0');cohort=[]
    for number,path in enumerate(('src/physics/physics_live_support_tu15_generated.cpp',
        'src/rendering/shadows/source_shadow_frame_tu15_generated.cpp',
        'src/campaign/ai/source_ai_frame_tu15_generated.cpp')):
        directory=scratch/f'copy-{number}';directory.mkdir()
        source=root/path;canonical=source.read_text();specialized,receipt=specialize(canonical)
        bodies=[]
        for namespace,text in (('reference',canonical),('production',specialized)):
            selected=[]
            for name in names:
                original=leaf_body(canonical,name);pin=LEAVES[name][0]
                assert digest(original[0].replace(original[1]+' &s','STATE &s')) in (pin if isinstance(pin,tuple) else (pin,))
                assert name in receipt['native_leaves'],(path,name)
                body=leaf_body(text,name)[0]
                if namespace=='reference':
                    body=CALL.sub(lambda match:match[1]+'u',body)
                    body=body.replace('_mm_add_ss(', 'halo2::native_tu15_leaves::ordered_add_ss(')
                    body=body.replace('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(')
                    if name=='14cea0':body=body.replace('u(spill(s.fp.st(0))));s.fp.pop();',
                        'halo2::native_tu15_leaves::portal_spill_word(s.fp.st(0),s.fp.status));s.fp.pop();')
                selected.append(body)
            bodies.append('namespace '+namespace+' {\n'+'\n'.join(selected)+'\n}')
        helper=re.search(r'std::uint32_t native_single_spill\(.*?\n\}',canonical,re.S)
        (directory/'portal_predicate_leaf_bodies.inc').write_text((helper[0]+'\n' if helper else '')+'\n'.join(bodies))
        binary=directory/'portal_predicate_leaves'
        command=['c++','-std=c++20','-O2','-ffp-contract=off','-fno-fast-math','-frounding-math',
            '-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(directory),
            str(root/'tests/native_tu15_portal_predicate_leaves_tests.cpp'),'-o',str(binary)]
        subprocess.run(command,check=True)
        modes={}
        for mode in ('native','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_VERIFY_MATRIX_LEAVES'):
            environment=dict(os.environ)
            environment.pop('HALO2_REFERENCE_MATRIX_LEAVES',None);environment.pop('HALO2_VERIFY_MATRIX_LEAVES',None)
            if mode!='native':environment[mode]='1'
            result=subprocess.run([str(binary)]+([str(directory)] if mode=='native' and number==0 else []),
                env=environment,text=True,capture_output=True)
            if result.returncode:sys.stderr.write(result.stderr);result.check_returncode()
            modes[mode]=json.loads(result.stdout)
        cohort.append(dict(source=path,source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),modes=modes))
    hardware=qualify_hardware(scratch/'copy-0',(0x123210,0x243450,0x14cea0),
        ((0x4ecc0c,struct.pack('<I',0x1002300)),),initial_x87_status=0x200)
    report=dict(passed=True,cohort=cohort,hardware_reference=hardware,
        native_header_sha256=hashlib.sha256((root/'src/physics/runtime/native_tu15_portal_predicate_leaves.hpp').read_bytes()).hexdigest(),
        input_coverage=['IEEE edge words','stack/vector aliases','16 stack alignments','all x87 precisions and rounding controls',
            'all SSE rounding controls and DAZ/FTZ','zero/degenerate/negative-count polygons','128-vertex portals',
            'separate distance/projection plane owners','PC53/PC64 cancellation adversary'],
        comparisons=['all memory bytes','ordered reads and stores','all GP/SSE registers','all arithmetic flags',
            'MXCSR','x87 depth/TOP','x87 exception and C1 status'],live_frame_qualification=False,
        exceptional_partial_abi_qualification=False,scratch_directory=str(scratch),retained_large_artifacts=[])
    if args.report:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
