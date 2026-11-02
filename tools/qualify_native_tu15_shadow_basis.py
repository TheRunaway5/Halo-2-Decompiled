#!/usr/bin/env python3
"""Freeze and qualify native32340 arithmetic through disposable CMake runners."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--refresh-fixture',action='store_true');a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=scratch/'source';source.mkdir();a.output.mkdir(parents=True,exist_ok=True)
    shutil.copytree(ROOT/'include',source/'include')
    names=['include/halo2/native_tu15_shadow_basis_32340.hpp','src/rendering/shadows/native_tu15_shadow_basis_32340.cpp',
        'src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp','include/halo2/detail/native_tu15_arithmetic_environment.hpp',
        'tests/native_tu15_shadow_basis_tests.cpp','tools/validate_native_tu15_shadow_basis.py','tools/test_native_tu15_shadow_basis_fixture.py','cmake/NativeTu15ShadowBasis.cmake']
    pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in names}
    for name in names:
        target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    # Physical oracle imports remain in the owning checkout. Exact candidate
    # bytes are frozen here; the final pins reject concurrent helper drift.
    (source/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(NativeShadowBasis LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include("{ROOT}/cmake/TestStorage.cmake")
set(HALO2_RENDERER_ORACLE_PYTHON "{ROOT}/.tools/python/bin/python")
set(HALO2_TU15_GAMEPLAY_XBE "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
set(HALO2_SHADOW_BASIS_ORACLE_SCRIPT "{ROOT}/tools/validate_native_tu15_shadow_basis.py")
set(HALO2_SHADOW_BASIS_FIXTURE_PATH "{ROOT}/tests/fixtures/native_tu15_shadow_basis.json.gz")
include(cmake/NativeTu15ShadowBasis.cmake)
''')
    results=[]
    for kind,compiler,sanitize in [('gcc','g++',False),('clang-sanitizers','clang++',True)]:
        selected='^original_tu15_native_shadow_basis$'if a.refresh_fixture and kind=='gcc'else'^(original_tu15_native_shadow_basis|native_tu15_shadow_basis_fixture)$'
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'builds'),
            '--output',str(a.output/kind),'--target','native_tu15_shadow_basis_tests','--filter',selected,
            '--jobs','2','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native-tu15-shadow-basis.json']
        if sanitize:command+=['--sanitizers']
        if a.refresh_fixture:command+=['--cmake-arg=-DHALO2_SHADOW_BASIS_FIXTURE_OUTPUT='+str(ROOT/'tests/fixtures/native_tu15_shadow_basis.json.gz')]
        r=subprocess.run(command);results.append(dict(kind=kind,returncode=r.returncode,command=command))
        if r.returncode:break
    unchanged=all(hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==pin for name,pin in pins.items())
    passed=len(results)==2 and all(r['returncode']==0 for r in results)and unchanged
    (a.output/'validation.json').write_text(json.dumps(dict(passed=passed,results=results,source_sha256=pins,
        source_unchanged=unchanged,owned_scratch=str(scratch),retained_large_artifacts=[]),indent=2)+'\n')
    return int(not passed)


if __name__=='__main__':raise SystemExit(main())
