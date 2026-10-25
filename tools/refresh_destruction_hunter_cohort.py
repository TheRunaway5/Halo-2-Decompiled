#!/usr/bin/env python3
"""Admit only the reviewed Hunter authored input correction into the active owner.

Use the finite tools/run_test.py session. Authenticate the effective frozen
source before applying exact substitutions; use the official CMake worker.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

from refresh_destruction_current_cohort import sha

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cohort',required=True,type=Path)
    parser.add_argument('--production-refresh',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--jobs',type=int,default=8)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use the still-owned finite test session')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source = scratch/'source'
    cohort = json.loads(args.cohort.read_text());prior = json.loads(args.production_refresh.read_text())
    if any(Path(row['scratch']).resolve()!=scratch for row in (cohort,prior)) or not prior['passed']:
        raise ValueError('Hunter production receipt has another owner or failed')
    if prior['prior_cohort']!=str(args.cohort.resolve()):raise ValueError('Hunter receipt belongs to another cohort')
    args.output = args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    receipt = args.output/'admission.json'
    report = dict(passed=False,scratch=str(scratch),prior_cohort=str(args.cohort.resolve()),
                  inherited_production_refresh=str(args.production_refresh.resolve()),patches=dict(prior['patches']),
                  tool_sha256=sha(__file__),retained_large_artifacts=[])
    def save():receipt.write_text(json.dumps(report,indent=2)+'\n')
    manifest = json.loads((args.cohort.parent/'source/source-manifest.json').read_text())
    expected = dict(manifest['source_files_sha256']);expected.update(cohort['additional_frozen_inputs'])
    expected.update({name:change['after'] for name,change in prior['patches'].items()})
    expected['CMakeLists.txt'] = prior['cmake_sha256']
    if cohort.get('private_build_partition'):
        part = cohort['private_build_partition'];expected[part['path']] = part['after']
    report['frozen_source_drift_before_refresh'] = [p for p,h in expected.items() if sha(source/p)!=h]
    save()
    if report['frozen_source_drift_before_refresh']:raise ValueError('Hunter frozen source drifted')
    profile = json.loads((ROOT/'tools/mcc-destruction-hunter-contract-profile.json').read_text())
    if sha(ROOT/profile['authority_report'])!=profile['authority_sha256']:raise ValueError('Hunter authority changed')
    maps = {}
    for row in profile['paired_maps']:
        for field,hash_field in (('path','sha256'),('mcc_path','mcc_sha256')):
            if sha(row[field])!=row[hash_field]:raise ValueError('Hunter reference owner changed')
        maps[Path(row['mcc_path']).stem] = Path(row['mcc_path']).resolve()
    report['runtime_map_sha256'] = {str(p):sha(p) for p in maps.values()}
    baseline = json.loads((ROOT/'analysis/destruction-goal-20261022/hunter-native-before-v1/component.json').read_text())
    if not baseline.get('expected_before_change_rejection_observed'):raise ValueError('Hunter lacks direct native red evidence')
    report['native_before_change_regression'] = dict(report='analysis/destruction-goal-20261022/hunter-native-before-v1/component.json',
        expected_rejections=baseline['expected_original_contract_rejections'])
    plan = {}
    for name in ('include/halo2/mcc_destruction_model_contract.hpp','src/assets/mcc/mcc_destruction_model_contract.cpp',
                 'tests/mcc_destruction_hunter_contract_tests.cpp','tools/mcc-destruction-hunter-contract-profile.json'):
        if (source/name).exists():raise ValueError('Hunter admission source already exists')
        data = (ROOT/name).read_bytes()
        if len(data)>128*1024:raise ValueError('Hunter finite source budget exceeded')
        plan[name] = data
    name = 'src/assets/mcc_cache.cpp';text = (source/name).read_text()
    include = '#include "halo2/mcc_destruction_model_scalar.hpp"'
    call = '        (void)convert_mcc_destruction_model_scalars(t.type(),t.name,t.address,reference_access.bytes);'
    if text.count(include)!=1 or text.count(call)!=1:raise ValueError('Hunter Cache normalization seam changed')
    text = text.replace(include,include+'\n#include "halo2/mcc_destruction_model_contract.hpp"')
    text = text.replace(call,call+'\n    for(const auto& t:tags_)if(t.address && t.size && t.type()==tag_groups::model)\n'
                        '        (void)convert_mcc_destruction_model_contract(t.type(),t.name,t.address,reference_access.bytes);')
    plan[name] = text.encode()
    name = 'tests/physics_multiplayer_acceptance.cpp';text = (source/name).read_text()
    old = 'unsigned source_drive_ticks=vehicle && std::getenv("HALO2_TU15_VEHICLE_BLAST_INPUT")?300u:0u;'
    if text.count(old)!=1:raise ValueError('Moving-vehicle source tick fixture seam changed')
    plan[name] = text.replace(old,old.replace('vehicle &&','vehicle && !fixed_turret_test &&')).encode()
    cmake = source/'CMakeLists.txt';text = cmake.read_text()
    needle = 'target_sources(halo2_cache PRIVATE src/assets/mcc/mcc_destruction_model_scalar.cpp)'
    if text.count(needle)!=1:raise ValueError('Hunter Cache source registration seam changed')
    text = text.replace(needle,needle+'\ntarget_sources(halo2_cache PRIVATE src/assets/mcc/mcc_destruction_model_contract.cpp)')
    text += '''
add_executable(mcc_destruction_hunter_contract_tests EXCLUDE_FROM_ALL tests/mcc_destruction_hunter_contract_tests.cpp src/assets/mcc/mcc_destruction_model_contract.cpp)
target_include_directories(mcc_destruction_hunter_contract_tests PRIVATE include)
target_link_libraries(mcc_destruction_hunter_contract_tests PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME mcc_destruction_hunter_contract COMMAND mcc_destruction_hunter_contract_tests "${CMAKE_CURRENT_SOURCE_DIR}/tools/mcc-destruction-hunter-contract-profile.json")
add_executable(mcc_destruction_hunter_cache_tests EXCLUDE_FROM_ALL tests/mcc_destruction_hunter_contract_tests.cpp)
target_compile_definitions(mcc_destruction_hunter_cache_tests PRIVATE HALO2_DESTRUCTION_HUNTER_CACHE_CHECK)
target_link_libraries(mcc_destruction_hunter_cache_tests PRIVATE halo2_cache)
'''
    for name,path in maps.items():
        text += (f'add_test(NAME destruction_hunter_cache_{name} COMMAND mcc_destruction_hunter_cache_tests '
                 f'"${{CMAKE_CURRENT_SOURCE_DIR}}/tools/mcc-destruction-hunter-contract-profile.json" "{path}" "{path.parent}")\n')
    report['cmake_before'] = sha(cmake)
    for name,data in plan.items():
        target = source/name;before = sha(target) if target.exists() else None
        target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
        report['patches'][name] = dict(before=before,after=sha(target))
    cmake.write_text(text);report['cmake_sha256'] = sha(cmake)
    command = [sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
               '--target','mcc_destruction_hunter_contract_tests','--target','mcc_destruction_hunter_cache_tests',
               '--target','halo2_native','--target','physics_multiplayer_acceptance',
               '--filter','^(mcc_destruction_hunter_contract|destruction_hunter_cache_.*|destruction_lan_identity_tu15)$',
               '--jobs',str(args.jobs),'--test-jobs','1','--output',str(args.output/'validation'),
               '--retain-report','destruction-lan-identity.json']
    report['command'] = command;save();print(json.dumps(dict(status='validating',report=str(receipt))),flush=True)
    code = subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report.update(passed=code==0,returncode=code);save()
    print(json.dumps(dict(passed=code==0,returncode=code,report=str(receipt))),flush=True);return code


if __name__=='__main__':raise SystemExit(main())
