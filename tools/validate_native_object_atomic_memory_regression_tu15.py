#!/usr/bin/env python3
"""Run existing retained transfer/backing regressions on the staged byte owner."""
import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

from validate_native_object_atomic_byte_tu15 import ROOT, STAGE, sha

FILES = (
    'tests/source_runtime_transfer_tests.cpp',
    'tests/source_runtime_shared_backing_tests.cpp',
    'src/runtime/source_runtime_snapshot_bytes.cpp',
    'src/assets/cache_file_snapshot.cpp',
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    project = scratch / 'project'
    project.mkdir()
    report = dict(passed=False, owned_scratch=str(scratch), retained_large_artifacts=[], results=[])
    try:
        shutil.copytree(ROOT / 'include', project / 'include')
        inputs = {rel: ROOT / rel for rel in FILES}
        for p in STAGE.rglob('*'):
            if p.is_file():
                inputs[str(p.relative_to(STAGE))] = p
        for rel, p in inputs.items():
            dst = project / rel
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(p, dst)
        report['input_sha256'] = {str(p.relative_to(ROOT)): sha(p) for p in inputs.values()}
        report['validator_sha256'] = sha(__file__)
        (project / 'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(atomicMemoryRegression LANGUAGES CXX)
include(CTest)
find_package(Threads REQUIRED)
add_library(memory STATIC src/runtime/source_runtime_memory.cpp src/runtime/source_runtime_snapshot_bytes.cpp src/assets/cache_file_snapshot.cpp)
target_include_directories(memory PUBLIC include PRIVATE src)
target_compile_features(memory PUBLIC cxx_std_20)
target_link_libraries(memory PUBLIC Threads::Threads)
add_executable(transfer tests/source_runtime_transfer_tests.cpp)
add_executable(backing tests/source_runtime_shared_backing_tests.cpp)
target_link_libraries(transfer PRIVATE memory)
target_link_libraries(backing PRIVATE memory)
add_custom_target(memory_regressions DEPENDS transfer backing)
foreach(mode read write boundaries)
    add_test(NAME source_runtime_transfer_${mode} COMMAND transfer ${mode})
endforeach()
add_test(NAME source_runtime_shared_backing COMMAND backing)
''')
        compiled = {str(p.relative_to(project)): sha(p) for p in project.rglob('*') if p.is_file()}
        report['compiled_source_pins'] = compiled
        for cc in ('g++', 'clang++'):
            command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--source', str(project), '--scratch-root', str(scratch / 'builds'), '--output', str(output / cc), '--target', 'memory_regressions', '--filter', '^source_runtime_(transfer|shared_backing)', '--cmake-arg=-DCMAKE_CXX_COMPILER=' + cc, '--jobs', '2', '--test-jobs', '1']
            result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            (output / (cc + '-console.log')).write_text(result.stdout[-64000:])
            report['results'].append(dict(compiler=cc, command=command, exit_code=result.returncode))
            assert result.returncode == 0, result.stdout[-6000:]
        assert all(sha(project / p) == h for p, h in compiled.items())
        assert all(sha(ROOT / p) == h for p, h in report['input_sha256'].items())
        report['passed'] = True
        report['scope'] = 'Existing transfer read/write/boundaries and shared backing lifetime/guard/checkpoint/admission regressions. No standalone new runtime link dependency; byte kernel is inline.'
    except BaseException as error:
        report['error'] = str(error) or repr(error)
        raise
    finally:
        report['temporary_allocated_bytes_before_cleanup'] = sum(p.stat().st_blocks * 512 for p in scratch.rglob('*') if p.is_file())
        (output / 'report.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
