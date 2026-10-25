#!/usr/bin/env python3
"""Admit the qualified DC540 owner and Enforcer caller in the finite cohort."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from refresh_destruction_pelican_parent_tu15 import sha,FILTER,ATTACHMENTS

ROOT=Path(__file__).resolve().parents[1]
CASE='campaign_sentinel_enforcer'
PRIOR_EXTRA=(*ATTACHMENTS,'campaign_pelican_parent')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline',required=True,type=Path)
    parser.add_argument('--native-probe',required=True,type=Path)
    parser.add_argument('--original-probe',required=True,type=Path)
    parser.add_argument('--runtime-inputs',required=True,type=Path)
    parser.add_argument('--cohort-output',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use the same finite tools/run_test.py owner')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source';build=owner/'build'
    baseline=json.loads(args.baseline.read_text());probe=json.loads(args.native_probe.read_text());original=json.loads(args.original_probe.read_text())
    old_registration=Path(baseline['full_cohort_registration']).resolve();registration=json.loads(old_registration.read_text())
    candidate=probe.get('engine_candidate')
    if not baseline['passed']or not baseline['source_unchanged']or not probe['passed']or not original['passed']or not candidate:
        raise ValueError('Native cohort, Enforcer or actual original qualification failed')
    if any(Path(row.get('owner',row.get('native_owner'))).resolve()!=owner for row in (baseline,probe,original)):
        raise ValueError('Different native owner')
    if original['engine_candidate']['source_sha256']!=candidate['source_sha256']or original['engine_candidate']['after_sha256']!=candidate['after_sha256']:
        raise ValueError('Actual original comparison used a different candidate')
    if not json.loads((old_registration.parent/'summary.json').read_text())['passed']:raise ValueError('Existing complete cohort failed')
    if any(sha(source/name)!=row['after']for name,row in baseline['changes'].items()):raise ValueError('Baseline source drifted')
    if sha(source/'CMakeLists.txt')!=registration['cmake_after']:raise ValueError('Baseline registration drifted')
    if any(sha(name)!=pin for name,pin in probe['native_libraries_sha256'].items())or any(sha(name)!=pin for name,pin in probe['native_headers_sha256'].items()):
        raise ValueError('Qualified candidate libraries or headers drifted')
    runtime=json.loads(args.runtime_inputs.read_text())
    if not runtime['passed']or any(sha(name)!=row['sha256']for name,row in runtime['files'].items()):raise ValueError('MCC runtime input drifted')
    fixture='tests/destruction_runtime_tu15_tests.cpp';before=(source/fixture).read_text();after=before
    if sha(source/fixture)!=probe['fixture_before_sha256']:raise ValueError('Different Enforcer caller baseline')
    for needle,replacement in probe['caller_substitutions']:
        if after.count(needle)!=1:raise ValueError('Enforcer caller seam drifted')
        after=after.replace(needle,replacement)
    if hashlib.sha256(after.encode()).hexdigest()!=probe['fixture_after_sha256']:raise ValueError('Different qualified Enforcer caller')
    world=source/candidate['source'];world_before=world.read_text()
    include='#include "halo2/physics_live_world_tu15.hpp"\n';anchor='        if(source_damage_receiver_tu15_owns_entry(c.target)) {\n'
    if sha(world)!=candidate['before_sha256']or world_before.count(include)!=1 or world_before.count(anchor)!=1:
        raise ValueError('Native world baseline drifted')
    world_after=world_before.replace(include,include+'#include "halo2/source_damage_model_children_tu15.hpp"\n').replace(anchor,candidate['world_addition']+anchor)
    if hashlib.sha256(world_after.encode()).hexdigest()!=candidate['after_sha256']:raise ValueError('Different native world route')
    plan={name:(ROOT/name).read_bytes()for name in candidate['source_sha256']}
    if any(hashlib.sha256(data).hexdigest()!=candidate['source_sha256'][name]for name,data in plan.items()):raise ValueError('Qualified native child owner drifted')
    module='cmake/DamageModelChildrenTu15.cmake';plan[module]=(ROOT/module).read_bytes()
    if any((source/name).exists()for name in plan):raise ValueError('Native child owner already staged')
    proof=json.loads((args.native_probe.parent/'validation/evidence/vehicle.json').read_text());map_path=Path(proof['map']).resolve(strict=True)
    if str(map_path)not in runtime['files']or sha(map_path)!=runtime['files'][str(map_path)]['sha256']:raise ValueError('Enforcer map outside qualified inputs')
    args.output=args.output.resolve();args.cohort_output=args.cohort_output.resolve()
    if args.output.exists()or args.cohort_output.exists():raise ValueError('Refresh output already exists')
    receipt=args.cohort_output/'registration.json';cmake_before=(source/'CMakeLists.txt').read_text()
    cmake=cmake_before.replace(str(old_registration),str(receipt)).replace(str(old_registration.parent),str(args.cohort_output))
    module_include='\ninclude("${CMAKE_CURRENT_SOURCE_DIR}/cmake/DamageModelChildrenTu15.cmake")\n';cmake+=module_include
    security=build/'online-protocol/Release/libhalo2_online_security_ffi.so'
    addition='\n# BEGIN qualified campaign Enforcer and native model-child owner\n'
    addition+=f'add_test(NAME destruction_mcc_{CASE} COMMAND "{sys.executable}" "{source/"tools/run_test.py"}" -- $<TARGET_FILE:destruction_runtime_tu15_tests> "{map_path}" "{map_path.parent}" {CASE} "${{CMAKE_BINARY_DIR}}/destruction-{CASE}.json")\n'
    addition+=f'set_tests_properties(destruction_mcc_{CASE} PROPERTIES TIMEOUT 360 FIXTURES_SETUP native_material_full_cohort ENVIRONMENT "HALO2_DESTRUCTION_FIXED_CLOCK=1;HALO2_DESTRUCTION_CONTINUATION=1;H2_SECURITY_LIBRARY={security}")\n'
    addition+='# END qualified campaign Enforcer and native model-child owner\n';cmake+=addition
    args.output.mkdir(parents=True);args.cohort_output.mkdir(parents=True)
    report=copy.deepcopy(baseline)
    report.update(passed=False,status='refreshing',source_unchanged=False,validation_returncode=None,
        baseline=dict(path=str(args.baseline.resolve()),sha256=sha(args.baseline)),
        native_qualification=dict(path=str(args.native_probe.resolve()),sha256=sha(args.native_probe)),
        original_qualification=dict(path=str(args.original_probe.resolve()),sha256=sha(args.original_probe)),
        full_cohort_registration=str(receipt),tool_sha256=sha(__file__),retained_large_artifacts=[],new_campaign_case=CASE,
        source_refresh=dict(fixture_before_sha256=sha(source/fixture),fixture_after_sha256=probe['fixture_after_sha256'],
            caller_substitutions=probe['caller_substitutions'],native_candidate=candidate,new_sources={name:data.decode()for name,data in plan.items()},
            native_module_cmake_include=module_include,fresh_source_mtime=True))
    def save():(args.output/'admission.json').write_text(json.dumps(report,indent=2)+'\n')
    save();(source/fixture).write_text(after);world.write_text(world_after)
    report['changes'][fixture]['after']=sha(source/fixture)
    report['changes'][candidate['source']]=dict(before=candidate['before_sha256'],after=sha(world))
    for name,data in plan.items():
        target=source/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
        report['changes'][name]=dict(before=None,after=sha(target))
    (source/'CMakeLists.txt').write_text(cmake)
    new_reg=copy.deepcopy(registration)
    new_reg.update(status='prepared',passed=False,validation_not_run=True,cmake_after=sha(source/'CMakeLists.txt'),
        cmake_addition=registration['cmake_addition'].replace(str(old_registration),str(receipt)).replace(str(old_registration.parent),str(args.cohort_output))+module_include+addition,
        prior_cohort_sha256=sha(old_registration.parent/'summary.json'),expected_selected_tests=sorted(set(registration['expected_selected_tests'])|{'destruction_mcc_'+CASE}),additional_campaign_case=CASE)
    receipt.write_text(json.dumps(new_reg,indent=2)+'\n')
    report['retired_prior_generated_reports']=[]
    for name in [f'destruction-{case}.json'for case in registration['cases']+list(PRIOR_EXTRA)+[CASE]]+['pelican-children.json']:
        path=build/name
        if path.is_symlink()or path.resolve().parent!=build:raise ValueError('Report escaped the owned build')
        if path.exists():
            report['retired_prior_generated_reports'].append(dict(path=str(path),sha256=sha(path),bytes=path.stat().st_size));path.unlink()
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','destruction_runtime_tu15_tests','--target','halo2_native','--target','physics_multiplayer_acceptance',
        '--filter',FILTER,'--jobs','2','--test-jobs','1','--output',str(args.output/'validation')]
    for case in baseline['cases']+list(PRIOR_EXTRA)+[CASE]:command+=['--retain-report',f'destruction-{case}.json']
    command+=['--retain-report','pelican-children.json'];report.update(status='running',command=command,source_cmake_sha256=new_reg['cmake_after']);save()
    print(json.dumps(dict(status='expanded_native_campaign_cohort_running',expected_CTests=len(new_reg['expected_selected_tests']),owner=str(owner))),flush=True)
    code=subprocess.run(command,env=dict(os.environ,HALO2_DESTRUCTION_FIXED_CLOCK='1',HALO2_DESTRUCTION_CONTINUATION='1',
        HALO2_DESTRUCTION_REQUIRE_NATIVE_MODEL='1',HALO2_DESTRUCTION_REQUIRE_NATIVE_MATERIAL='1'),stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    selected=subprocess.run(['ctest','--test-dir',str(build),'--show-only=json-v1','--tests-regex',FILTER],capture_output=True,text=True)
    names=sorted(row['name']for row in json.loads(selected.stdout)['tests'])if selected.returncode==0 else[]
    expansion=dict(passed=names==new_reg['expected_selected_tests'],selected_tests=names,registration_sha256=sha(receipt));(args.cohort_output/'expansion.json').write_text(json.dumps(expansion,indent=2)+'\n')
    evidence=args.output/'validation/evidence';path=evidence/f'destruction-{CASE}.json';data=json.loads(path.read_text())if path.exists()else{}
    report.update(validation_returncode=code,fixture_expansion_passed=expansion['passed'],
        source_unchanged=all(sha(source/name)==row['after']for name,row in report['changes'].items())and sha(source/'CMakeLists.txt')==new_reg['cmake_after']and
            all(sha(source/name)==row['sha256']for name,row in report['sdk_references'].items()),
        runtime_inputs_unchanged=all(sha(name)==row['sha256']for name,row in runtime['files'].items()),
        new_campaign_case_passed=data.get('passed')is True and data.get('source_dead')is True and data.get('native_model_children_activity',0)>0 and
            data.get('post_destruction_continuation',{}).get('complete_encoded_world_sha256_comparisons')==16,new_campaign_case_error=data.get('error'))
    summary=args.cohort_output/'summary.json';report['full_existing_cohort_passed']=summary.exists()and json.loads(summary.read_text()).get('passed')is True
    report['prior_extras_passed']=all((evidence/f'destruction-{case}.json').exists()and json.loads((evidence/f'destruction-{case}.json').read_text()).get('passed')is True for case in PRIOR_EXTRA)
    ctor=evidence/'pelican-children.json';report['constructor_passed']=ctor.exists()and json.loads(ctor.read_text()).get('source_death_and_respawn_readback_qualified')is True
    report['passed']=code==0 and all(report[k]for k in ('fixture_expansion_passed','source_unchanged','runtime_inputs_unchanged','full_existing_cohort_passed','constructor_passed','prior_extras_passed','new_campaign_case_passed'))
    report['status']='passed'if report['passed']else'failed';save();print(json.dumps(dict(passed=report['passed'],report=str(args.output/'admission.json'))),flush=True)
    return 0 if report['passed']else code or 1


if __name__=='__main__':raise SystemExit(main())
