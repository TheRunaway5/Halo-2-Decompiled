#!/usr/bin/env python3
"""Qualify actual residual ABI transport around native constraint kernels.

This is an integration model, not an original solver or live parity result.
All generated CMake/build outputs belong to the enclosing run_test.py owner.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
def sha(path):
    return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=True)
    sources=['tests/native_tu15_constraint_residuals_tests.cpp',
        'src/physics/native_tu15_constraint_step.cpp',
        'src/physics/native_tu15_constraint_step_residuals.cpp',
        'src/physics/native_tu15_dynamic_accumulator.cpp',
        'src/physics/native_tu15_motion_forces.cpp','src/physics/native_tu15_motion_commit.cpp',
        'src/physics/native_tu15_constraint_solver.cpp',
        'src/physics/native_tu15_constraint_solver_rows.cpp',
        'src/physics/native_tu15_constraint_solver_initializer.cpp',
        'src/physics/native_tu15_constraint_custom_schema.cpp',
        'src/physics/native_tu15_constraint_clock.cpp','src/physics/native_tu15_constraint_producer.cpp',
        'src/physics/native_tu15_constraint_base.cpp','src/physics/native_tu15_constraint_schema_emitter.cpp']
    headers=['include/halo2/native_tu15_constraint_step.hpp',
        'include/halo2/native_tu15_constraint_step_residuals.hpp',
        'include/halo2/native_tu15_dynamic_accumulator.hpp',
        'include/halo2/native_tu15_motion_forces.hpp','include/halo2/native_tu15_motion_commit.hpp',
        'include/halo2/native_tu15_constraint_solver.hpp',
        'src/physics/native_tu15_constraint_solver_internal.hpp',
        'include/halo2/native_tu15_constraint_solver_initializer.hpp',
        'include/halo2/native_tu15_constraint_custom_schema.hpp',
        'include/halo2/native_tu15_constraint_clock.hpp','include/halo2/native_tu15_constraint_producer.hpp',
        'include/halo2/native_tu15_constraint_base.hpp','include/halo2/native_tu15_constraint_schema_emitter.hpp']
    pins={p:sha(ROOT/p) for p in sources+headers}
    project=scratch/'project';project.mkdir()
    units=' '.join('"'+str(ROOT/p)+'"' for p in sources)
    (project/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(native_constraint_binding LANGUAGES CXX)
include(CTest)
add_executable(native_constraint_binding_tests {units})
target_include_directories(native_constraint_binding_tests PRIVATE "{ROOT}/include")
target_compile_features(native_constraint_binding_tests PRIVATE cxx_std_20)
target_compile_options(native_constraint_binding_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME native_constraint_binding_model_tu15 COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_constraint_binding_tests>)
''')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),
        '--scratch-root',str(scratch/'builds'),'--output',str(args.output/'cmake'),
        '--target','native_constraint_binding_tests','--filter','^native_constraint_binding_model_tu15$',
        '--jobs','2','--test-jobs','1']
    result=subprocess.run(command,check=False)
    changed=[p for p,h in pins.items() if sha(ROOT/p)!=h]
    report=dict(passed=result.returncode==0 and not changed,exit_code=result.returncode,
        source_hashes=pins,changed_during_validation=changed,command=command,
        scope='Actual named residual ABI, native dynamic setup, mutable packet, FP and lifetime integration model',
        original_solver_parity_proven=False,live_multiplayer_proven=False,
        scratch_owner=str(scratch),retained_large_artifacts=[])
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in ['passed','changed_during_validation','scope']}))
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
