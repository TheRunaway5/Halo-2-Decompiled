#!/usr/bin/env python3
"""Freeze and validate the title component, including original-backed mutants.

Run through run_test.py. CMake validation uses run_tests.py and all source,
objects, executables and expanded references stay in its owned scratch tree.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--sanitizers',action='store_true');a=p.parse_args()
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not a.output.resolve().is_relative_to(scratch)
    project=scratch/'project';project.mkdir();shutil.copytree(root/'include',project/'include')
    paths=['cmake/TestStorage.cmake','cmake/WorkshopTitles.cmake','tools/run_test.py',
        'src/campaign/source_workshop_titles_tu15_generated.cpp',
        'tests/workshop_title_tests.cpp','tests/workshop_title_lifetime_tests.cpp','tests/workshop_title_tests.py']
    paths += [str(f.relative_to(root)) for f in (root/'src/physics/runtime').glob('*.hpp')]
    paths += [str(f.relative_to(root)) for f in (root/'tests/fixtures').glob('workshop_title*_tu15.json*')]
    pins={name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in paths}
    for name in paths:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(root/name,target)
    source=(project/'src/campaign/source_workshop_titles_tu15_generated.cpp').read_text()
    changes=[
        ('wrong_expired_title','title_lifetime','0x13f245u,(0x0u+s.gp[0]),2,0xffffu','0x13f245u,(0x0u+s.gp[0]),2,0xfffeu'),
        ('wrong_expiry_boundary','title_lifetime','if(s.flags.cf)goto L13f250;','if(!s.flags.cf)goto L13f250;'),
        ('wrong_delayed_counter','title','0x13ed20u,(0xau+s.gp[1]+s.gp[0]*4u),2,s.reg(2,2,0)',
            '0x13ed20u,(0xau+s.gp[1]+s.gp[0]*4u),2,s.reg(2,2,0)^1u'),
        ('missing_lifetime','title_lifetime','case 0x13ecc0u:case 0x13f160u:case 0x2b3620u:return true;',
            'case 0x13ecc0u:case 0x2b3620u:return true;')]
    for name,kind,needle,replacement in changes:
        assert source.count(needle)==1,(name,source.count(needle))
        (project/f'{name}.cpp').write_text(source.replace(needle,replacement))
    (project/'negative.py').write_text('''import subprocess,sys
p=subprocess.run(sys.argv[1:],capture_output=True,text=True)
assert p.returncode!=0 and any(message in p.stderr for message in
 ('Native title lifecycle state differs','Native title lifecycle ordered stores differ',
  'Native delayed title slot or timing differs','No whole native title lifecycle owner')),(p.returncode,p.stdout,p.stderr)
print('Expected original-backed title mutation rejection:',p.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(WorkshopTitles LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(cmake/TestStorage.cmake)
if(HALO2_SANITIZERS)
add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
add_link_options(-fsanitize=address,undefined)
endif()
include(cmake/WorkshopTitles.cmake)
'''
    if not a.sanitizers:
        for name,kind,_,_ in changes:
            cmake+=f'''add_executable({name} {name}.cpp tests/workshop_{kind}_tests.cpp)
target_include_directories({name} PRIVATE include src)
target_compile_features({name} PRIVATE cxx_std_20)
target_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries({name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME workshop_title_mutant_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py"
 "${{Python3_EXECUTABLE}}" -B "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/workshop_title_tests.py"
 --fixture "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/workshop_{kind}_tu15.json.zlib" --test-executable "$<TARGET_FILE:{name}>")
'''
    (project/'CMakeLists.txt').write_text(cmake)
    argv=[sys.executable,str(root/'tools/run_tests.py'),'--source',str(project),
        '--scratch-root',str(scratch/'builds'),'--output',str(a.output/'cmake'),
        '--target','workshop_title_tests','--target','workshop_title_lifetime_tests',
        '--filter','^workshop_title','--jobs','2','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log']
    if a.sanitizers:argv+=['--sanitizers','--cmake-arg=-DCMAKE_CXX_COMPILER=clang++']
    else:
        for name,_,_,_ in changes:argv+=['--target',name]
    run=subprocess.run(argv);unchanged=all(hashlib.sha256((root/name).read_bytes()).hexdigest()==value for name,value in pins.items())
    report=dict(passed=run.returncode==0 and unchanged,exit_code=run.returncode,source_hashes=pins,
        sources_unchanged=unchanged,sanitizers=a.sanitizers,negative_controls=[] if a.sanitizers else [v[0] for v in changes],
        command=argv,scratch=str(scratch),runtime_enabled=False,retained_large_artifacts=[])
    a.output.mkdir(parents=True,exist_ok=True);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
