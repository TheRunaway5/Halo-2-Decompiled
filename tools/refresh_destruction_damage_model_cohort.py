#!/usr/bin/env python3
"""Admit the original-qualified native model lookup to the destruction cohort."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
from native_damage_model_kernel_tu15 import strip_native_model_lookup
from refresh_destruction_current_cohort import sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cohort',required=True,type=Path)
    parser.add_argument('--previous-refresh',required=True,type=Path)
    parser.add_argument('--generation',required=True,type=Path)
    parser.add_argument('--controls',required=True,type=Path)
    parser.add_argument('--negative',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use the finite tools/run_test.py owner')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=scratch/'source'
    cohort=json.loads(args.cohort.read_text());prior=json.loads(args.previous_refresh.read_text())
    if any(Path(row['scratch']).resolve()!=scratch for row in (cohort,prior)):raise ValueError('Different native build owner')
    if not prior.get('passed'):raise ValueError('Resolve the previous fixture regression before native model admission')
    generation=json.loads(args.generation.read_text());controls=json.loads(args.controls.read_text());negative=json.loads(args.negative.read_text())
    if not controls.get('passed')or not negative.get('passed')or not negative.get('comparison_rejected_mutation'):
        raise ValueError('Native lookup requires passing original controls and a rejected private mutation')
    cpp='src/gameplay/units/source_damage_delivery_runtime_generated.cpp'
    kernel='src/gameplay/objects/source_damage_model_native_tu15_internal.hpp'
    header='include/halo2/source_damage_model_native_tu15.hpp'
    pins=json.loads((args.controls.parent/'source-hashes.json').read_text())
    for name,key in ((cpp,'generated_cpp_sha256'),(kernel,'kernel_sha256'),(header,'activity_header_sha256')):
        if sha(ROOT/name)!=generation[key]or pins[name]!=generation[key]:raise ValueError('Native lookup source differs from original qualification')
    if strip_native_model_lookup((ROOT/cpp).read_text())!=(source/cpp).read_text():
        raise ValueError('Native lookup conversion changed the older-image fallback or another original body')
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False);receipt=args.output/'admission.json'
    report=dict(passed=False,scratch=str(scratch),prior_cohort=str(args.cohort.resolve()),previous_refresh=str(args.previous_refresh.resolve()),
        patches=dict(prior['patches']),tool_sha256=sha(__file__),retained_large_artifacts=[],
        scope='Whole native model lookup, unchanged fallback/other89 bodies and bounded destruction only; full goal remains open')
    def save():receipt.write_text(json.dumps(report,indent=2)+'\n')
    manifest=json.loads((args.cohort.parent/'source/source-manifest.json').read_text())
    expected=dict(manifest['source_files_sha256']);expected.update(cohort['additional_frozen_inputs'])
    expected.update({name:value['after']for name,value in prior['patches'].items()});expected['CMakeLists.txt']=prior['registered_cmake_sha256']
    if cohort.get('private_build_partition'):
        partition=cohort['private_build_partition'];expected[partition['path']]=partition['after']
    report['frozen_source_drift_before_refresh']=[name for name,digest in expected.items()if sha(source/name)!=digest];save()
    if report['frozen_source_drift_before_refresh']:raise ValueError('Frozen native source drift before model lookup admission')
    report['original_qualification']={name:dict(path=str(path.resolve()),sha256=sha(path))for name,path in
        (('generation',args.generation),('controls',args.controls),('negative',args.negative))}
    report['other89_bodies_and_older_image_fallback_byte_identical']=True
    fixture='tests/destruction_runtime_tu15_tests.cpp'
    for name in (cpp,kernel,header,'tools/native_damage_model_kernel_tu15.py','tools/fix_damage_delivery_observation_tu15.py',fixture):
        target=source/name;before=sha(target)if target.exists()else None
        target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes((ROOT/name).read_bytes())
        report['patches'][name]=dict(before=before,after=sha(target))
    report['registered_cmake_sha256']=report['cmake_sha256']=sha(source/'CMakeLists.txt')
    report['registered_cases']=prior['registered_cases'];report['cases']=['ghost','wraith_minigun','campaign_flood_clump']
    report['fixture_refresh']=dict(report['patches'][fixture],path=fixture)
    for case in report['cases']:(scratch/'build'/f'destruction-{case}.json').unlink(missing_ok=True)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','destruction_runtime_tu15_tests','--target','halo2_native','--target','physics_multiplayer_acceptance',
        '--filter','^destruction_mcc_(ghost|wraith_minigun|campaign_flood_clump)$','--jobs','8','--test-jobs','2',
        '--output',str(args.output/'validation')]
    for case in report['cases']:command+=['--retain-report',f'destruction-{case}.json']
    report['command']=command;save();print(json.dumps(dict(status='validating',report=str(receipt))),flush=True)
    result=subprocess.run(command,env=dict(os.environ,HALO2_DESTRUCTION_FIXED_CLOCK='1',HALO2_DESTRUCTION_CONTINUATION='1',
        HALO2_DESTRUCTION_REQUIRE_NATIVE_MODEL='1'),stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report.update(passed=result==0,returncode=result);save()
    print(json.dumps(dict(passed=result==0,report=str(receipt))),flush=True);return result


if __name__=='__main__':raise SystemExit(main())
