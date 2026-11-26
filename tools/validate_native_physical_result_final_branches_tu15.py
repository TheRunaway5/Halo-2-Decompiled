#!/usr/bin/env python3
"""Compare the last two TU1.5 physical-result caller stages and child ABI."""

import argparse
import json
import os
from pathlib import Path
import re

import native_physical_result_activity_replacements as activity
import native_physical_result_classification_e4b81_replacements as classification
import native_physical_result_contact_e4dc0_replacements as contact
import native_physical_result_decay_e51d6_replacements as decay
import native_physical_result_final_branches_replacements as binding
from specialize_native_operands import specialize
from validate_native_physical_result_decay_e51d6_tu15 import invoke


ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'src/physics/physics_live_support_tu15_generated.cpp'
FIXTURE=ROOT/'tests/native_tu15_e4a40_final_branches_tests.cpp'


def exports(stages):
    def retained(bounds,children):
        stage=stages[bounds]
        for site,target,resume,dispatch in children:
            stage=stage.replace(activity.child_call(site,target,resume,dispatch),
                                f's.push(0x{site}u,0x{resume}u);route(s,0x{site}u,0x{target}u);')
        stage=binding.retained(stage,bounds).removeprefix(f'L{bounds[0]}: ')
        return re.sub(r'if\(s.services.instruction\)\{s.services.instruction\((0x[0-9a-f]+u)\);\}',
                      r's.instruction_literal(\1);',stage)
    notify=retained(binding.NOTIFY,binding.NOTIFY_CHILDREN)
    impulse=retained(binding.IMPULSE,binding.IMPULSE_CHILDREN)
    return (f'void retained_notify(WorldState &s,const Route &route) {{\n{notify}Le4dc0: return;\n}}\n'
            f'void retained_impulse(WorldState &s,const Route &route,ImpulseEntry entry) {{\n'
            f'    if(entry==ImpulseEntry::publish)goto Le51cd;\n{impulse}}}\n'
            f'bool selected_notify(WorldState &s,const Route &route) {{return {binding.NOTIFY_DETAIL}::notify(s,route);}}\n'
            f'bool selected_impulse(WorldState &s,const Route &route,ImpulseEntry entry) '
            f'{{return {binding.IMPULSE_DETAIL}::publish(s,route,entry);}}\n')


def qualify(args,scratch,report):
    source=SOURCE.read_text()
    stages=binding.canonical_stages(source)
    if args.staged_donor:
        donor=args.staged_donor.read_text()
        old='static '+re.search(r'^void run_e4a40\(WorldState &s\) \{.*?'+re.escape(activity.CLOSE),donor,re.M|re.S)[0]
        body=binding.new_body(classification.new_body(decay.new_body(contact.new_body(activity.new_body(old)))))
    else:
        active,metadata=specialize(source)
        if any(metadata['native_spatial_owners'].count(entry)!=1 for entry in (binding.NOTIFY[0],binding.IMPULSE[0])):
            raise ValueError('Final physical-result production binding is missing')
        body=activity.BODY.search(active)[0]
    (scratch/'physical_result_final_branches_reference.inc').write_text(exports(stages))
    sites={int(pc,16) for stage in stages.values() for pc in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',stage)}
    children={int(pc,16) for pc,*_ in (*binding.NOTIFY_CHILDREN,*binding.IMPULSE_CHILDREN)}
    report.update(bound_owner_sha256=activity.sha(body), profiles={},
        sources={str(p.relative_to(ROOT)):activity.sha(p.read_text()) for p in
                 (SOURCE,FIXTURE,*[ROOT/'src'/header for header in binding.HEADERS])})
    flags=['-std=c++20','-O2','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math',
           *(f'-I{p}' for p in (ROOT/'include',ROOT/'src',ROOT/'tests',scratch))]
    for name,extra in (('release',[]),('asan_ubsan',['-O1','-fsanitize=address,undefined',
                 '-fno-omit-frame-pointer','-fno-sanitize-recover=all','-fno-pie','-no-pie'])):
        binary=scratch/name
        invoke([args.compiler,*flags,*extra,FIXTURE,'-o',binary],scratch)
        output=json.loads(invoke([binary],scratch))
        observed=set(output.pop('sites'))
        output.update(observed_sites=len(observed),missing_sites=sorted(hex(pc) for pc in sites-observed))
        report['profiles'][name]=output
        print(name,output,flush=True)
        if observed!=sites or set(output['children'])!=children:
            raise ValueError('Final physical-result fixture missed an instruction or child')
    report['passed']=True


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',default='c++')
    parser.add_argument('--staged-donor',type=Path)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    report=dict(passed=False,scratch=str(scratch),staged=bool(args.staged_donor))
    try:
        qualify(args,scratch,report)
    except BaseException as error:
        report['error']=str(error)
        raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':
    main()
