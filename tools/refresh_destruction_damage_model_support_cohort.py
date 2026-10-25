#!/usr/bin/env python3
"""Route the live damage graph's identical lookup through the qualified kernel."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
from native_damage_model_kernel_tu15 import install_actual_model_lookup,strip_actual_model_lookup
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
    if any(Path(row['scratch']).resolve()!=scratch for row in (cohort,prior)):raise ValueError('Different native build owner')
    base=ROOT/'analysis/destruction-goal-20261022'
    controls=json.loads((base/'model-lookup-original-controls-v1/report.json').read_text())
    negative=json.loads((base/'model-lookup-original-negative-v1/report.json').read_text())
    qualified=json.loads((base/'model-lookup-native-generation-v1.json').read_text())
    generation=json.loads((base/'model-lookup-support-generation-v1.json').read_text())
    if not controls['passed']or not negative['passed']or not negative['comparison_rejected_mutation']:
        raise ValueError('Native lookup original controls are unqualified')
    kernel='src/gameplay/objects/source_damage_model_native_tu15_internal.hpp'
    cpp='src/physics/physics_live_support_tu15_generated.cpp'
    if sha(ROOT/kernel)!=qualified['kernel_sha256']or sha(source/kernel)!=qualified['kernel_sha256']:
        raise ValueError('Shared qualified native kernel changed')
    if sha(ROOT/cpp)!=generation['generated_cpp_sha256']:raise ValueError('Production support routing changed after generation')
    red=json.loads((args.previous_refresh.parent/'validation/evidence/destruction-campaign_guntower_no_damage.json').read_text())
    if red.get('error')!='Negative result has no actual full-target dispatcher/model-lookup rejection' or not red.get('actual_target_damage_requests'):
        raise ValueError('Actual direct damage has no scoped duplicate-lookup failure')
    if any(event.get('caller_return')==0xda180 for event in red.get('actual_target_model_rejections',[])):
        raise ValueError('Failure does not isolate the still-literal live damage lookup')
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False);receipt=args.output/'admission.json'
    report=dict(passed=False,scratch=str(scratch),prior_cohort=str(args.cohort.resolve()),previous_refresh=str(args.previous_refresh.resolve()),
        patches=dict(prior['patches']),tool_sha256=sha(__file__),retained_large_artifacts=[],
        scope='Shared original-qualified D80A0 kernel in the actual damage graph, target negative and positive controls only; full goal open')
    def save():receipt.write_text(json.dumps(report,indent=2)+'\n')
    manifest=json.loads((args.cohort.parent/'source/source-manifest.json').read_text())
    expected=dict(manifest['source_files_sha256']);expected.update(cohort['additional_frozen_inputs'])
    expected.update({name:value['after']for name,value in prior['patches'].items()});expected['CMakeLists.txt']=prior['registered_cmake_sha256']
    if cohort.get('private_build_partition'):
        part=cohort['private_build_partition'];expected[part['path']]=part['after']
    report['frozen_source_drift_before_refresh']=[name for name,digest in expected.items()if sha(source/name)!=digest];save()
    if report['frozen_source_drift_before_refresh']:raise ValueError('Frozen source drift before shared lookup routing')
    target=source/cpp;before_text=target.read_text();before=sha(target)
    after_text=install_actual_model_lookup(before_text)
    if strip_actual_model_lookup(after_text)!=before_text:raise ValueError('Support conversion changed another original instruction')
    target.write_text(after_text);report['patches'][cpp]=dict(before=before,after=sha(target))
    report['all_previous_support_literal_bodies_byte_identical']=True
    report['workspace_support_contains_unrelated_drift']=strip_actual_model_lookup((ROOT/cpp).read_text())!=before_text
    report['workspace_support_whole_file_copied']=False
    generator='tools/generate_physics_live_support_tu15.py';target=source/generator;text=target.read_text();before=sha(target)
    anchor="    temporary=a.output.with_name('.'+a.output.name+f'.{os.getpid()}.tmp')\n"
    addition="    generated=preamble+body_source+ending\n    if 0xd80a0 in ENTRIES:\n        from native_damage_model_kernel_tu15 import install_actual_model_lookup\n        generated=install_actual_model_lookup(generated)\n"
    if text.count(anchor)!=1 or text.count('temporary.write_text(preamble+body_source+ending)')!=1:
        raise ValueError('Frozen support generator native hook seam changed')
    target.write_text(text.replace(anchor,addition+anchor).replace('temporary.write_text(preamble+body_source+ending)','temporary.write_text(generated)'))
    report['patches'][generator]=dict(before=before,after=sha(target))
    fixture='tests/destruction_runtime_tu15_tests.cpp'
    for name in ('tools/native_damage_model_kernel_tu15.py',fixture):
        target=source/name;before=sha(target)if target.exists()else None;target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes((ROOT/name).read_bytes());report['patches'][name]=dict(before=before,after=sha(target))
    report['fixture_refresh']=dict(report['patches'][fixture],path=fixture)
    report['registered_cmake_sha256']=report['cmake_sha256']=sha(source/'CMakeLists.txt')
    report['registered_cases']=prior['registered_cases'];report['cases']=['campaign_guntower_no_damage','ghost','wraith_minigun','campaign_flood_clump']
    for case in report['cases']:(scratch/'build'/f'destruction-{case}.json').unlink(missing_ok=True)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','destruction_runtime_tu15_tests','--target','halo2_native','--target','physics_multiplayer_acceptance',
        '--filter','^destruction_mcc_(campaign_guntower_no_damage|ghost|wraith_minigun|campaign_flood_clump)$',
        '--jobs','8','--test-jobs','2','--output',str(args.output/'validation')]
    for case in report['cases']:command+=['--retain-report',f'destruction-{case}.json']
    report['command']=command;save();print(json.dumps(dict(status='validating',report=str(receipt))),flush=True)
    result=subprocess.run(command,env=dict(os.environ,HALO2_DESTRUCTION_FIXED_CLOCK='1',HALO2_DESTRUCTION_CONTINUATION='1',
        HALO2_DESTRUCTION_REQUIRE_NATIVE_MODEL='1'),stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report.update(passed=result==0,returncode=result);save()
    print(json.dumps(dict(passed=result==0,report=str(receipt))),flush=True);return result


if __name__=='__main__':raise SystemExit(main())
