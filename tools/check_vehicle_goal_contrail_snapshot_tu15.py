#!/usr/bin/env python3
"""Check native contrail snapshot changes against a read-only current build.

Only the focused test and snapshot implementation are privately rebuilt. The
existing native libraries are authenticated before/after; run_test.py owns all
generated files and run_tests.py performs CMake validation.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys

from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('prepared','cohort','refresh','xbe','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--fade-curves',action='store_true',help='Check the complete native transition bank against the original fixture')
    parser.add_argument('--corrupt-curve-byte',action='store_true',help='Require rejection of a private one-bit change at the observed Wraith lookup')
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.corrupt_curve_byte and not args.fade_curves:parser.error('Curve corruption requires the transition-bank check')
    prepared=json.loads(args.prepared.read_text());cohort=json.loads(args.cohort.read_text())
    refresh=json.loads(args.refresh.read_text());borrowed=Path(cohort['owner']);frozen=borrowed/'source'
    if owner==borrowed or Path(prepared['runner_scratch'])!=borrowed or Path(refresh['owner'])!=borrowed:
        raise ValueError('Separate authenticated current native build owner required')
    if sha(args.xbe)!='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd':
        raise ValueError('Original English TU1.5 input differs')
    expected=dict(prepared['registered_snapshot_source_files_sha256'])
    for row in cohort['changes']:
        if expected[row['path']]!=row['before']:raise ValueError('Cohort preimage differs')
        expected[row['path']]=row['after']
    expected.update({'data/'+name:pin for name,pin in cohort['private_data_sha256'].items()})
    expected.update(refresh['refreshed_source_sha256'])
    if any(sha(frozen/name)!=pin for name,pin in expected.items()):raise ValueError('Frozen native source drifted')
    source=owner/'source';source.mkdir();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    copies={}
    names=('tests/light_fade_curves_tu15_tests.cpp','src/physics/native_tu15_physics_branch_data.cpp') if args.fade_curves else (
        'tests/source_contrail_snapshot_tests.cpp','src/rendering/effects/source_contrail_snapshot.cpp')
    for name in names:
        path=source/Path(name).name;pin=sha(ROOT/name);shutil.copyfile(ROOT/name,path)
        if sha(path)!=pin or sha(ROOT/name)!=pin:raise ValueError('Focused input changed while copying')
        copies[name]=pin
    extra=[];mutation=None
    if args.fade_curves:
        for name in ('tests/fixtures/light_fade_curves_tu15_original.json','src/physics/native_tu15_fade_curve_data.inc'):
            path=ROOT/name
            if not path.exists():continue
            target=source/('original-curves.json' if path.suffix=='.json' else 'physics/'+path.name)
            target.parent.mkdir(parents=True,exist_ok=True);pin=sha(path);shutil.copyfile(path,target)
            if sha(target)!=pin or sha(path)!=pin:raise ValueError('Curve reference input changed while copying')
            copies[name]=pin
        extra=[str(source/'original-curves.json'),str(ROOT/'assets/disc/maps/lockout.map'),str(ROOT/'assets/disc/maps')]
        if args.corrupt_curve_byte:
            path=source/'physics/native_tu15_fade_curve_data.inc';text=path.read_text()
            tokens=list(re.finditer(r'std::byte\{0x([0-9a-f]{2})\}',text));index=0x478b38-0x4786a8
            if len(tokens)!=7168:raise ValueError('Requalify the private transition-data mutation')
            token=tokens[index];value=int(token[1],16);before=sha(path)
            text=text[:token.start()]+f'std::byte{{0x{value^1:02x}}}'+text[token.end():];path.write_text(text)
            mutation=dict(address=0x478b38,before_byte=value,after_byte=value^1,
                private_source_before_sha256=before,private_source_after_sha256=sha(path),original_fixture_changed=False)
    build=borrowed/'build';link_file=build/'CMakeFiles/native_tu15_world_memory_tests.dir/link.txt'
    link=shlex.split(link_file.read_text());at=link.index('-o');pins={};resolved=[]
    for name in link[at+2:]:
        if name.startswith('-'):resolved.append(name);continue
        path=(build/name).resolve(strict=True);pins[str(path)]=sha(path);resolved.append(str(path))
    q=lambda value:'"'+str(value).replace('\\','/').replace('"','\\"')+'"'
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeContrailSnapshotInputs LANGUAGES CXX)
enable_testing()
find_package(Python3 COMPONENTS Interpreter REQUIRED)
target_compile_features(contrail_snapshot_inputs PRIVATE cxx_std_20)
target_compile_options(contrail_snapshot_inputs PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
'''
    cmake=cmake.replace('target_compile_features(',
        'add_executable(contrail_snapshot_inputs '+' '.join(Path(name).name for name in names)+')\n'
        'target_compile_features(',1)
    cmake+='target_include_directories(contrail_snapshot_inputs PRIVATE '+q(source)+' '+q(frozen/'include')+' '+q(frozen/'src')+')\n'
    cmake+='target_link_libraries(contrail_snapshot_inputs PRIVATE "-Wl,--start-group" '+' '.join(map(q,resolved))+' "-Wl,--end-group")\n'
    cmake+='add_test(NAME original_contrail_snapshot_inputs COMMAND "${Python3_EXECUTABLE}" '+q(ROOT/'tools/run_test.py')+' -- $<TARGET_FILE:contrail_snapshot_inputs> '+' '.join(map(q,extra))+' '+q(args.xbe.resolve())+')\n'
    (source/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--output',str(args.output/'validation'),'--target','contrail_snapshot_inputs',
        '--filter','^original_contrail_snapshot_inputs$','--jobs','2','--test-jobs','1',
        '--retain-report','Testing/Temporary/LastTest.log']
    result=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT)
    unchanged=all(sha(Path(name))==pin for name,pin in pins.items())
    unchanged &= all(sha(frozen/name)==pin for name,pin in expected.items())
    rejection=result.returncode==8 and 'Native transition bank differs from the independent original fixture' in (
        args.output/'validation/tests.log').read_text() if args.corrupt_curve_byte else False
    report=dict(passed=unchanged and (rejection if args.corrupt_curve_byte else result.returncode==0),returncode=result.returncode,owner=str(owner),
        borrowed_owner=str(borrowed),borrowed_libraries_sha256=pins,focused_source_sha256=copies,
        original_XBE_sha256=sha(args.xbe),original_opcode_execution=False,
        mode='transition-bank' if args.fade_curves else 'live-snapshot',
        private_negative_control=mutation,negative_control_rejected=rejection,
        borrowed_inputs_unchanged=unchanged,command=command,full_vehicle_parity=False,
        tool_sha256=sha(Path(__file__)))
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],exit_code=result.returncode,report=str(args.output/'report.json'))),flush=True)
    return 0 if report['passed'] else result.returncode or 1


if __name__=='__main__':raise SystemExit(main())
