#!/usr/bin/env python3
"""Inspect actual canonical sound metadata using this task's still-owned build.

No game, resource binding or original executable is run. Headers, archives and
responses are borrowed read-only and hashed before/after; the caller must keep
that build owner alive. Compiler products belong to this run_test.py owner.
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
    parser.add_argument('--borrowed-owner',type=Path,required=True)
    parser.add_argument('--maps',type=Path,required=True)
    parser.add_argument('--map',action='append',required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.maps=args.maps.resolve(strict=True);args.output=args.output.resolve()
    borrowed=args.borrowed_owner.resolve(strict=True);headers=borrowed/'source';build=borrowed/'build'
    if borrowed==owner or borrowed.parent!=(ROOT/'.local/test-scratch').resolve():
        raise ValueError('Use a separate still-owned build for read-only metadata inspection')
    source=owner/'source';source.mkdir();args.output.mkdir(parents=True,exist_ok=False)
    cpp=ROOT/'tools/inspect_vehicle_flyby_native_metadata_tu15.cpp';cpp_pin=sha(cpp)
    shutil.copyfile(cpp,source/'inspect.cpp')
    if sha(source/'inspect.cpp')!=cpp_pin:raise ValueError('Inspection source changed')
    header_pins={str(p):sha(p) for p in (headers/'include').rglob('*') if p.is_file()}
    linkfile=build/'CMakeFiles/inspect_mcc_runtime_delivery.dir/link.txt';link_pin=sha(linkfile)
    link=shlex.split(linkfile.read_text());libraries=[];library_pins={}
    for token in link[link.index('-o')+2:]:
        if token.startswith('-Wl,@'):
            path=Path(token[6:]);library_pins[str(path)]=sha(path);libraries.append(token);continue
        if token.startswith('-'):libraries.append(token);continue
        path=(build/token).resolve(strict=True);library_pins[str(path)]=sha(path);libraries.append(str(path))
    q=lambda value:'"'+str(value).replace('\\','/').replace('"','\\"')+'"'
    cmake='''cmake_minimum_required(VERSION 3.20)
project(VehicleFlybyMetadata LANGUAGES CXX)
enable_testing()
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
add_executable(inspect_metadata inspect.cpp)
target_compile_features(inspect_metadata PRIVATE cxx_std_20)
target_compile_options(inspect_metadata PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
'''
    cmake+='target_include_directories(inspect_metadata PRIVATE ${SDL2_INCLUDE_DIRS} '+q(headers/'include')+' '+q(headers/'src')+')\n'
    cmake+='target_link_libraries(inspect_metadata PRIVATE "-Wl,--start-group" '+' '.join(map(q,libraries))+' "-Wl,--end-group")\n'
    map_pins={}
    for n,name in enumerate(args.map):
        if Path(name).name!=name or not name.endswith('.map'):raise ValueError('Use one actual MCC map filename')
        path=args.maps/name;map_pins[str(path)]=sha(path)
        cmake+='add_test(NAME inspect_flyby_'+str(n)+' COMMAND inspect_metadata '+q(path)+' '+q(args.maps)+' '+q(args.output/(name+'.json'))+')\n'
    (source/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
             '--target','inspect_metadata','--filter','^inspect_flyby_','--jobs','2','--test-jobs','1',
             '--output',str(args.output/'validation')]
    result=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    unchanged=(sha(cpp)==cpp_pin and sha(linkfile)==link_pin and
        all(sha(Path(path))==pin for path,pin in (header_pins|library_pins|map_pins).items()))
    reports=[args.output/(name+'.json') for name in args.map]
    passed=result==0 and unchanged and all(p.exists() and json.loads(p.read_text())['passed'] for p in reports)
    report=dict(passed=passed,owner=str(owner),borrowed_owner=str(borrowed),command=command,
                borrowed_headers=header_pins,borrowed_libraries=library_pins,map_pins=map_pins,
                source_sha256=cpp_pin,link_sha256=link_pin,borrowed_inputs_unchanged=unchanged,
                validation_returncode=result,runtime_sound_binding_changed=False,full_vehicle_parity=False,
                retained_large_artifacts=[],reports=[dict(path=str(p),sha256=sha(p)) for p in reports if p.exists()])
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=passed,report=str(args.output/'report.json'))),flush=True)
    return 0 if passed else result or 1


if __name__=='__main__':raise SystemExit(main())
