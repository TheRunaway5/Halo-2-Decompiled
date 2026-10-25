#!/usr/bin/env python3
"""Disposable authored model regression; optionally borrow a pinned native Cache.

Use tools/run_test.py. The CMake worker owns all generated source/build files.
Before-change evidence is explicit; it cannot count as a passing conversion.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

from refresh_destruction_current_cohort import sha

ROOT = Path(__file__).resolve().parents[1]
SPECS = {
    'hunter': dict(header='include/halo2/mcc_destruction_model_contract.hpp',
        source='src/assets/mcc/mcc_destruction_model_contract.cpp',
        test='tests/mcc_destruction_hunter_contract_tests.cpp',
        profile='tools/mcc-destruction-hunter-contract-profile.json',
        define='HALO2_DESTRUCTION_HUNTER_CACHE_CHECK',
        signature='Native Cache Hunter original empty response differs: section=0 field=4 expected=0 actual=2'),
    'flood': dict(header='include/halo2/mcc_destruction_model_reference.hpp',
        source='src/assets/mcc/mcc_destruction_model_reference.cpp',
        test='tests/mcc_destruction_model_reference_tests.cpp',
        profile='tools/mcc-destruction-model-reference-profile.json',
        define='HALO2_DESTRUCTION_MODEL_REFERENCE_CACHE_CHECK',
        signature='Native Cache Flood-clump response effect differs: expected=null actual=nonnull'),
}



def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--family',choices=tuple(SPECS),default='hunter')
    parser.add_argument('--baseline-cohort',type=Path)
    parser.add_argument('--baseline-refresh',type=Path)
    parser.add_argument('--expect-unconverted',action='store_true')
    args = parser.parse_args();family=args.family;spec=SPECS[family]
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    if bool(args.baseline_cohort)!=bool(args.baseline_refresh):parser.error('Supply both native provenance receipts')
    if args.expect_unconverted and not args.baseline_cohort:parser.error('Before-change check requires native Cache')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source = scratch/'source';source.mkdir()
    args.output = args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    headers = ROOT/'include';libraries = [];archive_hashes = {};maps = []
    profile_path = ROOT/spec['profile']
    profile = json.loads(profile_path.read_text())
    if sha(ROOT/profile['authority_report'])!=profile['authority_sha256']:
        raise ValueError('Authored model authority changed')
    if args.baseline_cohort:
        cohort = json.loads(args.baseline_cohort.read_text())
        refresh = json.loads(args.baseline_refresh.read_text())
        frozen = Path(cohort['scratch']).resolve(strict=True)
        if Path(refresh['scratch']).resolve()!=frozen or not refresh['passed']:
            raise ValueError('Native Cache provenance is incomplete or belongs to another owner')
        headers = frozen/'source/include'
        manifest = json.loads((args.baseline_cohort.parent/'source/source-manifest.json').read_text())
        name = 'src/assets/mcc_cache.cpp'
        expected = refresh['patches'].get(name,{}).get('after',manifest['source_files_sha256'][name])
        if sha(frozen/'source'/name)!=expected:raise ValueError('Native Cache source drifted')
        link = shlex.split((frozen/'build/CMakeFiles/mcc_gameplay_scalar_tests.dir/link.txt').read_text())
        for item in link[link.index('-o')+2:]:
            if item.startswith('-'):raise ValueError('Requalify unexpected native library flags')
            path = Path(item) if Path(item).is_absolute() else frozen/'build'/item
            libraries.append(str(path));archive_hashes[str(path)] = sha(path)
        for row in profile['paired_maps']:
            if sha(row['mcc_path'])!=row['mcc_sha256']:raise ValueError('Authored model native donor map changed')
            maps.append(Path(row['mcc_path']).resolve())
    files = [path for path in headers.rglob('*') if path.is_file()]
    files += [ROOT/spec[key] for key in ('header','source','test','profile')]
    if any(path.is_symlink() for path in files) or sum(path.stat().st_size for path in files)>16*1024*1024:
        raise ValueError('Authored model source inputs exceed private budget')
    hashes = {}
    for path in files:
        relative = Path('include')/path.relative_to(headers) if path.is_relative_to(headers) else path.relative_to(ROOT)
        before = sha(path);target = source/relative;target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(path,target)
        if sha(path)!=before or sha(target)!=before:raise ValueError('Authored model input changed during copy')
        hashes[str(relative)] = before
    cmake = f'''cmake_minimum_required(VERSION 3.20)
project(DestructionAuthoredContract LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
find_package(nlohmann_json CONFIG REQUIRED)
enable_testing()
add_executable({family} {spec['test']} {spec['source']})
target_include_directories({family} PRIVATE include)
target_link_libraries({family} PRIVATE nlohmann_json::nlohmann_json)
'''
    if libraries:
        if any(any(c in value for c in '\n\r";$') for value in libraries):raise ValueError('Unexpected library path syntax')
        cmake += f"target_compile_definitions({family} PRIVATE {spec['define']})\n"
        cmake += f'target_link_libraries({family} PRIVATE '+' '.join('"'+x+'"' for x in libraries)+')\n'
    argument = f"\"${{CMAKE_CURRENT_SOURCE_DIR}}/{spec['profile']}\""
    cmake += f'add_test(NAME {family}_component COMMAND {family} '+argument+')\n'
    for path in maps:
        cmake += f'add_test(NAME {family}_cache_{path.stem} COMMAND {family} {argument} "{path}" "{path.parent}")\n'
    (source/'CMakeLists.txt').write_text(cmake)
    command = [sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
               '--target',family,'--filter',f'^{family}_','--jobs','2','--test-jobs','1',
               '--output',str(args.output/'validation')]
    report = dict(passed=False,owned_scratch=str(scratch),source_files_sha256=hashes,
                  command=command,tool_sha256=sha(__file__),generated_cmake_sha256=sha(source/'CMakeLists.txt'),
                  read_only_native_library_inputs=archive_hashes,retained_large_artifacts=[],
                  scope='Authored input component/Cache only; no native damage or full-goal acceptance')
    def save():(args.output/'component.json').write_text(json.dumps(report,indent=2)+'\n')
    save();code = subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report.update(passed=code==0,returncode=code,native_library_inputs_unchanged=all(sha(p)==h for p,h in archive_hashes.items()))
    if args.expect_unconverted:
        log = (args.output/'validation/tests.log').read_text()
        signature = spec['signature']
        worker = json.loads((args.output/'validation/report.json').read_text())
        report['expected_original_contract_rejections'] = log.count(signature)
        report['expected_before_change_rejection_observed'] = (code!=0 and log.count(signature)==len(maps) and
            report['native_library_inputs_unchanged'] and all(s['exit_code']==0 for s in worker['steps'][:2]))
    save();print(json.dumps(dict(passed=report['passed'],returncode=code,report=str(args.output/'component.json'))))
    return (0 if report['expected_before_change_rejection_observed'] else 1) if args.expect_unconverted else code


if __name__=='__main__':raise SystemExit(main())
