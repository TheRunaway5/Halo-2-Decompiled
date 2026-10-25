#!/usr/bin/env python3
"""Extend an active owned CTest run to all existing destruction controls.

Fixture setup includes the other cases; fixture cleanup retains bounded
reports before the enclosing run_test.py removes the owned build.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def retain(args):
    registration=json.loads(args.registration.read_text());build=Path(registration['build']).resolve(strict=True)
    args.output=args.output.resolve()
    if args.output!=args.registration.resolve().parent:raise ValueError('Different cohort retention owner')
    results={};total=0;comparisons=0;material_calls=0
    for name in registration['cases']:
        path=build/f'destruction-{name}.json'
        if path.is_symlink()or path.resolve().parent!=build:raise ValueError('Case report escaped its owned build')
        if not path.is_file():results[name]=dict(passed=False,error='No current case report');continue
        raw=path.read_bytes();total+=len(raw)
        if len(raw)>1024*1024 or total>8*1024*1024 or raw.startswith((b'\x7fELF',b'MZ')):
            raise ValueError('Case reports exceed bounded retention')
        data=json.loads(raw);post=data.get('post_destruction_continuation',{});pending=data.get('pending_response_continuation',{})
        count=post.get('complete_encoded_world_sha256_comparisons',0)+pending.get('complete_encoded_world_sha256_comparisons',0)
        activity=data.get('native_damage_material_activity',{})
        passed=data.get('passed')is True and data.get('declared_external_clock')is True and post.get('complete_encoded_world_sha256_comparisons')==16
        if name=='power_module':passed &= pending.get('complete_encoded_world_sha256_comparisons')==16
        if name in ('ghost','wraith_minigun','campaign_flood_clump'):passed &= activity.get('calls',0)>0
        target=args.output/'evidence'/path.name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
        results[name]=dict(passed=bool(passed),report=str(target),sha256=sha(target),bytes=len(raw),
            post_comparisons=post.get('complete_encoded_world_sha256_comparisons',0),pending_comparisons=pending.get('complete_encoded_world_sha256_comparisons',0),
            native_material_activity=activity,error=data.get('error'))
        if passed:comparisons+=count;material_calls+=activity.get('calls',0)
    unchanged=sha(Path(registration['source'])/'CMakeLists.txt')==registration['cmake_after']
    helper=Path(registration['source'])/'tools'/Path(__file__).name
    unchanged &= sha(helper)==registration['helper_sha256']
    passed=unchanged and len(results)==len(registration['cases'])and all(row['passed']for row in results.values())
    report=dict(passed=passed,scope='Fresh full existing30-case native destruction cohort after material kernel replacement; all full-definition and D01-D13 gates remain open',
        cases=results,total_complete_world_comparisons=comparisons,actual_native_material_calls=material_calls,
        registration_sha256=sha(args.registration),registration_unchanged=unchanged,retained_report_bytes=total,
        full_goal_complete=False,retained_large_artifacts=[])
    (args.output/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=passed,cases=len(results),world_comparisons=comparisons,native_material_calls=material_calls)),flush=True)
    return 0 if passed else 1


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--retain',action='store_true')
    parser.add_argument('--admission',type=Path)
    parser.add_argument('--prior-cohort',type=Path)
    parser.add_argument('--registration',type=Path)
    parser.add_argument('--source-only',action='store_true',help='Prepare fixture registration before the owned build configures')
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    if args.retain:
        if not args.registration:parser.error('Retention requires its exact registration')
        return retain(args)
    if not args.admission or not args.prior_cohort:parser.error('Registration requires admission and prior cohort')
    admission=json.loads(args.admission.read_text());prior=json.loads(args.prior_cohort.read_text())
    owner=Path(admission['owner']).resolve(strict=True);source=owner/'source';build=owner/'build'
    if args.source_only:
        if admission.get('status')!='prepared'or not admission.get('validation_not_run'):raise ValueError('Source registration requires a prepared owner')
    else:
        worker=json.loads((args.admission.parent/'validation/report.json').read_text())
        if worker['status']!='running'or worker['steps'][-1]['name']!='build'or worker['steps'][-1]['exit_code']is not None:
            raise ValueError('Only extend the confirmed active build before CTest starts')
    if not prior['passed']or len(prior['cases'])!=30:raise ValueError('Existing complete30 cohort is unqualified')
    cases=list(prior['cases']);primary=admission['cases'];setup=[n for n in cases if n not in primary]
    if any(not re.fullmatch('[a-z0-9_]+',n)for n in cases)or not set(primary)<=set(cases):raise ValueError('Changed destruction case names')
    testfile=build/'CTestTestfile.cmake';generated=testfile.read_text()if testfile.exists()else''
    declared=(source/'CMakeLists.txt').read_text()if args.source_only else generated
    if any(not re.search(r'add_test\((?:NAME )?"?destruction_mcc_'+n+r'\b',declared)for n in cases):raise ValueError('A full cohort case is not registered')
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False);receipt=args.output/'registration.json'
    helper=source/'tools'/Path(__file__).name
    if helper.exists():raise ValueError('Cohort extension helper already exists')
    helper.write_bytes(Path(__file__).read_bytes())
    fixture='native_material_full_cohort';cleanup='native_material_full_cohort_receipt'
    props='set_tests_properties('+ ' '.join('destruction_mcc_'+n for n in setup)+f' PROPERTIES FIXTURES_SETUP {fixture})\n'
    props+='set_tests_properties('+ ' '.join('destruction_mcc_'+n for n in primary)+f' PROPERTIES FIXTURES_REQUIRED {fixture})\n'
    props+='set_tests_properties(destruction_mcc_power_module PROPERTIES ENVIRONMENT_MODIFICATION "HALO2_DESTRUCTION_PENDING_CONTINUATION=set:1")\n'
    command=[sys.executable,str(helper),'--retain','--registration',str(receipt),'--output',str(args.output)]
    def quoted(values):return ' '.join('"'+v.replace('\\','\\\\').replace('"','\\"')+'"'for v in values)
    cmake=source/'CMakeLists.txt';before=sha(cmake)
    addition='\n# BEGIN owned native material full cohort\n'+props
    addition+='add_test(NAME '+cleanup+' COMMAND '+quoted(command)+')\n'
    addition+=f'set_tests_properties({cleanup} PROPERTIES FIXTURES_CLEANUP {fixture} TIMEOUT 60)\n'
    addition+='# END owned native material full cohort\n'
    if '# BEGIN owned native material full cohort'in cmake.read_text():raise ValueError('Cohort already extended')
    cmake.write_text(cmake.read_text()+addition)
    # Publish the same registration for the currently running build's first
    # CTest phase. Any automatic CMake regeneration reproduces these settings.
    wrapped=[sys.executable,str(source/'tools/run_test.py'),'--scratch-root',str(build/'Testing/Temporary/scratch'),'--',*command]
    emitted='\n'+props+'add_test("'+cleanup+'" '+quoted(wrapped)+')\n'
    emitted+=f'set_tests_properties({cleanup} PROPERTIES FIXTURES_CLEANUP {fixture} TIMEOUT 60)\n'
    if not args.source_only:testfile.write_text(generated+emitted)
    report=dict(passed=False,owner=str(owner),source=str(source),build=str(build),cases=cases,primary_cases=primary,
        cmake_before=before,cmake_after=sha(cmake),cmake_addition=addition,helper_sha256=sha(helper),
        generated_registration_sha256=sha(testfile)if testfile.exists()else None,prior_cohort_sha256=sha(args.prior_cohort),
        scope='Test registration only; original engine sources, fixtures, stimulus and expected outcomes unchanged',
        source_owner=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[])
    receipt.write_text(json.dumps(report,indent=2)+'\n')
    if args.source_only:
        report.update(status='prepared',validation_not_run=True)
        receipt.write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps(dict(passed=False,status='prepared',cases=len(cases),registration=str(receipt))),flush=True);return 0
    check=subprocess.run(['ctest','--test-dir',str(build),'--show-only=json-v1','--tests-regex',
        '^destruction_mcc_(ghost|campaign_guntower_no_damage|wraith_minigun|campaign_flood_clump)$'],capture_output=True,text=True,check=True)
    selected=json.loads(check.stdout)['tests'];names={r['name']for r in selected}
    report['expanded_selected_tests']=sorted(names)
    report['passed']=names=={'destruction_mcc_'+n for n in cases}|{cleanup}
    receipt.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],selected_tests=len(names),cases=len(cases),registration=str(receipt))),flush=True)
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
