#!/usr/bin/env python3
"""Check real cleared/rebound object-node fallback and compiled guard controls."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

from generated_routine_ownership import function_bodies

ROOT=Path(__file__).resolve().parents[1]
FILES=('src/physics/runtime/native_tu15_object_node_admission.hpp',
    'src/physics/runtime/native_tu15_object_prediction_copy.hpp',
    'tests/native_tu15_object_node_live_admission_tests.cpp',
    'tests/native_tu15_object_prediction_copy_reference.inc',
    'tools/validate_native_object_node_live_admission.py')
REFERENCE_PIN='8308d639cb2a5de597db3c3e85dec6553185384e6e58adfe074431c268986310'
def hashes():return {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in FILES}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',default='c++')
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--mutations',action='store_true')
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    report=dict(passed=False,source_before=hashes(),owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        body=next(body for match,body in function_bodies((ROOT/FILES[3]).read_text())if match[1]=='adcd0')
        if hashlib.sha256(body.encode()).hexdigest()!=REFERENCE_PIN:raise ValueError('Complete retained prediction body drift')
        base=[args.compiler,'-std=c++20','-O1'if args.sanitizers else'-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(ROOT/FILES[2])]
        binary=scratch/'guard'
        command=base+['-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        done=subprocess.run(command,capture_output=True,text=True)
        if done.returncode:raise ValueError('Live guard probe compile failed '+done.stderr[-3000:])
        done=subprocess.run([str(binary),'--expect-fixed'],capture_output=True,text=True)
        if done.returncode:raise ValueError('Real live guard fallback mismatch '+done.stderr[-3000:])
        report.update(stdout=done.stdout,complete_retained_reference_body_sha256=REFERENCE_PIN,
            actual_default_reference_full_returns=True,sanitizers=args.sanitizers,
            new_physical_original_image_proof=False)
        controls=[]
        if args.mutations:
            header=(ROOT/FILES[0]).read_text()
            for name,old,new in(
                ('cached-empty','if(!s.services.instruction)return !s.instruction_cursor&&!s.literal_service_sites&&!s.observe;',
                 'if(s.observe&&!s.services.instruction)return false;'),
                ('rebound-type','||s.services.instruction.target_type()!=*c->callback_type','')):
                if header.count(old)!=1:raise ValueError('Missing compiled live guard control '+name)
                directory=scratch/name;path=directory/'physics/runtime/native_tu15_object_node_admission.hpp';path.parent.mkdir(parents=True)
                path.write_text(header.replace(old,new,1));binary=directory/'guard'
                done=subprocess.run(base[:1]+['-I'+str(directory)]+base[1:]+['-o',str(binary)],capture_output=True,text=True)
                if done.returncode:raise ValueError('Live guard mutant did not compile '+done.stderr[-3000:])
                done=subprocess.run([str(binary),'--expect-fixed'],capture_output=True,text=True)
                if done.returncode==0 or 'genuine retained fallback'not in done.stderr:raise ValueError('Live guard semantic control escaped '+name+' '+done.stderr)
                controls.append(dict(mutation=name,compiled=True,rejected=True,reason=done.stderr.strip()))
        report.update(passed=True,compiled_semantic_mutation_rejections=controls)
    except BaseException as error:report.update(error=repr(error));raise
    finally:
        report['source_after']=hashes()
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS authentic full-return live fallback and compiled guards');return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
