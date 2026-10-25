#!/usr/bin/env python3
"""Bind the complete native 105460 kernel after its actual minigun failure."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
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
    if any(Path(r['scratch']).resolve()!=scratch for r in (cohort,prior)):raise ValueError('Different native build owner')
    red=args.previous_refresh.parent/'validation/evidence/destruction-wraith_minigun.json'
    if 'missing at db9d3 -> 105460 return dc928' not in json.loads(red.read_text()).get('error',''):
        raise ValueError('Native indexed-record kernel lacks its direct live failure')
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False);receipt=args.output/'admission.json'
    report=dict(passed=False,scratch=str(scratch),prior_cohort=str(args.cohort.resolve()),previous_refresh=str(args.previous_refresh.resolve()),
        patches=dict(prior['patches']),tool_sha256=sha(__file__),retained_large_artifacts=[],
        scope='Whole native indexed-record owner, actual live callback and bounded destruction; full goal remains open')
    def save():receipt.write_text(json.dumps(report,indent=2)+'\n')
    manifest=json.loads((args.cohort.parent/'source/source-manifest.json').read_text())
    expected=dict(manifest['source_files_sha256']);expected.update(cohort['additional_frozen_inputs'])
    expected.update({p:v['after'] for p,v in prior['patches'].items()});expected['CMakeLists.txt']=prior['registered_cmake_sha256']
    if cohort.get('private_build_partition'):
        p=cohort['private_build_partition'];expected[p['path']]=p['after']
    report['frozen_source_drift_before_refresh']=[p for p,h in expected.items() if sha(source/p)!=h];save()
    if report['frozen_source_drift_before_refresh']:raise ValueError('Frozen source drift before indexed-record admission')
    generation=ROOT/'analysis/destruction-goal-20261022/minigun-damage-generation-v2/generation.json'
    evidence=json.loads(generation.read_text());cpp='src/gameplay/objects/source_damage_receiver_tu15_generated.cpp'
    kernel='src/gameplay/objects/source_damage_record_native_tu15_internal.hpp'
    if sha(ROOT/cpp)!=evidence['generated_cpp_sha256'] or sha(ROOT/kernel)!=evidence['structured_native_record_owner']['kernel_sha256']:
        raise ValueError('Native indexed-record source differs from its generation')
    preserved=json.loads((ROOT/'analysis/destruction-goal-20261022/minigun-damage-body-preservation-v1.json').read_text())
    if not preserved['passed'] or sha(source/cpp)!=preserved['before_sha256']:raise ValueError('Prior receiver module changed')
    report['generation']=dict(path=str(generation),sha256=sha(generation),native_entry='105460',complete_original_instructions=23)
    report['prior145_original_instruction_bodies_byte_identical']=True
    fixture='tests/destruction_runtime_tu15_tests.cpp'
    for name in (cpp,kernel,'include/halo2/source_damage_record_native_tu15.hpp','tools/generate_damage_receiver_tu15.py',fixture):
        target=source/name;before=sha(target) if target.exists() else None
        target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes((ROOT/name).read_bytes())
        report['patches'][name]=dict(before=before,after=sha(target))
    report['registered_cmake_sha256']=report['cmake_sha256']=sha(source/'CMakeLists.txt')
    report['registered_cases']=prior['registered_cases'];report['cases']=['ghost','wraith_minigun','campaign_sentinel_emitter']
    report['fixture_refresh']=dict(report['patches'][fixture],path=fixture)
    for case in report['cases']:(scratch/'build'/f'destruction-{case}.json').unlink(missing_ok=True)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','destruction_runtime_tu15_tests','--target','halo2_native','--target','physics_multiplayer_acceptance',
        '--filter','^destruction_mcc_(ghost|wraith_minigun|campaign_sentinel_emitter)$','--jobs','8','--test-jobs','1',
        '--output',str(args.output/'validation')]
    for case in report['cases']:command+=['--retain-report',f'destruction-{case}.json']
    report['command']=command;save();print(json.dumps(dict(status='validating',report=str(receipt))),flush=True)
    code=subprocess.run(command,env=dict(os.environ,HALO2_DESTRUCTION_FIXED_CLOCK='1',HALO2_DESTRUCTION_CONTINUATION='1'),
        stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report.update(passed=code==0,returncode=code);save();print(json.dumps(dict(passed=code==0,report=str(receipt))),flush=True);return code


if __name__=='__main__':raise SystemExit(main())
