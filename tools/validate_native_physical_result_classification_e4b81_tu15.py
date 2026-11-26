#!/usr/bin/env python3
"""Compare the TU1.5 contact classification and its four caller boundaries."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_physical_result_activity_replacements as activity
import native_physical_result_classification_e4b81_replacements as classification
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT/'src/physics/physics_live_support_tu15_generated.cpp'
FIXTURE = ROOT/'tests/native_tu15_e4b81_physical_result_classification_tests.cpp'
ROLLBACK = ('HALO2_REFERENCE_PHYSICAL_RESULT_ACTIVITY', 'HALO2_REFERENCE_PHYSICAL_RESULT_CLASSIFICATION_E4B81')


def exports(original, selected):
    if selected.count(classification.hook()) != 1:
        raise ValueError('Physical classification production route changed')
    stage = original
    for site, target, resume, dispatch in classification.CHILDREN:
        stage = stage.replace(activity.child_call(site, target, resume, dispatch),
                              f's.push(0x{site}u,0x{resume}u);route(s,0x{site}u,0x{target}u);')
    stage = classification.retained(stage)
    stage = re.sub(r'if\(s.services.instruction\)\{s.services.instruction\((0x[0-9a-f]+u)\);\}',
                   r's.instruction_literal(\1);', stage)
    return (f'void retained(WorldState &s, const Route &route) {{\n{stage}}}\n'
            f'bool selected(WorldState &s, const Route &route) {{return {classification.DETAIL}::classify(s,route);}}\n')


def qualify(args, scratch, report):
    canonical = SOURCE.read_text()
    raw_stage = classification.canonical_stage(canonical)
    active, metadata = specialize(canonical)
    if args.staged:
        active, entries = classification.install(canonical, active)
    else:
        entries = [pc for pc in metadata['native_spatial_owners'] if pc == classification.ENTRY]
    if entries != [classification.ENTRY]:
        raise ValueError('Physical classification binding is missing')
    body = activity.BODY.search(active)[0]
    (scratch/'physical_result_classification_reference.inc').write_text(exports(raw_stage, body))
    sites = {int(pc,16) for pc in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',raw_stage)}
    children = {int(pc,16) for pc,*_ in classification.CHILDREN}
    report.update(bound_owner_sha256=activity.sha(body), original_sites=len(sites), profiles={},
        sources={str(path.relative_to(ROOT)):activity.sha(path.read_text()) for path in
                 (SOURCE, FIXTURE, ROOT/'src'/classification.HEADER, ROOT/'tools/native_physical_result_classification_e4b81_replacements.py')})
    flags = ['-std=c++20','-O2','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math',
             *(f'-I{p}' for p in (ROOT/'include', ROOT/'src', ROOT/'tests', scratch))]
    env = {name:value for name,value in os.environ.items() if name not in ROLLBACK}
    for name, extra in (('release',[]),('asan_ubsan',['-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all',
                         '-fno-omit-frame-pointer','-fno-pie','-no-pie'])):
        binary = scratch/name
        subprocess.run([args.compiler,*flags,*extra,FIXTURE,'-o',binary],cwd=scratch,env=env,check=True)
        run = subprocess.run([binary],cwd=scratch,env=env,check=True,text=True,stdout=subprocess.PIPE)
        row = json.loads(run.stdout)
        reached = set(row.pop('sites'))
        row.update(observed_sites=len(reached), missing_sites=sorted(hex(pc) for pc in sites-reached))
        report['profiles'][name] = row
        print(name,row,flush=True)
        if reached != sites or set(row['children']) != children:
            raise ValueError('Physical classification fixture missed an instruction or child route')
    report['passed'] = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',default='c++')
    parser.add_argument('--staged',action='store_true')
    parser.add_argument('--report',type=Path,required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    report = dict(passed=False,staged=args.staged,scratch=str(scratch))
    try:
        qualify(args,scratch,report)
    except BaseException as error:
        report['error'] = str(error)
        raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__ == '__main__':
    main()
