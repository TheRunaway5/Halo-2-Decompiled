#!/usr/bin/env python3
"""Refresh the same finite native owner with the qualified player readback fix.

Reconstruct only the exact passing probe's caller changes and one native engine
correction. Rebuild native/SDL targets and rerun the complete existing cohort
plus the Pelican construction/death/respawn regression through run_tests.py.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline',required=True,type=Path)
    parser.add_argument('--constructor-probe',required=True,type=Path)
    parser.add_argument('--cohort-output',required=True,type=Path)
    parser.add_argument('--runtime-inputs',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use the same finite tools/run_test.py owner')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source';build=owner/'build'
    baseline=json.loads(args.baseline.read_text());probe=json.loads(args.constructor_probe.read_text())
    if Path(baseline['owner']).resolve()!=owner or Path(probe['native_owner']).resolve()!=owner:
        raise ValueError('Refresh belongs to another native owner')
    old_registration=Path(baseline['full_cohort_registration']).resolve()
    registration=json.loads(old_registration.read_text())
    old_cohort=json.loads((old_registration.parent/'summary.json').read_text())
    proof_path=args.constructor_probe.parent/'validation/evidence/pelican-children.json'
    proof=json.loads(proof_path.read_text())
    if not(baseline['passed']and baseline['source_unchanged']and old_cohort['passed']and len(old_cohort['cases'])==30):
        raise ValueError('The preceding native/SDL and complete30 qualification has not passed')
    if not(probe['passed']and proof['passed']and proof.get('source_death_and_respawn_readback_qualified')):
        raise ValueError('The explicit actual death/respawn regression has not passed')
    runtime_inputs=json.loads(args.runtime_inputs.read_text())
    if not runtime_inputs['passed']or any(sha(name)!=row['sha256']for name,row in runtime_inputs['files'].items()):
        raise ValueError('Qualified MCC runtime inputs drifted')
    if proof['map']not in runtime_inputs['files']or sha(proof['map'])!=probe['mcc_map_sha256']:
        raise ValueError('Constructor map is outside the qualified runtime input set')
    if any(sha(source/name)!=row['after']for name,row in baseline['changes'].items()):raise ValueError('Baseline source drift')
    if sha(source/'CMakeLists.txt')!=registration['cmake_after']:raise ValueError('Baseline CTest drift')
    if any(sha(source/name)!=row['sha256']for name,row in baseline['sdk_references'].items()):raise ValueError('SDK reference drift')
    fixture='tests/destruction_runtime_tu15_tests.cpp';fixture_before=(source/fixture).read_text();fixture_after=fixture_before
    for before,after in probe['diagnostic_caller_substitutions']:
        if fixture_after.count(before)!=1:raise ValueError('Qualified caller seam drifted')
        fixture_after=fixture_after.replace(before,after)
    if hashlib.sha256(fixture_after.encode()).hexdigest()!=probe['fixture_probe_sha256']:raise ValueError('Different Pelican caller fixture')
    candidate=probe['engine_candidate'];engine=candidate['source'];engine_before=(source/engine).read_text()
    if sha(source/engine)!=candidate['before_sha256']or engine_before.count(candidate['before'])!=1:
        raise ValueError('Qualified native readback seam drifted')
    engine_after=engine_before.replace(candidate['before'],candidate['after'])
    if hashlib.sha256(engine_after.encode()).hexdigest()!=candidate['after_sha256']:raise ValueError('Different native readback correction')
    helper='tests/helpers/pelican_children_constructor_tu15.hpp'
    if sha(ROOT/helper)!=probe['helper_sha256']or(source/helper).exists():raise ValueError('Qualified constructor helper drifted')
    args.output=args.output.resolve();args.cohort_output=args.cohort_output.resolve()
    if args.output.exists()or args.cohort_output.exists():raise ValueError('Refresh output already exists')
    receipt=args.cohort_output/'registration.json'
    cmake_before=(source/'CMakeLists.txt').read_text()
    cmake=cmake_before.replace(str(old_registration),str(receipt)).replace(str(old_registration.parent),str(args.cohort_output))
    if cmake==cmake_before:raise ValueError('Full cohort receipt command absent')
    pelican_map=proof['map'];security=build/'online-protocol/Release/libhalo2_online_security_ffi.so'
    addition=f'''\n# BEGIN qualified Pelican constructor and source player readback regression
add_test(NAME native_pelican_children_constructor COMMAND "{sys.executable}" "{source/'tools/run_test.py'}" -- $<TARGET_FILE:destruction_runtime_tu15_tests> "{pelican_map}" "{Path(pelican_map).parent}" campaign_pelican_children "${{CMAKE_BINARY_DIR}}/pelican-children.json")
set_tests_properties(native_pelican_children_constructor PROPERTIES TIMEOUT 360 FIXTURES_SETUP native_material_full_cohort ENVIRONMENT "HALO2_DESTRUCTION_FIXED_CLOCK=1;HALO2_DESTRUCTION_CONTINUATION=1;H2_SECURITY_LIBRARY={security}")
# END qualified Pelican constructor and source player readback regression
'''
    cmake+=addition
    args.output.mkdir(parents=True);args.cohort_output.mkdir(parents=True)
    report=copy.deepcopy(baseline)
    report.update(passed=False,status='refreshing',source_unchanged=False,complete_goal_acceptance=False,
        baseline=dict(path=str(args.baseline.resolve()),sha256=sha(args.baseline)),
        constructor_probe=dict(path=str(args.constructor_probe.resolve()),sha256=sha(args.constructor_probe)),
        runtime_input_reference=dict(path=str(args.runtime_inputs.resolve()),sha256=sha(args.runtime_inputs),scope='Map hashes only; refreshed CTest registration recorded separately'),
        tool_sha256=sha(__file__),full_cohort_registration=str(receipt),retained_large_artifacts=[],refresh_changes={})
    def save():(args.output/'admission.json').write_text(json.dumps(report,indent=2)+'\n')
    save()
    for name,text in ((fixture,fixture_after),(engine,engine_after),(helper,(ROOT/helper).read_text())):
        path=source/name;before=sha(path)if path.exists()else None;path.write_text(text)
        report['refresh_changes'][name]=dict(before=before,after=sha(path))
        if name in report['changes']:report['changes'][name]['after']=sha(path)
        else:report['changes'][name]=dict(before=before,after=sha(path))
    (source/'CMakeLists.txt').write_text(cmake)
    new_registration=copy.deepcopy(registration)
    new_registration.update(passed=False,status='prepared',validation_not_run=True,cmake_after=sha(source/'CMakeLists.txt'),
        cmake_addition=registration['cmake_addition'].replace(str(old_registration),str(receipt)).replace(str(old_registration.parent),str(args.cohort_output))+addition,
        prior_cohort_sha256=sha(old_registration.parent/'summary.json'),generated_registration_sha256=None,
        generated_fixture_expansion_verified=False,expected_selected_tests=sorted(set(registration['expanded_selected_tests'])|{'native_pelican_children_constructor'}),
        scope='Full30 plus the qualified Pelican constructor/death/respawn caller; one native player readback correction',
        additional_case='native_pelican_children_constructor')
    new_registration.pop('expanded_selected_tests',None)
    receipt.write_text(json.dumps(new_registration,indent=2)+'\n')
    report['retired_prior_generated_reports']=[]
    for name in [f'destruction-{name}.json'for name in registration['cases']]+['pelican-children.json']:
        path=build/name
        if path.is_symlink()or path.resolve().parent!=build:raise ValueError('Prior report escaped the native build')
        if path.exists():
            report['retired_prior_generated_reports'].append(dict(path=str(path),sha256=sha(path),bytes=path.stat().st_size))
            path.unlink()
    report['source_cmake_sha256']=new_registration['cmake_after'];report['status']='running'
    report['refresh_overlay']=dict(cmake_before_sha256=hashlib.sha256(cmake_before.encode()).hexdigest(),
        cmake_after_sha256=new_registration['cmake_after'],caller_substitutions=probe['diagnostic_caller_substitutions'],
        engine_correction=candidate,helper_source=(ROOT/helper).read_text(),source_refreshed_with_fresh_mtimes=True)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','destruction_runtime_tu15_tests','--target','halo2_native','--target','physics_multiplayer_acceptance',
        '--filter','^destruction_mcc_(ghost|campaign_guntower_no_damage|wraith_minigun|campaign_flood_clump)$',
        '--jobs','2','--test-jobs','1','--output',str(args.output/'validation')]
    for name in baseline['cases']:command+=['--retain-report',f'destruction-{name}.json']
    command+=['--retain-report','pelican-children.json'];report['command']=command;save()
    print(json.dumps(dict(status='qualified_readback_refresh_running',owner=str(owner))),flush=True)
    code=subprocess.run(command,env=dict(os.environ,HALO2_DESTRUCTION_FIXED_CLOCK='1',HALO2_DESTRUCTION_CONTINUATION='1',
        HALO2_DESTRUCTION_REQUIRE_NATIVE_MODEL='1',HALO2_DESTRUCTION_REQUIRE_NATIVE_MATERIAL='1'),
        stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    selected=subprocess.run(['ctest','--test-dir',str(build),'--show-only=json-v1','--tests-regex',
        '^destruction_mcc_(ghost|campaign_guntower_no_damage|wraith_minigun|campaign_flood_clump)$'],capture_output=True,text=True)
    names=sorted(row['name']for row in json.loads(selected.stdout)['tests'])if selected.returncode==0 else[]
    expansion=dict(passed=names==new_registration['expected_selected_tests'],selected_tests=names,registration_sha256=sha(receipt))
    (args.cohort_output/'expansion.json').write_text(json.dumps(expansion,indent=2)+'\n')
    cohort=args.cohort_output/'summary.json';constructor=args.output/'validation/evidence/pelican-children.json'
    report.update(validation_returncode=code,source_unchanged=all(sha(source/name)==row['after']for name,row in report['changes'].items())and
        sha(source/'CMakeLists.txt')==new_registration['cmake_after']and all(sha(source/name)==row['sha256']for name,row in report['sdk_references'].items()),
        full_cohort_passed=cohort.exists()and json.loads(cohort.read_text()).get('passed')is True,
        constructor_passed=constructor.exists()and json.loads(constructor.read_text()).get('source_death_and_respawn_readback_qualified')is True,
        runtime_inputs_unchanged=all(sha(name)==row['sha256']for name,row in runtime_inputs['files'].items()),
        fixture_expansion_passed=expansion['passed'])
    report['passed']=code==0 and all(report[k]for k in ('source_unchanged','full_cohort_passed','constructor_passed','fixture_expansion_passed','runtime_inputs_unchanged'))
    report['status']='passed'if report['passed']else'failed';save()
    print(json.dumps(dict(passed=report['passed'],report=str(args.output/'admission.json'))),flush=True)
    return 0 if report['passed']else code or 1


if __name__=='__main__':raise SystemExit(main())
