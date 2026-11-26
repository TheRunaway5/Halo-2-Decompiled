#!/usr/bin/env python3
"""Compare the indexed death-cycle motion paths and pinned TU1.5 rollback."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_death_cycle_motion_records_10387c_replacements as binding
from specialize_native_operands import specialize

ROOT=Path(__file__).resolve().parents[1]
UNIT=ROOT/'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'
FIXTURE=ROOT/'tests/native_tu15_death_cycle_motion_records_10387c_tests.cpp'
HEADER=ROOT/'src/gameplay/units/native_tu15_death_cycle_motion_records_10387c.hpp'


def export_reference(body):
    first=binding.hook(*binding.ENTRIES[0])
    start=body.index(first)
    end=body.index(binding.END,start)
    phase=body[start:end]
    for pc,entry in binding.ENTRIES:
        hook=binding.hook(pc,entry)
        if phase.count(hook)!=1:
            raise ValueError('Expected exact prepared record-motion ingress')
        phase=phase.replace(hook,f'\nL{pc}: ',1)
    expected={int(pc,16) for pc in re.findall(r's\.instruction_literal\(0x([0-9a-f]+)u\)',phase)}
    if len(expected)!=130:
        raise ValueError('Expected all 130 original record-motion sites')
    source=('void reference_motion_records(WorldState &s,records::Entry entry) {\n'
            '    if(entry==records::Entry::alternate)goto L1038f5;\n'
            '    if(entry==records::Entry::contextual)goto L103994;\n'
            + phase + '\nL103cd3: s.instruction_literal(0x103cd3u); return;\n'
            'L1040f0: s.instruction_literal(0x1040f0u);\n}\n')
    return source,expected


def qualify(args,scratch,report):
    canonical=UNIT.read_text()
    advanced,_=specialize(canonical)
    staged=binding.INCLUDE not in advanced
    if staged:
        advanced,owners=binding.install(canonical,advanced)
        if owners!=[binding.OWNER]:
            raise ValueError('Staged record-motion owner did not bind')
    body=binding.BODY.search(advanced)[0]
    source,sites=export_reference(body)
    (scratch/'death_motion_records_reference.inc').write_text(source)
    report.update(staged_only=staged,bound_body_sha256=binding.digest(body),kernel_sha256=binding.digest(HEADER.read_text()),
                  fixture_sha256=binding.digest(FIXTURE.read_text()),profiles={})
    flags=['-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
           *(f'-I{p}' for p in (ROOT/'include',ROOT/'src',scratch))]
    env={k:v for k,v in os.environ.items() if k!='HALO2_REFERENCE_DEATH_CYCLE_MOTION_RECORDS_10387C'}
    for name,extra in (('release',[]),('asan_ubsan',['-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all',
                                          '-fno-omit-frame-pointer','-fno-pie','-no-pie'])):
        executable=scratch/name
        subprocess.run([args.compiler,*flags,*extra,FIXTURE,'-o',executable],check=True,cwd=scratch,env=env)
        result=subprocess.run([executable],cwd=scratch,env=env,text=True,capture_output=True)
        if result.returncode:
            raise ValueError(f'{name} record-motion fixture failed\n{result.stdout[-800:]}\n{result.stderr[-5000:]}')
        reached={int(pc,16) for pc in result.stderr.split()}
        report['profiles'][name]={**json.loads(result.stdout),'unreached_sites':sorted(hex(pc) for pc in sites-reached)}
        if reached!=sites:
            raise ValueError(f'{name} missed an original record-motion instruction')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',default='c++')
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    report=dict(passed=False,scratch=str(scratch))
    try:
        qualify(args,scratch,report)
        report['passed']=True
    except BaseException as error:
        report['error']=str(error)
        raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':
    main()
