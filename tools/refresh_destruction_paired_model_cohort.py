#!/usr/bin/env python3
"""Admit the two original-qualified paired model-lookup API routes."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
from native_damage_model_kernel_tu15 import install_paired_model_lookup,strip_paired_model_lookup
from refresh_destruction_current_cohort import sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cohort',required=True,type=Path)
    parser.add_argument('--previous-refresh',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use the finite tools/run_test.py owner')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=scratch/'source'
    cohort=json.loads(args.cohort.read_text());prior=json.loads(args.previous_refresh.read_text())
    if any(Path(row['scratch']).resolve()!=scratch for row in (cohort,prior))or not prior['passed']:
        raise ValueError('Previous accepted cohort has a different owner or failed')
    base=ROOT/'analysis/destruction-goal-20261022'
    controls=json.loads((base/'model-lookup-paired-original-controls-v2/report.json').read_text())
    negative=json.loads((base/'model-lookup-paired-original-negative-v2/report.json').read_text())
    closure=json.loads((base/'model-lookup-paired-header-closure-v1.json').read_text())
    if not controls['passed']or controls['total_route_cases']!=192 or not negative['passed']or not negative['comparison_rejected_mutation']or not closure['passed']:
        raise ValueError('Paired routes require192 original comparisons, rejected mutations and exact header closure')
    pins=json.loads((base/'model-lookup-paired-original-controls-v2/source-hashes.json').read_text())
    names=['src/gameplay/units/source_direct_damage_runtime_generated.cpp','src/gameplay/units/source_vitality_runtime_generated.cpp']
    if any(sha(ROOT/name)!=pins[name]for name in names):raise ValueError('Qualified paired source changed')
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False);receipt=args.output/'admission.json'
    report=dict(passed=False,scratch=str(scratch),prior_cohort=str(args.cohort.resolve()),previous_refresh=str(args.previous_refresh.resolve()),
        patches=dict(prior['patches']),tool_sha256=sha(__file__),retained_large_artifacts=[],
        scope='Paired TU1.5 model lookup routes with canonical observation ABI;192 original comparisons and four live controls; full goal open')
    def save():receipt.write_text(json.dumps(report,indent=2)+'\n')
    manifest=json.loads((args.cohort.parent/'source/source-manifest.json').read_text())
    expected=dict(manifest['source_files_sha256']);expected.update(cohort['additional_frozen_inputs'])
    expected.update({name:value['after']for name,value in prior['patches'].items()});expected['CMakeLists.txt']=prior['registered_cmake_sha256']
    if cohort.get('private_build_partition'):
        part=cohort['private_build_partition'];expected[part['path']]=part['after']
    report['frozen_source_drift_before_refresh']=[name for name,digest in expected.items()if sha(source/name)!=digest];save()
    if report['frozen_source_drift_before_refresh']:raise ValueError('Frozen source drift before paired model admission')
    for name in names:
        target=source/name;before=target.read_text();before_hash=sha(target)
        output=install_paired_model_lookup(before)
        if strip_paired_model_lookup(output)!=before or output!=(ROOT/name).read_text():
            raise ValueError('Paired routing changed another original body or older-image path')
        target.write_text(output);report['patches'][name]=dict(before=before_hash,after=sha(target))
    report['all_existing_paired_literal_bodies_and_TU11_paths_byte_identical']=True
    for name in ['tools/native_damage_model_kernel_tu15.py']:
        target=source/name;before=sha(target);target.write_bytes((ROOT/name).read_bytes())
        report['patches'][name]=dict(before=before,after=sha(target))
    # Copy only verified generator additions; a concurrent edit to another
    # generator owner must not enter this frozen validation source.
    direct_addition='    from native_damage_model_kernel_tu15 import install_paired_model_lookup\n    a.output.write_text(install_paired_model_lookup(a.output.read_text()))\n'
    direct_meta="    manifest['native_model_lookup']=dict(entry='d80a0',canonical_observation_ABI_preserved=True,\n        original_execution_and_runtime_admission_required=True)\n"
    vitality_addition=direct_addition+"    manifest=json.loads(a.manifest.read_text());manifest['generated_cpp_sha256']=sha(a.output)\n    manifest['configuration_tool_sha256']=sha(Path(__file__))\n"+direct_meta+"    a.manifest.write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\\n')\n"
    for name,additions in [('tools/generate_source_direct_damage_runtime.py',(direct_addition,direct_meta)),
                            ('tools/generate_source_vitality_runtime.py',(vitality_addition,))]:
        target=source/name;text=(ROOT/name).read_text();literal=text
        for addition in additions:
            if literal.count(addition)!=1:raise ValueError('Paired native generator hook changed')
            literal=literal.replace(addition,'')
        if literal!=target.read_text():raise ValueError('Paired generator contains unrelated drift')
        before=sha(target);target.write_text(text);report['patches'][name]=dict(before=before,after=sha(target))
    fixture='tests/destruction_runtime_tu15_tests.cpp'
    report['fixture_refresh']=dict(path=fixture,before=sha(source/fixture),after=sha(source/fixture))
    report['registered_cmake_sha256']=report['cmake_sha256']=sha(source/'CMakeLists.txt')
    report['registered_cases']=prior['registered_cases'];report['cases']=['ghost','campaign_guntower_no_damage','wraith_minigun','campaign_flood_clump']
    for case in report['cases']:(scratch/'build'/f'destruction-{case}.json').unlink(missing_ok=True)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','destruction_runtime_tu15_tests','--target','halo2_native','--target','physics_multiplayer_acceptance',
        '--filter','^destruction_mcc_(ghost|campaign_guntower_no_damage|wraith_minigun|campaign_flood_clump)$',
        '--jobs','8','--test-jobs','2','--output',str(args.output/'validation')]
    for case in report['cases']:command+=['--retain-report',f'destruction-{case}.json']
    report['command']=command;save();print(json.dumps(dict(status='validating',report=str(receipt))),flush=True)
    result=subprocess.run(command,env=dict(os.environ,HALO2_DESTRUCTION_FIXED_CLOCK='1',HALO2_DESTRUCTION_CONTINUATION='1',
        HALO2_DESTRUCTION_REQUIRE_NATIVE_MODEL='1'),stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report.update(passed=result==0,returncode=result);save()
    print(json.dumps(dict(passed=result==0,report=str(receipt))),flush=True);return result


if __name__=='__main__':raise SystemExit(main())
