#!/usr/bin/env python3
"""Capture an actual Midship scheduler input using an authenticated owned build.

Run inside tools/run_test.py, optionally its finite command session. Existing
native libraries are read-only inputs. All new binaries and raw entry banks
remain in this owner; bounded provenance is the only external output.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepared',type=Path,required=True)
    parser.add_argument('--cohort',type=Path,required=True)
    parser.add_argument('--refresh',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    prepared=json.loads(args.prepared.read_text());cohort=json.loads(args.cohort.read_text())
    refresh=json.loads(args.refresh.read_text());borrowed=Path(cohort['owner']);build=borrowed/'build'
    frozen=borrowed/'source'
    if not borrowed.name.startswith('halo2-test-') or not borrowed.is_dir() or borrowed==owner:
        raise ValueError('An independent active owned source/build is required')
    if Path(prepared['runner_scratch'])!=borrowed or Path(refresh['owner'])!=borrowed:
        raise ValueError('Borrowed cohort identity differs')
    expected=dict(prepared['registered_snapshot_source_files_sha256'])
    for change in cohort['changes']:expected[change['path']]=change['after']
    expected.update({'data/'+name:pin for name,pin in cohort.get('private_data_sha256',{}).items()})
    expected.update(refresh['refreshed_source_sha256'])
    if any(sha(frozen/name)!=pin for name,pin in expected.items()):
        raise ValueError('Borrowed frozen source has undeclared changes')
    args.output.mkdir(parents=True,exist_ok=False)
    source=owner/'source';source.mkdir(exist_ok=False)
    copied={}
    for name,target in [('tools/capture_vehicle_bv_entry_tu15.cpp','capture.cpp'),
                        ('tests/helpers/live_physics_entry_capture_tu15.hpp','helpers/live_physics_entry_capture_tu15.hpp')]:
        pin=sha(ROOT/name);path=source/target;path.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(ROOT/name,path)
        if sha(path)!=pin or sha(ROOT/name)!=pin:raise ValueError('Capture helper changed during copy')
        copied[name]=pin
    link_file=build/'CMakeFiles/vehicle_variants_world_tu15_tests.dir/link.txt'
    link=shlex.split(link_file.read_text());libraries=link[link.index('-o')+2:]
    pins={};resolved=[]
    for name in libraries:
        if name.startswith('-'):resolved.append(name);continue
        path=(build/name).resolve(strict=True);pins[str(path)]=sha(path);resolved.append(str(path))
    cmake_quote=lambda value:'"'+str(value).replace('\\','/').replace('"','\\"')+'"'
    maps=Path(prepared['runtime_cache_inputs']['maps_directory'])
    captured=owner/'midship-player-entry'
    cmake='''cmake_minimum_required(VERSION 3.20)
project(VehicleBvEntryCapture LANGUAGES CXX)
enable_testing()
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
add_executable(vehicle_bv_entry_capture capture.cpp)
target_compile_features(vehicle_bv_entry_capture PRIVATE cxx_std_20)
target_compile_options(vehicle_bv_entry_capture PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
'''
    cmake+='target_include_directories(vehicle_bv_entry_capture PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS} '+cmake_quote(frozen/'include')+' '+cmake_quote(frozen/'src')+')\n'
    cmake+='target_link_libraries(vehicle_bv_entry_capture PRIVATE "-Wl,--start-group" '+' '.join(map(cmake_quote,resolved))+' "-Wl,--end-group")\n'
    cmake+='add_test(NAME actual_midship_player_entry_capture COMMAND "${Python3_EXECUTABLE}" '+cmake_quote(ROOT/'tools/run_test.py')+' --scratch-root '+cmake_quote(owner/'ctest-scratch')+' -- $<TARGET_FILE:vehicle_bv_entry_capture> '+cmake_quote(maps/'midship.map')+' '+cmake_quote(maps)+' '+cmake_quote(captured)+')\n'
    cmake+='set_tests_properties(actual_midship_player_entry_capture PROPERTIES TIMEOUT 120)\n'
    (source/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
             '--output',str(args.output/'validation'),'--target','vehicle_bv_entry_capture',
             '--filter','^actual_midship_player_entry_capture$','--jobs','2','--test-jobs','1']
    report=dict(captured=False,gameplay_or_original_parity=False,owner=str(owner),
                borrowed_owner=str(borrowed),borrowed_libraries_sha256=pins,
                borrowed_link_command_sha256=sha(link_file),frozen_source_authenticated=True,
                frozen_source_receipts={str(p):sha(p) for p in (args.prepared,args.cohort,args.refresh)},
                copied_helpers_sha256=copied,command=command,capture=str(captured),
                boundary='Read-only actual scheduler input; deliberate stop before execution; no supplied admission or return')
    receipt=args.output/'report.json';receipt.write_text(json.dumps(report,indent=2)+'\n')
    result=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT)
    entry=captured/'capture-entry.json'
    unchanged=all(sha(Path(name))==pin for name,pin in pins.items())
    report.update(captured=result.returncode==0 and entry.is_file(),validation_returncode=result.returncode,
                  borrowed_libraries_unchanged=unchanged)
    if entry.exists():
        record=json.loads(entry.read_text());report.update(entry_sha256=sha(entry),
            raw_input_bytes=sum(row['size'] for row in record['regions']),regions=len(record['regions']))
    receipt.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(captured=report['captured'],raw_input_bytes=report.get('raw_input_bytes'),report=str(receipt))),flush=True)
    return 0 if report['captured'] and unchanged else 1


if __name__=='__main__':raise SystemExit(main())
