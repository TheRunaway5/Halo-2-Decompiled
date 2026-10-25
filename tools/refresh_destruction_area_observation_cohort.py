#!/usr/bin/env python3
"""Admit read-only original area-entry diagnostics in the finite native cohort."""
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
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False);receipt=args.output/'admission.json'
    report=dict(passed=False,scratch=str(scratch),prior_cohort=str(args.cohort.resolve()),
        previous_refresh=str(args.previous_refresh.resolve()),patches=dict(prior['patches']),tool_sha256=sha(__file__),
        retained_large_artifacts=[],scope='Read-only D91D0 entry ABI; original instruction bodies and gameplay inputs unchanged')
    def save():receipt.write_text(json.dumps(report,indent=2)+'\n')
    manifest=json.loads((args.cohort.parent/'source/source-manifest.json').read_text())
    expected=dict(manifest['source_files_sha256']);expected.update(cohort['additional_frozen_inputs'])
    expected.update({p:v['after'] for p,v in prior['patches'].items()});expected['CMakeLists.txt']=prior['registered_cmake_sha256']
    if cohort.get('private_build_partition'):
        p=cohort['private_build_partition'];expected[p['path']]=p['after']
    report['frozen_source_drift_before_refresh']=[p for p,h in expected.items() if sha(source/p)!=h];save()
    if report['frozen_source_drift_before_refresh']:raise ValueError('Frozen source drift before diagnostic admission')
    name='src/physics/physics_live_support_tu15_generated.cpp';original=(source/name).read_text()
    include='#include "halo2/physics_live_support_tu15.hpp"'
    entry='static void run_d91d0(WorldState &s) {'
    diagnostic='SourcePhysicsEntryObservation area_diagnostic(s.memory,0xd91d0u,[&]{return s.save();});'
    if original.count(include)!=1 or original.count(entry)!=1 or diagnostic in original:raise ValueError('Area diagnostic seam changed')
    changed=original.replace(include,include+'\n#include "halo2/source_physics_entry_observation.hpp"')
    changed=changed.replace(entry,entry+'\n'+diagnostic)
    stripped=changed.replace('\n#include "halo2/source_physics_entry_observation.hpp"','').replace('\n'+diagnostic,'')
    if stripped!=original:raise ValueError('Diagnostic changed the original instruction body')
    report['original_instruction_bodies_byte_identical']=True
    plan={name:changed.encode()}
    name='tools/generate_physics_live_support_tu15.py';text=(source/name).read_text()
    needle="        lines=[f'static void run_{entry:x}(WorldState &s) {{']"
    body="    body_source='\\n\\n'.join(bodies)"
    if text.count(needle)!=1 or text.count(body)!=1:raise ValueError('Generator diagnostic seam changed')
    text=text.replace(needle,needle+"\n        if entry==0xd91d0:\n            lines.append('"+diagnostic+"')")
    text=text.replace(body,body+"\n    if 'SourcePhysicsEntryObservation area_diagnostic' in body_source:\n        preamble=preamble.replace('#include \\\"physics/runtime/physics_world_runtime_internal.hpp\\\"',\n            '#include \\\"physics/runtime/physics_world_runtime_internal.hpp\\\"\\n#include \\\"halo2/source_physics_entry_observation.hpp\\\"')")
    plan[name]=text.encode()
    fixture='tests/destruction_runtime_tu15_tests.cpp';plan[fixture]=(ROOT/fixture).read_bytes()
    for name,data in plan.items():
        target=source/name;before=sha(target);target.write_bytes(data)
        report['patches'][name]=dict(before=before,after=sha(target))
    report['registered_cmake_sha256']=report['cmake_sha256']=sha(source/'CMakeLists.txt')
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
    report.update(passed=code==0,returncode=code);save();print(json.dumps(dict(passed=code==0,report=str(receipt))),flush=True);return code


if __name__=='__main__':raise SystemExit(main())
