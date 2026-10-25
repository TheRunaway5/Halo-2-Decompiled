#!/usr/bin/env python3
"""Qualify the recovered native BV-agent graph against the untouched witness.

Use the active capture's run_test.py owner. Borrowed native libraries remain
unchanged; all candidate/compiler products belong to this disposable owner.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture-report',type=Path,required=True)
    parser.add_argument('--generation',type=Path,required=True)
    parser.add_argument('--original',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--negative-control',action='store_true')
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    captured=json.loads(args.capture_report.read_text());generation=json.loads(args.generation.read_text())
    original=json.loads(args.original.read_text())
    if Path(captured['owner'])!=owner or not captured['captured'] or not original['original_kernel_returned']:
        raise ValueError('Complete original witness and its active capture owner are required')
    libraries=captured['borrowed_libraries_sha256']
    if any(sha(Path(name))!=pin for name,pin in libraries.items()):raise ValueError('Borrowed library changed')
    fixture=Path(original['fixture']);candidate=owner/'bv-native-prototype/vehicle_bv.cpp'
    if sha(fixture)!=original['fixture_sha256'] or sha(fixture)!=generation['original_witness_sha256']:
        raise ValueError('Original witness changed')
    if sha(candidate)!=generation['generated_cpp_sha256']:raise ValueError('Recovered native body changed')
    args.output.mkdir(parents=True,exist_ok=False);source=owner/'source';pins={}
    for before,after in [(candidate,source/'vehicle_bv.cpp'),
        (ROOT/'tests/vehicle_bv_agent_tu15_tests.cpp',source/'vehicle_bv_test.cpp'),
        (ROOT/'include/halo2/source_vehicle_bv_agent_tu15.hpp',source/'include/halo2/source_vehicle_bv_agent_tu15.hpp')]:
        pin=sha(before);after.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(before,after)
        if sha(after)!=pin or sha(before)!=pin:raise ValueError('Component input changed during copy')
        pins[str(after.relative_to(source))]=pin
    mutation=None
    if args.negative_control:
        path=source/'vehicle_bv.cpp';text=path.read_text();needle='    abi=s.save();\n}'
        if text.count(needle)!=1:raise ValueError('Requalify private BV ABI negative control')
        path.write_text(text.replace(needle,'    abi=s.save();abi.gp[0]^=1u;\n}'))
        mutation=dict(path='vehicle_bv.cpp',before=pins['vehicle_bv.cpp'],after=sha(path),
                      scope='Private final returned-EAX mutation; original input/instructions are unchanged')
        pins['vehicle_bv.cpp']=sha(path)
    cmake=source/'CMakeLists.txt';prior=cmake.read_text();marker='\n# VEHICLE_BV_COMPONENT_TU15\n'
    text=prior.split(marker)[0]
    link=next(line for line in text.splitlines() if line.startswith('target_link_libraries(vehicle_bv_entry_capture PRIVATE '))
    include=next(line for line in text.splitlines() if line.startswith('target_include_directories(vehicle_bv_entry_capture PRIVATE '))
    label=args.output.name;report_name='vehicle-bv-'+label+'.json'
    text+=marker+'add_executable(vehicle_bv_agent_component vehicle_bv_test.cpp vehicle_bv.cpp)\n'
    text+='target_compile_features(vehicle_bv_agent_component PRIVATE cxx_std_20)\n'
    text+='target_compile_options(vehicle_bv_agent_component PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
    text+=include.replace('vehicle_bv_entry_capture','vehicle_bv_agent_component').replace('PRIVATE ','PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" ',1)+'\n'
    text+=link.replace('vehicle_bv_entry_capture','vehicle_bv_agent_component')+'\n'
    text+='add_test(NAME vehicle_bv_original COMMAND "${Python3_EXECUTABLE}" "'+str(ROOT/'tools/run_test.py')+'" --scratch-root "'+str(owner/'ctest-scratch')+'" -- $<TARGET_FILE:vehicle_bv_agent_component> "'+str(fixture)+'" "${CMAKE_CURRENT_BINARY_DIR}/'+report_name+'")\n'
    text+='set_tests_properties(vehicle_bv_original PROPERTIES TIMEOUT 120)\n';cmake.write_text(text)
    pins['CMakeLists.txt']=sha(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
             '--output',str(args.output/'validation'),'--target','vehicle_bv_agent_component',
             '--filter','^vehicle_bv_original$','--jobs','2','--test-jobs','1','--retain-report',report_name]
    result=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT)
    if args.negative_control:
        validation=json.loads((args.output/'validation/report.json').read_text())
        tests=(args.output/'validation/tests.log').read_text()
        passed=result.returncode!=0 and all(row['exit_code']==0 for row in validation['steps'] if row['name']!='tests') and 'Native BV query changed original integer/SSE/FS ABI' in tests
    else:passed=result.returncode==0
    unchanged=all(sha(Path(name))==pin for name,pin in libraries.items())
    frozen=all(sha(source/name)==pin for name,pin in pins.items())
    report=dict(passed=passed and unchanged and frozen,owner=str(owner),command=command,
        validation_returncode=result.returncode,source_sha256=pins,borrowed_libraries_unchanged=unchanged,
        component_source_unchanged=frozen,original_fixture_sha256=sha(fixture),original_report_sha256=sha(args.original),
        generation_sha256=sha(args.generation),negative_control=mutation,full_vehicle_parity=False,
        native_expected_output_used=False,production_emulator=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],report=str(args.output/'report.json'))),flush=True)
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
