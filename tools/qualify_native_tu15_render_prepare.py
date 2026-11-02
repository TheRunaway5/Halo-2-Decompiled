#!/usr/bin/env python3
"""Freeze renderer preparation/admission kernel owners and validate with disposable CMake builds."""
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
    names=['include/halo2/native_tu15_render_prepare.hpp','src/rendering/world/native_tu15_render_prepare.cpp',
        'tests/native_tu15_render_prepare_tests.cpp','tools/validate_native_tu15_render_prepare.py',
        'tools/observe_native_campaign_ai_vector_math_tu15.cpp','cmake/NativeTu15RenderPrepare.cmake',
        'src/rendering/source_geometry_resource_request.cpp','src/rendering/source_geometry_resource.cpp',
        'include/halo2/source_geometry_resource_request.hpp','include/halo2/source_geometry_resource.hpp']
    pins={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()for n in names}
    for name in names:
        target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    (source/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(NativeRenderPrepare LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include("{ROOT}/cmake/TestStorage.cmake")
set(HALO2_RENDERER_ORACLE_PYTHON "{ROOT}/.tools/python/bin/python")
set(HALO2_RENDER_PREPARE_ORACLE_SCRIPT "{ROOT}/tools/validate_native_tu15_render_prepare.py")
set(HALO2_TU15_GAMEPLAY_XBE "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
include(cmake/NativeTu15RenderPrepare.cmake)
''')
    results=[]
    for kind,compiler,sanitize in [('gcc','g++',False),('clang-sanitizers','clang++',True)]:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'builds'),
            '--output',str(a.output/kind),'--target','native_tu15_render_prepare_tests','--filter','^(native_tu15_render_prepare_controls|original_tu15_native_render_prepare)$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native-tu15-render-prepare.json']
        if sanitize:command+=['--sanitizers']
        run=subprocess.run(command);results.append(dict(kind=kind,returncode=run.returncode,command=command))
        if run.returncode:break
    mutants=[]
    if a.mutations and len(results)==2 and all(r['returncode']==0 for r in results):
        kernel=source/'src/rendering/world/native_tu15_render_prepare.cpp';original=kernel.read_text()
        for label,before,after in [
            ('unsigned-unit','32u*std::uint32_t(signed_half(unit))','32u*std::uint32_t(std::uint16_t(unit))'),
            ('unsigned-definition','std::uint32_t(signed_half(read(m,instances+88u*((packed>>18)&2047u)+0x34u,2)))','std::uint32_t(std::uint16_t(read(m,instances+88u*((packed>>18)&2047u)+0x34u,2)))'),
            ('stale-queue','queue=read(m,context+0x14u);\n        const auto count','queue=queue;\n        const auto count'),
            ('missing-child','if(child!=0xffffffffu)','if(false && child!=0xffffffffu)'),
            ('wrong-model-header','header_offset=0x38u','header_offset=0x28u')]:
            assert original.count(before)==1;kernel.write_text(original.replace(before,after))
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'mutants'),
                '--output',str(a.output/('mutant-'+label)),'--target','native_tu15_render_prepare_tests',
                '--filter','^(native_tu15_render_prepare_controls|original_tu15_native_render_prepare)$','--jobs','2','--test-jobs','1']
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
