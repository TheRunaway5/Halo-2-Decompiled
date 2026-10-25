#!/usr/bin/env python3
"""Run the complete native vehicle cohort bound to its exact frozen inputs.

The enclosing run_test.py session owns source/build storage. run_tests.py owns
CMake validation and bounded reports; every source receipt is checked again
after the run so delivery cannot reuse an earlier source epoch's green result.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]
PRIMARY=('ghost','banshee','scorpion','wraith','warthog-mg','warthog-gauss',
         'spectre','h_turret_ap','c_turret_ap')
OWNERSHIP=('native_tu15_world_memory_ownership','native_tu15_physics_branch_ownership',
           'original_tu15_contrail_snapshot_inputs','original_tu15_light_fade_curves')


def source_digest(pins):
    return hashlib.sha256(json.dumps(pins,sort_keys=True,separators=(',',':')).encode()).hexdigest()


def native_model_lookup_routes(source):
    installer=source/'tools/native_damage_model_kernel_tu15.py'
    spec=importlib.util.spec_from_file_location('frozen_vehicle_model_routes',installer)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    routes={
        'src/gameplay/units/source_damage_delivery_runtime_generated.cpp':('delivery',module.install_native_model_lookup),
        'src/gameplay/units/source_direct_damage_runtime_generated.cpp':('direct',module.install_paired_model_lookup),
        'src/gameplay/units/source_vitality_runtime_generated.cpp':('vitality',module.install_paired_model_lookup),
        'src/physics/physics_live_support_tu15_generated.cpp':('live_support',module.install_actual_model_lookup),
    }
    rows=[]
    for name,(kind,install) in routes.items():
        text=(source/name).read_text()
        if install(text)!=text:raise ValueError('Required native model lookup route is absent: '+name)
        rows.append(dict(path=name,route=kind,sha256=sha(source/name),
            complete_authenticated_literal_body_preserved=True,native_route_installed=True))
    return dict(routes=rows,installer_sha256=sha(installer),
        kernel_sha256=sha(source/'src/gameplay/objects/source_damage_model_native_tu15_internal.hpp'),
        scope='Four TU1.5 lookup compositions; the separate MapPack11-only unit graph is not a TU1.5 route')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('prepared','cohort','refresh','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--jobs',type=int,default=5)
    parser.add_argument('--test-jobs',type=int,default=4)
    parser.add_argument('--ownership-only',action='store_true',
        help='Rebuild the native targets and run only four ownership regressions before rendered diagnosis')
    parser.add_argument('--reference-xbe',type=Path,default=Path.home()/
        '.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    prepared=json.loads(args.prepared.read_text());cohort=json.loads(args.cohort.read_text())
    refresh=json.loads(args.refresh.read_text());source=owner/'source'
    if (Path(prepared['runner_scratch'])!=owner or Path(cohort['owner'])!=owner or Path(refresh['owner'])!=owner):
        raise ValueError('Cohort requires the same authenticated private source/build owner')
    expected=dict(prepared['registered_snapshot_source_files_sha256'])
    for row in cohort['changes']:
        if expected[row['path']]!=row['before']:raise ValueError('Cohort preimage differs')
        expected[row['path']]=row['after']
    expected.update({'data/'+name:pin for name,pin in cohort['private_data_sha256'].items()})
    expected.update(refresh['refreshed_source_sha256'])
    drift=lambda:[name for name,pin in expected.items() if sha(source/name)!=pin]
    if drift():raise ValueError('Undeclared frozen source drift before validation')
    model_routes=native_model_lookup_routes(source)
    original_pin=sha(args.reference_xbe)
    if original_pin!='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd':
        raise ValueError('Original readonly contrail test data differs')
    receipts={str(path.resolve()):sha(path) for path in (args.prepared,args.cohort,args.refresh)}
    reports=[*('vehicle-variant-'+name+'.json' for name in refresh['registered_cases']),
             *('vehicle-destruction-'+name+'.json' for name in cohort['destruction_cases']),
             *('vehicle-weapon-'+name+'.json' for name in PRIMARY),
             'vehicle-secondary-banshee.json','vehicle-secondary-scorpion.json',
             'vehicle-boost-ghost.json','vehicle-bv-agent-original.json']
    if args.ownership_only:reports=[]
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--build-worker',
        '--source',str(source),'--output',str(args.output),'--jobs',str(args.jobs),
        '--test-jobs',str(args.test_jobs),'--filter',
        ('^('+'|'.join(OWNERSHIP)+')$' if args.ownership_only else
         '^(native_tu15_world_memory_ownership$|native_tu15_physics_branch_ownership$|original_tu15_contrail_snapshot_inputs$|original_tu15_light_fade_curves$|original_vehicle_bv_agent_tu15$|vehicle_goal_(weapon_|destruction_|variant_|secondary_|boost_))')]
    for name in ('native_tu15_world_memory_tests','native_tu15_physics_branch_ownership_tests',
                 'source_contrail_snapshot_tests','light_fade_curves_tu15_tests',
                 'halo2_native','physics_multiplayer_acceptance','destruction_runtime_tu15_tests',
                 'vehicle_weapon_hud_world_tests','vehicle_weapon_hud_runtime_tests',
                 'vehicle_variants_world_tu15_tests','vehicle_bv_agent_tu15_tests'):
        command+=['--target',name]
    for value in cohort['cmake_args']:command+=['--cmake-arg='+value]
    command+=['--cmake-arg=-DHALO2_TU15_CONTRAIL_XBE='+str(args.reference_xbe.resolve())]
    for name in reports:command+=['--retain-report',name]
    command+=['--retain-report','Testing/Temporary/LastTest.log']
    binding=dict(source_receipts_sha256=receipts,frozen_source_manifest_sha256=source_digest(expected),
        frozen_source_files=len(expected),ownership_tests=list(OWNERSHIP),
        native_model_lookup_routes=model_routes,
        readonly_reference_data=dict(path=str(args.reference_xbe.resolve()),sha256=original_pin,
            original_instruction_execution=False,native_game_XBE_argument=False),
        expected_cases=len(reports)+len(OWNERSHIP),source_unchanged_after_run=False,
        ownership_only=args.ownership_only,complete_declared_vehicle_cohort=not args.ownership_only,
        runtime_opcode_execution=False,full_vehicle_parity=False,tool_sha256=sha(Path(__file__)))
    (args.output/'source-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
    print(json.dumps(dict(status='running',expected_cases=binding['expected_cases'],output=str(args.output))),flush=True)
    result=subprocess.run(command,env=dict(os.environ,HALO2_TEST_TURRET_TARGET_DAMAGE='1'),
        stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT)
    changed=drift();unchanged=not changed and all(sha(Path(name))==pin for name,pin in receipts.items())
    path=args.output/'report.json';report=json.loads(path.read_text())
    match=re.search(r'^100% tests passed(?:, 0 tests failed)? out of (\d+)\s*$',
        (args.output/'tests.log').read_text(),re.MULTILINE)
    binding.update(source_unchanged_after_run=unchanged,source_drift=changed,
        declared_case_count_passed=bool(match and int(match[1])==binding['expected_cases']))
    report['vehicle_goal_source_binding']=binding
    report['passed'] &= result.returncode==0 and unchanged and binding['declared_case_count_passed']
    if not report['passed']:report['status']='failed'
    path.write_text(json.dumps(report,indent=2)+'\n')
    (args.output/'source-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],cases=binding['expected_cases'],report=str(path))),flush=True)
    return 0 if report['passed'] else result.returncode or 1


if __name__=='__main__':raise SystemExit(main())
