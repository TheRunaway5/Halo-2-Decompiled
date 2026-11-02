#!/usr/bin/env python3
"""Validate the actual cold-data factory using a frozen, owned source closure."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH'])
    assert owner.is_dir()
    project = owner/'project'
    project.mkdir()
    module = 'cmake/NativeTu15ColdData.cmake'
    top_level = (ROOT/'CMakeLists.txt').read_text()
    augmentation = re.search(r'if\(TARGET native_tu15_cold_data_tests\)\n.*?endif\(\)',
                             top_level, re.S).group()
    pending = [module, 'tests/native_tu15_cold_data_tests.cpp',
        'cmake/NativeTu15DataArrayDelete.cmake', 'src/core/native_tu15_data_array_delete.cpp',
        *re.findall(r'src/[^\s)]+\.cpp', augmentation),
        *re.findall(r'^\s+(src/[^\s)]+\.cpp)\s*$', (ROOT/module).read_text(), re.M)]
    inputs = {}
    while pending:
        name = pending.pop()
        if name in inputs:
            continue
        original = ROOT/name
        inputs[name] = hashlib.sha256(original.read_bytes()).hexdigest()
        destination = project/name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(original, destination)
        for include in re.findall(r'^\s*#include\s+"([^"]+)"', original.read_text(), re.M):
            for base in (original.parent, ROOT/'include', ROOT/'src', ROOT/'tests'):
                candidate = base/include
                if candidate.is_file():
                    pending.append(str(candidate.relative_to(ROOT)))
                    break
    (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(TimerColdData LANGUAGES CXX)
include(CTest)
find_package(Threads REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include(cmake/NativeTu15ColdData.cmake)
include(cmake/NativeTu15DataArrayDelete.cmake)
''' + augmentation + '\n')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    command = [sys.executable, str(ROOT/'tools/run_tests.py'), '--source', str(project),
        '--target', 'native_tu15_cold_data_tests', '--filter', '^native_tu15_cold_data$',
        '--jobs', '2', '--test-jobs', '1', '--output', str(output),
        '--retain-report', 'Testing/Temporary/LastTest.log']
    result = subprocess.run(command)
    unchanged = all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest() == h for p, h in inputs.items())
    (output/'source-inputs.json').write_text(json.dumps(dict(source_sha256=inputs,
        source_unchanged=unchanged, owned_scratch=str(owner), returncode=result.returncode,
        retained_large_artifacts=[]), indent=2) + '\n')
    raise SystemExit(result.returncode or (0 if unchanged else 1))


if __name__ == '__main__':
    main()
