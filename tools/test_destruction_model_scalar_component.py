#!/usr/bin/env python3
"""Build the finite model scalar regression without the full live world.

Use tools/run_test.py. The official CMake worker and its private sources live
under HALO2_TEST_SCRATCH; only bounded source hashes/logs survive cleanup.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--baseline-cohort', type=Path,
                        help='Run the new regression against the completed unmodified native Cache archives')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    source = scratch / 'source';source.mkdir()
    args.output = args.output.resolve();args.output.mkdir(parents=True, exist_ok=False)
    headers = ROOT / 'include'
    baseline = None
    libraries = []
    archive_hashes = {}
    if args.baseline_cohort:
        baseline = json.loads(args.baseline_cohort.read_text())
        frozen = Path(baseline['scratch']).resolve(strict=True)
        headers = frozen / 'source/include'
        frozen_manifest = json.loads((args.baseline_cohort.parent / 'source/source-manifest.json').read_text())
        path = 'src/assets/mcc_cache.cpp'
        if sha(frozen/'source'/path)!=frozen_manifest['source_files_sha256'][path]:
            raise ValueError('Native Cache baseline already changed from its frozen source')
        link = shlex.split((frozen/'build/CMakeFiles/mcc_gameplay_scalar_tests.dir/link.txt').read_text())
        for item in link[link.index('-o')+2:]:
            if item.startswith('-'):
                raise ValueError('Requalify unexpected baseline library flags')
            path = Path(item) if Path(item).is_absolute() else frozen/'build'/item
            libraries.append(str(path));archive_hashes[str(path)] = sha(path)
    files = [path for path in headers.rglob('*') if path.is_file()]
    files += [ROOT / name for name in ('src/assets/mcc/mcc_destruction_model_scalar.cpp',
        'tests/mcc_destruction_model_scalar_tests.cpp', 'tools/mcc-destruction-model-scalar-profile.json')]
    if any(path.is_symlink() for path in files) or sum(path.stat().st_size for path in files)>16*1024*1024:
        raise ValueError('Component source inputs exceed the declared private budget')
    hashes = {}
    for path in files:
        relative = Path('include')/path.relative_to(headers) if path.is_relative_to(headers) else path.relative_to(ROOT)
        before = sha(path)
        destination = source / relative;destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, destination)
        if sha(path)!=before or sha(destination)!=before:
            raise ValueError(f'Component input changed while copying: {relative}')
        hashes[str(relative)] = before
    if baseline:
        # The class/header ABI comes from the very same frozen native owner.
        # This new public declaration only exposes the finite pure converter.
        path = ROOT/'include/halo2/mcc_destruction_model_scalar.hpp'
        destination = source/'include/halo2/mcc_destruction_model_scalar.hpp'
        destination.write_bytes(path.read_bytes());hashes['include/halo2/mcc_destruction_model_scalar.hpp'] = sha(destination)
    cmake = '''cmake_minimum_required(VERSION 3.20)
project(DestructionModelScalar LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
find_package(nlohmann_json CONFIG REQUIRED)
enable_testing()
add_executable(model_scalar tests/mcc_destruction_model_scalar_tests.cpp src/assets/mcc/mcc_destruction_model_scalar.cpp)
target_include_directories(model_scalar PRIVATE include)
target_link_libraries(model_scalar PRIVATE nlohmann_json::nlohmann_json)
'''
    arguments = ''
    if baseline:
        if any(any(c in value for c in '\n\r";$') for value in libraries):
            raise ValueError('Requalify unexpected baseline library path syntax')
        cmake += 'target_compile_definitions(model_scalar PRIVATE HALO2_DESTRUCTION_MODEL_CACHE_CHECK)\n'
        cmake += 'target_link_libraries(model_scalar PRIVATE '+' '.join('"'+value+'"' for value in libraries)+')\n'
        maps = Path(next(iter(baseline['runtime_inputs']['destruction_maps']))).parent
        path = str(maps/'foundation.map')
        if sha(path)!=baseline['runtime_inputs']['destruction_maps'][path]:
            raise ValueError('Native Cache baseline map changed')
        arguments = f' "{path}" "{maps}"'
    cmake += ('add_test(NAME original_authored_model_tuple COMMAND model_scalar '
              '"${CMAKE_CURRENT_SOURCE_DIR}/tools/mcc-destruction-model-scalar-profile.json"'+arguments+')\n')
    (source / 'CMakeLists.txt').write_text(cmake)
    command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--build-worker', '--source', str(source),
               '--target', 'model_scalar', '--filter', '^original_authored_model_tuple$', '--jobs', '2',
               '--output', str(args.output / 'validation')]
    report = dict(passed=False, owned_scratch=str(scratch), source_files_sha256=hashes, command=command,
                  tool_sha256=sha(__file__), generated_cmake_sha256=sha(source/'CMakeLists.txt'),
                  scope='Pure authored scalar conversion and negative controls; native Cache/world acceptance remains separate',
                  retained_large_artifacts=[])
    if baseline:
        report['scope'] = 'Actual frozen native Cache before-change regression; expected original tuple rejection, no destruction or full goal acceptance'
        report['baseline_cohort'] = str(args.baseline_cohort.resolve())
        report['read_only_native_library_inputs'] = archive_hashes
    (args.output / 'component.json').write_text(json.dumps(report, indent=2) + '\n')
    code = subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT).returncode
    report.update(passed=code==0, returncode=code)
    if baseline:
        report['native_library_inputs_unchanged'] = all(sha(path)==digest for path,digest in archive_hashes.items())
        log = (args.output/'validation/tests.log').read_text()
        report['expected_original_tuple_rejection_observed'] = (code!=0 and
            'Native Cache model response field +04 differs: expected=0 actual=33554432' in log and
            report['native_library_inputs_unchanged'])
    (args.output / 'component.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], returncode=code, report=str(args.output / 'component.json'))))
    return (0 if report['expected_original_tuple_rejection_observed'] else 1) if baseline else code


if __name__ == '__main__':
    raise SystemExit(main())
