#!/usr/bin/env python3
"""Audit material inputs with the actual owned native Cache/delivery libraries."""
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


def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--admission',required=True,type=Path)
    parser.add_argument('--authored-inputs',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    admission=json.loads(args.admission.read_text());frozen=Path(admission['owner']).resolve(strict=True)
    if frozen==owner:raise ValueError('Use a separate read-only component owner')
    build=frozen/'build';inputs=frozen/'source'
    if any(sha(inputs/name)!=row['after']for name,row in admission['changes'].items()):raise ValueError('Frozen material source changed')
    authored=json.loads(args.authored_inputs.read_text());reference_pins={}
    for row in authored['maps']:
        for field,pin in (('path','sha256'),('mcc_path','mcc_sha256')):
            name=row['source'][field];reference_pins[name]=row['source'][pin]
    shared=authored['mcc_shared_globals'];reference_pins[shared['path']]=shared['sha256']
    if any(sha(path)!=pin for path,pin in reference_pins.items()):raise ValueError('Authored material reference changed')
    link=shlex.split((build/'CMakeFiles/mcc_gameplay_scalar_tests.dir/link.txt').read_text());libraries=[];library_pins={}
    for token in link[link.index('-o')+2:]:
        if token.startswith('-')or any(c in token for c in '\n\r";$'):raise ValueError('Unexpected native Cache link syntax')
        path=Path(token)if Path(token).is_absolute()else build/token
        libraries.append(str(path));library_pins[str(path)]=sha(path)
    pins={}
    files=[*(inputs/'include').rglob('*'),inputs/'src/core/kernels.cpp']
    for path in files:
        if not path.is_file():continue
        if path.is_symlink():raise ValueError('Source header is an external link')
        name=path.relative_to(inputs);target=source/name;target.parent.mkdir(parents=True,exist_ok=True)
        before=sha(path);shutil.copyfile(path,target)
        if sha(target)!=before or sha(path)!=before:raise ValueError('Frozen native source changed')
        pins[str(name)]=before
    name='tools/audit_destruction_material_native.cpp';target=source/name;target.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(ROOT/name,target);pins[name]=sha(target)
    cmake='''cmake_minimum_required(VERSION 3.20)
project(Halo2NativeMaterialInputs LANGUAGES CXX)
enable_testing()
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_executable(audit tools/audit_destruction_material_native.cpp src/core/kernels.cpp)
target_compile_features(audit PRIVATE cxx_std_20)
target_include_directories(audit PRIVATE include)
target_compile_options(audit PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(audit PRIVATE nlohmann_json::nlohmann_json)
'''
    cmake+='target_link_libraries(audit PRIVATE '+' '.join('"'+x+'"'for x in libraries)+')\n'
    cmake+=f'add_test(NAME native_destruction_material_inputs COMMAND "${{Python3_EXECUTABLE}}" "{ROOT/"tools/run_test.py"}" -- $<TARGET_FILE:audit> "{args.authored_inputs.resolve()}" "${{CMAKE_CURRENT_BINARY_DIR}}/native-material-inputs.json")\n'
    (source/'CMakeLists.txt').write_text(cmake)
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','audit','--filter','^native_destruction_material_inputs$',
        '--jobs','2','--test-jobs','1','--output',str(args.output/'validation'),'--retain-report','native-material-inputs.json']
    report=dict(passed=False,owned_scratch=str(owner),command=command,borrowed_native_libraries=library_pins,source_hashes=pins,
        authored_input_sha256=sha(args.authored_inputs),tool_sha256=sha(__file__),retained_large_artifacts=[],full_goal_complete=False)
    def save():(args.output/'audit.json').write_text(json.dumps(report,indent=2)+'\n')
    save();result=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report.update(returncode=result,libraries_unchanged=all(sha(path)==pin for path,pin in library_pins.items()),
        source_unchanged=all(sha(source/name)==pin for name,pin in pins.items()),references_unchanged=all(sha(path)==pin for path,pin in reference_pins.items()))
    report['passed']=result==0 and report['libraries_unchanged']and report['source_unchanged']and report['references_unchanged'];save()
    print(json.dumps(dict(passed=report['passed'],report=str(args.output/'audit.json'))),flush=True)
    return 0 if report['passed']else result or 1


if __name__=='__main__':raise SystemExit(main())
