#!/usr/bin/env python3
"""Compare native TU1.5 animation requests with the pinned original body."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_animation_request_replacements as binding
from specialize_native_operands import specialize

ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT/'src/physics/physics_live_support_tu15_generated.cpp'
HEADER = ROOT/'src'/binding.HEADER
TEST = ROOT/'tests/native_tu15_animation_request_tests.cpp'
LITERAL = re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}')


def fixture_reference(body):
    for pc,resume,target,routed in binding.routes(body):
        push = f's.push(0x{pc:x}u,0x{resume:x}u);'
        dispatch = f'dispatch(s,target,0x{pc:x}u,false);' if routed else f'run_{target:x}(s);'
        original = push+dispatch
        if body.count(original)!=1:
            raise ValueError(f'Original fixture child changed at {pc:x}')
        body=body.replace(original,push+f'fixture_child(s,0x{target:x}u,0x{pc:x}u);')
    return LITERAL.sub(r's.instruction_literal(\1);',binding.retained(body))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',default='c++')
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    canonical=UNIT.read_text()
    original,=binding.BODY.findall(canonical)
    alias,install=specialize(canonical)
    body,=binding.BODY.findall(alias)
    if install['native_spatial_owners'].count('113d30')!=1 or binding.hook(original) not in body:
        raise ValueError('Current 113D30 owner did not bind')
    (scratch/'animation_request_reference.inc').write_text(fixture_reference(original)+'\n')
    sites={int(pc[2:-1],16) for pc in LITERAL.findall(original)}
    report={'passed':False,'scratch':str(scratch),'canonical_body_sha256':binding.digest(original),
            'bound_body_sha256':binding.digest(body),'kernel_sha256':binding.digest(HEADER.read_text()),
            'fixture_sha256':binding.digest(TEST.read_text()),'retained_large_artifacts':[],'profiles':{}}
    profiles=(
        ('release',[],3072),
        ('asan_ubsan',['-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer',
                       '-fno-sanitize-recover=all','-fno-pie','-no-pie'],1536),
    )
    try:
        for name, flags, count in profiles:
            executable=scratch/name
            command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                     *flags,f'-I{ROOT/"include"}',f'-I{ROOT/"src"}',f'-I{scratch}',str(TEST),'-o',str(executable)]
            subprocess.run(command,check=True,cwd=scratch)
            run=subprocess.run([str(executable),str(count)],capture_output=True,text=True,cwd=scratch)
            if run.returncode:
                raise RuntimeError(f'{name}\n'+run.stderr[-6000:])
            reached={int(pc,16) for pc in run.stderr.split()}
            assert reached<=sites
            report['profiles'][name]={**json.loads(run.stdout),'unreached_sites':list(map(hex,sorted(sites-reached)))}
        if report['profiles']['release']['unreached_sites']:
            raise ValueError('Release corpus missed original 113D30 sites')
        report['passed']=True
    except BaseException as error:
        report['error']=str(error)
        raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':
    main()
