#!/usr/bin/env python3
"""Freeze small native script flag owners, then use disposable CMake validation."""
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
    names=['include/halo2/native_tu15_render_script_flags.hpp','src/rendering/native_tu15_render_script_flags.cpp',
        'tests/native_tu15_render_script_flags_tests.cpp','tools/validate_native_tu15_render_script_flags.py',
        'tools/test_native_tu15_render_script_flags_fixture.py','cmake/NativeTu15RenderScriptFlags.cmake']
    fixture=Path('tests/fixtures/native_tu15_render_script_flags.json.gz')
    if not a.refresh_fixture:names.append(str(fixture))
    pins={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()for n in names}
    for name in names:
        target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    (source/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(NativeRenderScriptFlags LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include("{ROOT}/cmake/TestStorage.cmake")
set(HALO2_RENDERER_ORACLE_PYTHON "{ROOT}/.tools/python/bin/python")
set(HALO2_TU15_GAMEPLAY_XBE "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
set(HALO2_RENDER_SCRIPT_FLAGS_ORACLE_SCRIPT "{ROOT}/tools/validate_native_tu15_render_script_flags.py")
include(cmake/NativeTu15RenderScriptFlags.cmake)
''')
    results=[]
    for kind,compiler,sanitize in [('gcc','g++',False),('clang-sanitizers','clang++',True)]:
        selected='^original_tu15_native_render_script_flags$'if a.refresh_fixture and kind=='gcc'else'^(original_tu15_native_render_script_flags|native_tu15_render_script_flags_fixture)$'
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'builds'),
            '--output',str(a.output/kind),'--target','native_tu15_render_script_flags_tests','--filter',selected,'--jobs','2','--test-jobs','1',
            '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native-tu15-render-script-flags.json']
        if sanitize:command+=['--sanitizers']
        if a.refresh_fixture:command+=['--cmake-arg=-DHALO2_RENDER_SCRIPT_FLAGS_FIXTURE_OUTPUT='+str(ROOT/fixture)]
        run=subprocess.run(command);results.append(dict(kind=kind,returncode=run.returncode,command=command))
        if run.returncode:break
        if a.refresh_fixture:
            target=source/fixture;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/fixture,target)
    unchanged=all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==pin for n,pin in pins.items())
    passed=len(results)==2 and all(r['returncode']==0 for r in results)and unchanged
    (a.output/'validation.json').write_text(json.dumps(dict(passed=passed,results=results,source_sha256=pins,source_unchanged=unchanged,
        required_reference_fixture=dict(path=str(fixture),bytes=(ROOT/fixture).stat().st_size,sha256=hashlib.sha256((ROOT/fixture).read_bytes()).hexdigest())if (ROOT/fixture).is_file()else None,
        owned_scratch=str(scratch),retained_large_artifacts=[]),indent=2)+'\n')
    return int(not passed)


if __name__=='__main__':raise SystemExit(main())
