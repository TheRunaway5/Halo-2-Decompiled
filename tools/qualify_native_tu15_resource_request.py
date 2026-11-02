#!/usr/bin/env python3
"""Freeze renderer resource policy/publication kernel owners and validate with disposable CMake builds."""
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
    names=['tests/native_tu15_resource_request_tests.cpp','tools/validate_native_tu15_resource_request.py',
        'tools/observe_native_campaign_ai_vector_math_tu15.cpp','cmake/NativeTu15ResourceRequest.cmake',
        'src/rendering/source_geometry_resource_request.cpp','src/rendering/source_geometry_resource.cpp',
        'include/halo2/source_geometry_resource_request.hpp','include/halo2/source_geometry_resource.hpp']
    pins={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()for n in names}
    for name in names:
        target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    (source/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(NativeResourceRequest LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include("{ROOT}/cmake/TestStorage.cmake")
set(HALO2_RENDERER_ORACLE_PYTHON "{ROOT}/.tools/python/bin/python")
set(HALO2_RESOURCE_REQUEST_ORACLE_SCRIPT "{ROOT}/tools/validate_native_tu15_resource_request.py")
set(HALO2_TU15_GAMEPLAY_XBE "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
include(cmake/NativeTu15ResourceRequest.cmake)
''')
    results=[]
    for kind,compiler,sanitize in [('gcc','g++',False),('clang-sanitizers','clang++',True)]:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'builds'),
            '--output',str(a.output/kind),'--target','native_tu15_resource_request_tests','--filter','^(native_tu15_resource_request_controls|original_tu15_native_resource_request)$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native-tu15-resource-request.json']
        if sanitize:command+=['--sanitizers']
        run=subprocess.run(command);results.append(dict(kind=kind,returncode=run.returncode,command=command))
        if run.returncode:break
    mutants=[]
    if a.mutations and len(results)==2 and all(r['returncode']==0 for r in results):
        variants=[
            ('selected-priority','src/rendering/source_geometry_resource_request.cpp','if(selective)priority=2;','if(selective)priority=7;'),
            ('flag2-selection','src/rendering/source_geometry_resource_request.cpp','loading && !flag2 && !blocking','loading && !blocking'),
            ('stale-physical','src/rendering/source_geometry_resource_request.cpp','services.promote(job,7);physical=word(g.physical);','services.promote(job,7);'),
            ('single-ready','src/rendering/source_geometry_resource_request.cpp','blocking && !byte(inflight+2) && !byte(inflight+2)','blocking && !byte(inflight+2)'),
            ('publication-stride','src/rendering/source_geometry_resource.cpp','kind==0?read<std::uint16_t>(m,descriptor+6):1u','1u'),
            ('selector-order','src/rendering/source_geometry_resource.cpp','const auto selector=read<std::uint16_t>(m,descriptor+6);\n        const auto locator=read<std::uint16_t>(m,descriptor+4);','const auto locator=read<std::uint16_t>(m,descriptor+4);\n        const auto selector=read<std::uint16_t>(m,descriptor+6);')]
        for label,name,before,after in variants:
            kernel=source/name;original=kernel.read_text();assert original.count(before)==1;kernel.write_text(original.replace(before,after))
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'mutants'),
                '--output',str(a.output/('mutant-'+label)),'--target','native_tu15_resource_request_tests',
                '--filter','^(native_tu15_resource_request_controls|original_tu15_native_resource_request)$','--jobs','2','--test-jobs','1']
            run=subprocess.run(command);receipt=json.loads((a.output/('mutant-'+label)/'report.json').read_text())
            steps={s['name']:s['exit_code']for s in receipt['steps']}
            killed=run.returncode!=0 and steps.get('configure')==0 and steps.get('build')==0 and steps.get('tests',0)!=0
            mutants.append(dict(name=label,returncode=run.returncode,killed=killed,command=command));kernel.write_text(original)
    unchanged=all(hashlib.sha256((ROOT/n).read_bytes()).hexdigest()==pin for n,pin in pins.items())
    passed=len(results)==2 and all(r['returncode']==0 for r in results)and unchanged and (not a.mutations or len(mutants)==6 and all(m['killed']for m in mutants))
    (a.output/'validation.json').write_text(json.dumps(dict(passed=passed,results=results,mutations=mutants,source_sha256=pins,source_unchanged=unchanged,
        owned_scratch=str(scratch),retained_large_artifacts=[]),indent=2)+'\n');return int(not passed)


if __name__=='__main__':raise SystemExit(main())
