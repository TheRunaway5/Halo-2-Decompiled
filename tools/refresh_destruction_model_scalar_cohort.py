#!/usr/bin/env python3
"""Admit the narrow model scalar fix into an authenticated owned live build.

Use the existing tools/run_test.py-owned finite session, after its current
worker finishes. Preserve the frozen source; add only the named converter,
Cache call and finite regressions. The official CMake worker owns validation.
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
    parser.add_argument('--cohort', required=True, type=Path)
    parser.add_argument('--previous-refresh', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--jobs', type=int, default=8)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use the still-owned finite test session')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source = scratch / 'source'
    cohort = json.loads(args.cohort.read_text())
    previous = json.loads(args.previous_refresh.read_text())
    if Path(cohort['scratch']).resolve()!=scratch or Path(previous['scratch']).resolve()!=scratch:
        raise ValueError('Model refresh has a different build owner')
    if previous['prior_cohort']!=str(args.cohort.resolve()):
        raise ValueError('Previous fixture refresh has a different source cohort')
    args.output = args.output.resolve();args.output.mkdir(parents=True, exist_ok=False)
    receipt = args.output / 'admission.json'
    report = dict(passed=False, scratch=str(scratch), prior_cohort=str(args.cohort.resolve()),
                  previous_refresh=str(args.previous_refresh.resolve()), patches={}, retained_large_artifacts=[])
    def save():
        receipt.write_text(json.dumps(report, indent=2) + '\n')
    manifest = json.loads((args.cohort.parent / 'source/source-manifest.json').read_text())
    expected = dict(manifest['source_files_sha256']);expected.update(cohort['additional_frozen_inputs'])
    expected['CMakeLists.txt'] = previous['registered_cmake_sha256']
    expected['tests/destruction_runtime_tu15_tests.cpp'] = previous['fixture_refresh']['after']
    if cohort.get('private_build_partition'):
        partition = cohort['private_build_partition'];expected[partition['path']] = partition['after']
    report['frozen_source_drift_before_refresh'] = [name for name,digest in expected.items() if sha(source/name)!=digest]
    save()
    if report['frozen_source_drift_before_refresh']:
        raise ValueError('Frozen source changed before model scalar admission')
    maps = Path(next(iter(cohort['runtime_inputs']['destruction_maps']))).parent
    report['runtime_map_sha256'] = {}
    for name in ('foundation','coagulation'):
        path = str(maps/(name+'.map'));digest = sha(path)
        if cohort['runtime_inputs']['destruction_maps'].get(path)!=digest:
            raise ValueError('Model Cache input changed since the authenticated cohort')
        report['runtime_map_sha256'][path] = digest
    plan = {}
    for name in ('include/halo2/mcc_destruction_model_scalar.hpp',
                 'src/assets/mcc/mcc_destruction_model_scalar.cpp',
                 'tests/mcc_destruction_model_scalar_tests.cpp',
                 'tools/mcc-destruction-model-scalar-profile.json'):
        if (source/name).exists():
            raise ValueError(f'Unexpected existing source before model admission: {name}')
        data = (ROOT/name).read_bytes()
        if len(data)>128*1024:
            raise ValueError('Model scalar source exceeds its finite input budget')
        plan[name] = data
    name = 'src/assets/mcc_cache.cpp';text = (source/name).read_text()
    include = '#include "halo2/mcc_gameplay_unit_scalar.hpp"'
    call = '        (void)convert_mcc_gameplay_unit_scalars(t.type(),t.name,t.address,reference_access.bytes);'
    if text.count(include)!=1 or text.count(call)!=1:
        raise ValueError('Frozen Cache normalization seam changed')
    text = text.replace(include, include+'\n#include "halo2/mcc_destruction_model_scalar.hpp"')
    text = text.replace(call,call+'\n    for(const auto& t:tags_)if(t.address && t.size && t.type()==tag_groups::model)\n'
                        '        (void)convert_mcc_destruction_model_scalars(t.type(),t.name,t.address,reference_access.bytes);')
    plan[name] = text.encode()
    cmake = source/'CMakeLists.txt';text = cmake.read_text()
    needle = 'target_sources(halo2_cache PRIVATE src/assets/mcc/mcc_gameplay_unit_scalar.cpp)'
    if text.count(needle)!=1 or 'add_executable(mcc_destruction_model_scalar_tests' in text:
        raise ValueError('Frozen CMake model registration seam changed')
    text = text.replace(needle,needle+'\ntarget_sources(halo2_cache PRIVATE src/assets/mcc/mcc_destruction_model_scalar.cpp)')
    text += '''
add_executable(mcc_destruction_model_scalar_tests EXCLUDE_FROM_ALL tests/mcc_destruction_model_scalar_tests.cpp src/assets/mcc/mcc_destruction_model_scalar.cpp)
target_include_directories(mcc_destruction_model_scalar_tests PRIVATE include)
target_link_libraries(mcc_destruction_model_scalar_tests PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME mcc_destruction_model_scalar_profiles COMMAND mcc_destruction_model_scalar_tests "${CMAKE_CURRENT_SOURCE_DIR}/tools/mcc-destruction-model-scalar-profile.json")
add_executable(mcc_destruction_model_cache_tests EXCLUDE_FROM_ALL tests/mcc_destruction_model_scalar_tests.cpp)
target_compile_definitions(mcc_destruction_model_cache_tests PRIVATE HALO2_DESTRUCTION_MODEL_CACHE_CHECK)
target_link_libraries(mcc_destruction_model_cache_tests PRIVATE halo2_cache)
'''
    for name in ('foundation','coagulation'):
        text += (f'add_test(NAME destruction_model_cache_{name} COMMAND mcc_destruction_model_cache_tests '
                 f'"${{CMAKE_CURRENT_SOURCE_DIR}}/tools/mcc-destruction-model-scalar-profile.json" '
                 f'"{maps/(name+".map")}" "{maps}")\n')
    report['cmake_before'] = sha(cmake)
    for name,data in plan.items():
        destination = source/name
        before = sha(destination) if destination.exists() else None
        destination.parent.mkdir(parents=True, exist_ok=True);destination.write_bytes(data)
        report['patches'][name] = dict(before=before, after=sha(destination))
    cmake.write_text(text);report['cmake_sha256'] = sha(cmake)
    output = scratch/'build/destruction-h_turret_ap.json'
    if output.exists():
        report['removed_previous_runtime_report_sha256'] = sha(output);output.unlink()
    command = [sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
               '--target','mcc_destruction_model_scalar_tests','--target','mcc_destruction_model_cache_tests',
               '--target','destruction_runtime_tu15_tests','--target','halo2_native',
               '--target','physics_multiplayer_acceptance','--filter',
               '^(mcc_destruction_model_scalar_profiles|destruction_model_cache_(foundation|coagulation)|destruction_mcc_h_turret_ap)$',
               '--jobs',str(args.jobs),'--test-jobs','1','--output',str(args.output/'validation'),
               '--retain-report','destruction-h_turret_ap.json']
    report['command'] = command;save()
    print(json.dumps(dict(status='validating', report=str(receipt))),flush=True)
    environment = dict(os.environ,HALO2_DESTRUCTION_CONTINUATION='1',HALO2_DESTRUCTION_FIXED_CLOCK='1')
    code = subprocess.run(command,env=environment,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report.update(passed=code==0, returncode=code);save()
    print(json.dumps(dict(passed=code==0,returncode=code,report=str(receipt))),flush=True)
    return code


if __name__ == '__main__':
    raise SystemExit(main())
