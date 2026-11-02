#!/usr/bin/env python3
"""Freeze the cube-camera visibility owner and use disposable CMake validation."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import shlex
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
NAMES=['tests/native_tu15_large_sphere_visibility_tests.cpp','cmake/NativeTu15LargeSphereVisibility.cmake',
       'src/rendering/world/source_portal_visibility.cpp','src/rendering/world/source_portal_frustum.cpp',
       'src/rendering/world/source_portal_clip.cpp','src/rendering/world/source_frustum_camera.cpp',
       'src/rendering/world/source_visibility.cpp','src/rendering/shadows/source_shadow_view.cpp','src/core/transform.cpp',
       'tools/differential_source_portal_visibility.py','tools/audit_rendering.py','tools/donor_tree.py',
       'tools/reference/halo2-decompiled/tools/xbe.py','tools/qualify_native_tu15_large_sphere_visibility.py']


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--gcc-only',action='store_true');parser.add_argument('--clang-only',action='store_true');a=parser.parse_args()
    if a.gcc_only and a.clang_only:parser.error('Choose at most one compiler restriction')
    cohorts=([('gcc','g++',False)]if a.gcc_only else[('clang-sanitizers','clang++',True)]if a.clang_only else[('gcc','g++',False),('clang-sanitizers','clang++',True)])
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=scratch/'source';source.mkdir();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=True)
    sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
    all_headers={p.relative_to(ROOT).as_posix():sha(p)for p in sorted((ROOT/'include').rglob('*'))if p.is_file()}
    shutil.copytree(ROOT/'include',source/'include')
    for name in NAMES:
        target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    (source/'assets').symlink_to(ROOT/'assets',target_is_directory=True)
    (source/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(NativeLargeSphereVisibility LANGUAGES CXX)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include("{ROOT}/cmake/TestStorage.cmake")
set(HALO2_RENDERER_ORACLE_PYTHON "{ROOT}/.tools/python/bin/python")
include(cmake/NativeTu15LargeSphereVisibility.cmake)
''')
    # Retain the actual compiler dependency closure. Unrelated active headers
    # can change while a long original-code corpus executes.
    dependencies=set(NAMES)
    for _,compiler,_ in cohorts:
        for name in NAMES:
            if not name.endswith('.cpp'):continue
            command=[compiler,'-std=c++20','-MM','-I'+str(source/'include'),str(source/name)]
            result=subprocess.run(command,capture_output=True,text=True,check=True)
            for token in shlex.split(result.stdout.replace('\\\n',' ').split(':',1)[1]):
                path=Path(token).resolve()
                if path.is_relative_to(source):dependencies.add(path.relative_to(source).as_posix())
    pins={name:sha(source/name)for name in sorted(dependencies)}
    results=[]
    for kind,compiler,sanitizers in cohorts:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'builds'),
            '--output',str(a.output/kind),'--target','native_tu15_large_sphere_visibility_tests','--filter','^original_tu15_large_sphere_visibility$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native-tu15-large-sphere-visibility.json',
            '--retain-report','Testing/Temporary/LastTest.log']
        if sanitizers:command.append('--sanitizers')
        result=subprocess.run(command);receipt=json.loads((a.output/kind/'report.json').read_text())
        results.append(dict(kind=kind,command=command,exit_code=result.returncode,steps={s['name']:s['exit_code']for s in receipt['steps']}))
        if result.returncode:break
    unchanged=all(sha(ROOT/n)==h for n,h in pins.items())
    passed=all(not r['exit_code']for r in results)and len(results)==len(cohorts)and unchanged
    report=dict(passed=passed,results=results,source_sha256=pins,source_unchanged=unchanged,
        unused_header_drift=[n for n,h in all_headers.items()if n not in dependencies and sha(ROOT/n)!=h],
        owned_scratch=str(scratch),retained_large_artifacts=[],
        scope='Cube-camera and native portal traversal component; no large-sphere pipeline admission or rendered-frame acceptance.')
    (a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not passed)


if __name__=='__main__':raise SystemExit(main())
