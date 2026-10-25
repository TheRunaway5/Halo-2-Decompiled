#!/usr/bin/env python3
"""Bind complete original E89B0/1E9290 owners in the active frozen native build."""
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
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use the active finite tools/run_test.py owner')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=scratch/'source'
    cohort=json.loads(args.cohort.read_text());prior=json.loads(args.previous_refresh.read_text())
    if any(Path(r['scratch']).resolve()!=scratch for r in (cohort,prior)):raise ValueError('Damage owner receipt belongs to another build')
    validation=args.previous_refresh.parent/'validation';worker=json.loads((validation/'report.json').read_text())
    if not all(s['exit_code']==0 for s in worker['steps'][:2]) or 'missing at d91af -> e89b0 return d91b4' not in (validation/'tests.log').read_text():
        raise ValueError('Damage owner lacks its direct native failure')
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False);receipt=args.output/'admission.json'
    report=dict(passed=False,scratch=str(scratch),prior_cohort=str(args.cohort.resolve()),previous_refresh=str(args.previous_refresh.resolve()),
        patches=dict(prior['patches']),tool_sha256=sha(__file__),retained_large_artifacts=[],
        scope='Original receiver callback binding and bounded live cases; semantic/full goal proof remains separate')
    def save():receipt.write_text(json.dumps(report,indent=2)+'\n')
    manifest=json.loads((args.cohort.parent/'source/source-manifest.json').read_text())
    expected=dict(manifest['source_files_sha256']);expected.update(cohort['additional_frozen_inputs'])
    expected.update({p:v['after'] for p,v in prior['patches'].items()});expected['CMakeLists.txt']=prior['registered_cmake_sha256']
    if cohort.get('private_build_partition'):
        p=cohort['private_build_partition'];expected[p['path']]=p['after']
    report['frozen_source_drift_before_refresh']=[p for p,h in expected.items() if sha(source/p)!=h];save()
    if report['frozen_source_drift_before_refresh']:raise ValueError('Frozen native source changed before damage admission')
    generation_path=ROOT/'analysis/destruction-goal-20261022/flood-damage-generation-v1/generation.json'
    generation=json.loads(generation_path.read_text());cpp='src/gameplay/objects/source_damage_receiver_tu15_generated.cpp'
    if sha(ROOT/cpp)!=generation['generated_cpp_sha256']:raise ValueError('Original damage generation changed')
    report['generation']=dict(path=str(generation_path),sha256=sha(generation_path),entries=['E89B0','1E9290'],complete_instruction_counts=[40,105])
    plan={}
    for name in ('include/halo2/source_damage_receiver_tu15.hpp',cpp,'tools/generate_damage_receiver_tu15.py'):
        if (source/name).exists():raise ValueError('Damage admission source already exists')
        plan[name]=(ROOT/name).read_bytes()
    relative='src/physics/physics_live_world_tu15.cpp';text=(source/relative).read_text()
    include='#include "halo2/source_collision_query_tu15.hpp"'
    branch='''        if(source_collision_query_tu15_owns_entry(c.target)) {
            source_run_collision_query_tu15(m,c.target,a,fp,services(),o);return;
        }'''
    if text.count(include)!=1 or text.count(branch)!=1:raise ValueError('Damage world binding seam changed')
    text=text.replace(include,include+'\n#include "halo2/source_damage_receiver_tu15.hpp"')
    text=text.replace(branch,branch+'''
        if(source_damage_receiver_tu15_owns_entry(c.target)) {
            source_run_damage_receiver_tu15(m,c.target,a,fp,services(),o);return;
        }''');plan[relative]=text.encode()
    fixture='tests/destruction_runtime_tu15_tests.cpp';plan[fixture]=(ROOT/fixture).read_bytes()
    cmake=source/'CMakeLists.txt';text=cmake.read_text();needle='include(cmake/PhysicsLiveWorldTu15.cmake)'
    if text.count(needle)!=1:raise ValueError('Damage source registration seam changed')
    text=text.replace(needle,needle+'\ntarget_sources(halo2_physics_world_runtime PRIVATE '+cpp+')')
    for name,data in plan.items():
        target=source/name;before=sha(target) if target.exists() else None;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
        report['patches'][name]=dict(before=before,after=sha(target))
    cmake.write_text(text);report['registered_cmake_sha256']=report['cmake_sha256']=sha(cmake)
    report['registered_cases']=prior['registered_cases'];report['cases']=['ghost','campaign_scorpion_cannon','campaign_flood_clump']
    report['fixture_refresh']=dict(report['patches'][fixture],path=fixture)
    for case in report['cases']:(scratch/'build'/f'destruction-{case}.json').unlink(missing_ok=True)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','destruction_runtime_tu15_tests','--target','halo2_native','--target','physics_multiplayer_acceptance',
        '--filter','^destruction_mcc_(ghost|campaign_scorpion_cannon|campaign_flood_clump)$','--jobs','8','--test-jobs','1',
        '--output',str(args.output/'validation')]
    for case in report['cases']:command+=['--retain-report',f'destruction-{case}.json']
    report['command']=command;save();print(json.dumps(dict(status='validating',report=str(receipt))),flush=True)
    code=subprocess.run(command,env=dict(os.environ,HALO2_DESTRUCTION_FIXED_CLOCK='1',HALO2_DESTRUCTION_CONTINUATION='1'),
        stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report.update(passed=code==0,returncode=code);save();print(json.dumps(dict(passed=code==0,returncode=code,report=str(receipt))),flush=True);return code


if __name__=='__main__':raise SystemExit(main())
