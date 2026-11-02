#!/usr/bin/env python3
"""Freeze command kernel owners and validate with disposable CMake builds."""
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
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--mutations',action='store_true');a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=scratch/'source';source.mkdir();a.output.mkdir(parents=True,exist_ok=True)
    shutil.copytree(ROOT/'include',source/'include')
    names=['include/halo2/native_tu15_command_walk.hpp','src/rendering/world/native_tu15_command_walk.cpp',
        'tests/native_tu15_command_walk_tests.cpp','tools/validate_native_tu15_command_walk.py',
        'tools/observe_native_campaign_ai_vector_math_tu15.cpp','cmake/NativeTu15CommandWalk.cmake']
    pins={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()for n in names}
    for name in names:
        target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    (source/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(NativeCommandWalk LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include("{ROOT}/cmake/TestStorage.cmake")
set(HALO2_RENDERER_ORACLE_PYTHON "{ROOT}/.tools/python/bin/python")
set(HALO2_COMMAND_WALK_ORACLE_SCRIPT "{ROOT}/tools/validate_native_tu15_command_walk.py")
set(HALO2_TU15_GAMEPLAY_XBE "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
include(cmake/NativeTu15CommandWalk.cmake)
''')
    results=[]
    for kind,compiler,sanitize in [('gcc','g++',False),('clang-sanitizers','clang++',True)]:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'builds'),
            '--output',str(a.output/kind),'--target','native_tu15_command_walk_tests','--filter','^(native_tu15_command_walk_controls|original_tu15_native_command_walk)$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native-tu15-command-walk.json']
        if sanitize:command+=['--sanitizers']
        run=subprocess.run(command);results.append(dict(kind=kind,returncode=run.returncode,command=command))
        if run.returncode:break
    mutants=[]
    if a.mutations and len(results)==2 and all(r['returncode']==0 for r in results):
        kernel=source/'src/rendering/world/native_tu15_command_walk.cpp';original=kernel.read_text()
        for label,before,after in [
            ('full-prefix','mode?std::uint16_t(prefix):0u','mode?prefix:0u'),
            ('unsigned-first','first=signed_half(read(m,p+0xau,2))','first=std::int32_t(std::uint16_t(read(m,p+0xau,2)))'),
            ('reverse-copy','for(unsigned i=0;i<8;++i)write','for(unsigned i=8;i--;)write'),
            ('reset-order','0x4ce26cu,0x4ce274u,0x4ce270u','0x4ce26cu,0x4ce270u,0x4ce274u'),
            ('pinned-count','const auto count=read(m,geometry);++part;','const auto count=2u;++part;')]:
            assert original.count(before)==1;kernel.write_text(original.replace(before,after))
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'mutants'),
                '--output',str(a.output/('mutant-'+label)),'--target','native_tu15_command_walk_tests',
                '--filter','^(native_tu15_command_walk_controls|original_tu15_native_command_walk)$','--jobs','2','--test-jobs','1']
            run=subprocess.run(command);receipt=json.loads((a.output/('mutant-'+label)/'report.json').read_text())
            steps={s['name']:s['exit_code']for s in receipt['steps']}
            killed=run.returncode!=0 and steps.get('configure')==0 and steps.get('build')==0 and steps.get('tests',0)!=0
            mutants.append(dict(name=label,returncode=run.returncode,killed=killed,command=command))
        kernel.write_text(original)
    unchanged=all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==pin for n,pin in pins.items())
    passed=len(results)==2 and all(r['returncode']==0 for r in results)and unchanged and (not a.mutations or len(mutants)==5 and all(m['killed']for m in mutants))
    (a.output/'validation.json').write_text(json.dumps(dict(passed=passed,results=results,mutations=mutants,source_sha256=pins,source_unchanged=unchanged,
        owned_scratch=str(scratch),retained_large_artifacts=[]),indent=2)+'\n');return int(not passed)


if __name__=='__main__':raise SystemExit(main())
