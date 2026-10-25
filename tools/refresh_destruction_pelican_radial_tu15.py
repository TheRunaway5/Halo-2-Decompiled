#!/usr/bin/env python3
"""Admit the qualified Pelican radial caller into the same finite native owner."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
CASES=('campaign_pelican_rear_gun','campaign_pelican_rocket_pod')
FILTER='^destruction_mcc_(ghost|campaign_guntower_no_damage|wraith_minigun|campaign_flood_clump)$'


def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def apply_caller(before,probe):
    after=before
    for needle,replacement in probe['caller_substitutions']:
        if after.count(needle)!=1:raise ValueError('Qualified Pelican caller seam drifted')
        after=after.replace(needle,replacement)
    if hashlib.sha256(after.encode()).hexdigest()!=probe['fixture_probe_sha256']:
        raise ValueError('Different qualified Pelican caller')
    return after


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline',required=True,type=Path)
    parser.add_argument('--routing-probe',required=True,type=Path)
    parser.add_argument('--original-probe',required=True,type=Path)
    parser.add_argument('--runtime-inputs',required=True,type=Path)
    parser.add_argument('--cohort-output',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use the same finite tools/run_test.py owner')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source';build=owner/'build'
    baseline=json.loads(args.baseline.read_text());routing=json.loads(args.routing_probe.read_text());original=json.loads(args.original_probe.read_text())
    old_registration=Path(baseline['full_cohort_registration']).resolve();registration=json.loads(old_registration.read_text())
    if not baseline['passed']or not baseline['source_unchanged']or not routing['passed']or not original['passed']:
        raise ValueError('Baseline/radial/original qualifications are incomplete')
    if any(Path(row['owner']if 'owner'in row else row['native_owner']).resolve()!=owner for row in (baseline,routing,original)):
        raise ValueError('Different native owner')
    if not json.loads((old_registration.parent/'summary.json').read_text())['passed']:raise ValueError('Existing complete cohort failed')
    if any(sha(source/name)!=row['after']for name,row in baseline['changes'].items()):raise ValueError('Baseline engine/caller source drifted')
    if sha(source/'CMakeLists.txt')!=registration['cmake_after']:raise ValueError('Baseline registration drifted')
    runtime=json.loads(args.runtime_inputs.read_text())
    if not runtime['passed']or any(sha(name)!=row['sha256']for name,row in runtime['files'].items()):raise ValueError('MCC runtime input drifted')
    fixture='tests/destruction_runtime_tu15_tests.cpp';before=(source/fixture).read_text()
    normal=apply_caller(before,routing);after=apply_caller(before,original)
    selector=('(!campaign_no_damage || Bytes(source.memory().view(abi.gp[4],4)).u32(0)==0xda180u);',
        '(!campaign_no_damage || Bytes(source.memory().view(abi.gp[4],4)).u32(0)==(pelican_damage?0xda875u:0xda180u));')
    if after.count(selector[1])!=1 or after.replace(selector[1],selector[0])!=normal:
        raise ValueError('Original capture changed more than its diagnostic selector')
    args.output=args.output.resolve();args.cohort_output=args.cohort_output.resolve()
    if args.output.exists()or args.cohort_output.exists():raise ValueError('Refresh output already exists')
    receipt=args.cohort_output/'registration.json';cmake_before=(source/'CMakeLists.txt').read_text()
    cmake=cmake_before.replace(str(old_registration),str(receipt)).replace(str(old_registration.parent),str(args.cohort_output))
    map_path=Path(json.loads((args.routing_probe.parent/'validation/evidence'/(CASES[0]+'.json')).read_text())['map'])
    if str(map_path)not in runtime['files']or sha(map_path)!=routing['mcc_map_sha256']:raise ValueError('Radial map outside qualified input set')
    security=build/'online-protocol/Release/libhalo2_online_security_ffi.so'
    addition='\n# BEGIN qualified Pelican radial no-information gates\n'
    for case in CASES:
        addition+=f'add_test(NAME destruction_mcc_{case} COMMAND "{sys.executable}" "{source/"tools/run_test.py"}" -- $<TARGET_FILE:destruction_runtime_tu15_tests> "{map_path}" "{map_path.parent}" {case} "${{CMAKE_BINARY_DIR}}/destruction-{case}.json")\n'
        addition+=f'set_tests_properties(destruction_mcc_{case} PROPERTIES TIMEOUT 360 FIXTURES_SETUP native_material_full_cohort ENVIRONMENT "HALO2_DESTRUCTION_FIXED_CLOCK=1;HALO2_DESTRUCTION_CONTINUATION=1;H2_SECURITY_LIBRARY={security}")\n'
    addition+='# END qualified Pelican radial no-information gates\n';cmake+=addition
    args.output.mkdir(parents=True);args.cohort_output.mkdir(parents=True)
    report=copy.deepcopy(baseline)
    report.update(passed=False,status='refreshing',source_unchanged=False,validation_returncode=None,
        baseline=dict(path=str(args.baseline.resolve()),sha256=sha(args.baseline)),
        radial_qualification=dict(path=str(args.routing_probe.resolve()),sha256=sha(args.routing_probe)),
        original_qualification_per_attachment=dict(path=str(args.original_probe.resolve()),sha256=sha(args.original_probe)),
        full_cohort_registration=str(receipt),tool_sha256=sha(__file__),retained_large_artifacts=[],
        additional_attachment_cases=list(CASES),attachment_cases_share_one_family_stimulus=True,
        source_refresh=dict(fixture_before_sha256=sha(source/fixture),fixture_after_sha256=original['fixture_probe_sha256'],
            caller_substitutions=original['caller_substitutions'],capture_selector_is_diagnostic_only=True,
            engine_sources_changed=False,fresh_source_mtime=True))
    def save():(args.output/'admission.json').write_text(json.dumps(report,indent=2)+'\n')
    save();(source/fixture).write_text(after);(source/'CMakeLists.txt').write_text(cmake)
    report['changes'][fixture]['after']=sha(source/fixture)
    new_reg=copy.deepcopy(registration)
    new_reg.update(status='prepared',passed=False,validation_not_run=True,cmake_after=sha(source/'CMakeLists.txt'),
        cmake_addition=registration['cmake_addition'].replace(str(old_registration),str(receipt)).replace(str(old_registration.parent),str(args.cohort_output))+addition,
        prior_cohort_sha256=sha(old_registration.parent/'summary.json'),
        expected_selected_tests=sorted(set(registration['expected_selected_tests'])|{'destruction_mcc_'+case for case in CASES}),
        additional_attachment_cases=list(CASES),attachment_cases_share_one_family_stimulus=True)
    receipt.write_text(json.dumps(new_reg,indent=2)+'\n')
    report['retired_prior_generated_reports']=[]
    for name in [f'destruction-{case}.json'for case in registration['cases']+list(CASES)]+['pelican-children.json']:
        path=build/name
        if path.is_symlink()or path.resolve().parent!=build:raise ValueError('Report escaped the owned build')
        if path.exists():
            report['retired_prior_generated_reports'].append(dict(path=str(path),sha256=sha(path),bytes=path.stat().st_size));path.unlink()
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','destruction_runtime_tu15_tests','--target','halo2_native','--target','physics_multiplayer_acceptance',
        '--filter',FILTER,'--jobs','2','--test-jobs','1','--output',str(args.output/'validation')]
    for case in baseline['cases']+list(CASES):command+=['--retain-report',f'destruction-{case}.json']
    command+=['--retain-report','pelican-children.json'];report.update(status='running',command=command,source_cmake_sha256=new_reg['cmake_after']);save()
    print(json.dumps(dict(status='expanded_native_radial_cohort_running',expected_CTests=len(new_reg['expected_selected_tests']),owner=str(owner))),flush=True)
    code=subprocess.run(command,env=dict(os.environ,HALO2_DESTRUCTION_FIXED_CLOCK='1',HALO2_DESTRUCTION_CONTINUATION='1',
        HALO2_DESTRUCTION_REQUIRE_NATIVE_MODEL='1',HALO2_DESTRUCTION_REQUIRE_NATIVE_MATERIAL='1'),stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    selected=subprocess.run(['ctest','--test-dir',str(build),'--show-only=json-v1','--tests-regex',FILTER],capture_output=True,text=True)
    names=sorted(row['name']for row in json.loads(selected.stdout)['tests'])if selected.returncode==0 else[]
    expansion=dict(passed=names==new_reg['expected_selected_tests'],selected_tests=names,registration_sha256=sha(receipt))
    (args.cohort_output/'expansion.json').write_text(json.dumps(expansion,indent=2)+'\n')
    results={}
    for case in CASES:
        path=args.output/'validation/evidence'/f'destruction-{case}.json';data=json.loads(path.read_text())if path.exists()else{}
        results[case]=dict(passed=data.get('passed')is True and data.get('original_radial_no_information_gate_verified')is True and
            data.get('post_destruction_continuation',{}).get('complete_encoded_world_sha256_comparisons')==16,error=data.get('error'),report=str(path))
    report.update(validation_returncode=code,attachment_cases=results,fixture_expansion_passed=expansion['passed'],
        source_unchanged=all(sha(source/name)==row['after']for name,row in report['changes'].items())and
            sha(source/'CMakeLists.txt')==new_reg['cmake_after']and all(sha(source/name)==row['sha256']for name,row in report['sdk_references'].items()),
        runtime_inputs_unchanged=all(sha(name)==row['sha256']for name,row in runtime['files'].items()))
    summary=args.cohort_output/'summary.json';report['full_existing_cohort_passed']=summary.exists()and json.loads(summary.read_text()).get('passed')is True
    ctor=args.output/'validation/evidence/pelican-children.json';report['constructor_passed']=ctor.exists()and json.loads(ctor.read_text()).get('source_death_and_respawn_readback_qualified')is True
    report['passed']=code==0 and all(row['passed']for row in results.values())and all(report[key]for key in (
        'fixture_expansion_passed','source_unchanged','runtime_inputs_unchanged','full_existing_cohort_passed','constructor_passed'))
    report['status']='passed'if report['passed']else'failed';save()
    print(json.dumps(dict(passed=report['passed'],report=str(args.output/'admission.json'))),flush=True)
    return 0 if report['passed']else code or 1


if __name__=='__main__':raise SystemExit(main())
