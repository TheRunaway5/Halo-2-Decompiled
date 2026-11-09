#!/usr/bin/env python3
"""Freeze and qualify native45CD0 with GCC and Clang ASan/UBSan."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
NAMES=['tests/native_tu15_command_part_prepare_tests.cpp','src/rendering/world/native_tu15_command_part_prepare.cpp',
    'cmake/NativeTu15CommandPartPrepare.cmake','tools/qualify_native_tu15_command_part_prepare.py',
    'tools/validate_native_tu15_command_part_prepare.py','tools/oracle_native_tu15_model_mask_order.py',
    'tools/native_tu15_command_part_genuine_inputs.py','tools/oracle_effect_light_snapshot.py',
    'tools/observe_native_campaign_ai_vector_math_tu15.cpp','tools/oracle_native_campaign_screen_lifecycle_tu15.py',
    'tools/reference/halo2-decompiled/tools/xbe.py','tools/reference/halo2-decompiled/tools/functions.py',
    'tools/inspect_multiplayer.py','tools/oracle_smg_firing_1_1.py','tools/research_mp_sandbox_update_functions.py',
    'tools/generate_physics_live_support_tu15.py','tools/validate_native_geometry_tu15.py']


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--gcc-only',action='store_true')
    p.add_argument('--genuine-retained',action='store_true');a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=scratch/'source';source.mkdir();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=True)
    sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
    shutil.copytree(ROOT/'include',source/'include')
    if a.genuine_retained:(source/'analysis').symlink_to(ROOT/'analysis',target_is_directory=True)
    for name in NAMES:
        target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    (source/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(NativeCommandPartPrepare LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include("{ROOT}/cmake/TestStorage.cmake")
set(HALO2_RENDERER_ORACLE_PYTHON "{ROOT}/.tools/python/bin/python")
set(HALO2_TU15_GAMEPLAY_XBE "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
include(cmake/NativeTu15CommandPartPrepare.cmake)
''')
    dependencies=set(NAMES)
    for name in NAMES:
        if not name.endswith('.cpp'):continue
        r=subprocess.run(['g++','-std=c++20','-MM','-I'+str(source/'include'),str(source/name)],capture_output=True,text=True,check=True)
        for token in shlex.split(r.stdout.replace('\\\n',' ').split(':',1)[1]):
            path=Path(token).resolve()
            if path.is_relative_to(source):dependencies.add(path.relative_to(source).as_posix())
    pins={name:sha(source/name)for name in sorted(dependencies)};results=[]
    for kind,compiler,sanitize in [('gcc','g++',False)]+([]if a.gcc_only else[('clang-sanitizers','clang++',True)]):
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'builds'),
            '--output',str(a.output/kind),'--target','native_tu15_command_part_prepare_tests','--filter','^original_tu15_command_part_prepare$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
            '--retain-report','native-tu15-command-part-prepare.json','--retain-report','Testing/Temporary/LastTest.log']
        if sanitize:command.append('--sanitizers')
        if a.genuine_retained:command.append('--cmake-arg=-DHALO2_PART_PREPARE_GENUINE=ON')
        result=subprocess.run(command);results.append(dict(kind=kind,command=command,exit_code=result.returncode))
        if result.returncode:break
    unchanged=all(sha(ROOT/name)==digest for name,digest in pins.items())
    report=dict(passed=unchanged and len(results)==(1 if a.gcc_only else 2)and all(r['exit_code']==0 for r in results),
        results=results,source_sha256=pins,source_unchanged=unchanged,owned_scratch=str(scratch),retained_large_artifacts=[])
    (a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
