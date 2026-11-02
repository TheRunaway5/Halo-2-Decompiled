"""Compare frozen native unit-control evaluators and meaningful mutants.

Run through run_test.py. All compiler trees, expanded references and binaries
belong to its scratch; only bounded CMake evidence leaves the owner.
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
    paths=['cmake/TestStorage.cmake','cmake/WorkshopUnitControls.cmake','tools/run_test.py',
        'src/campaign/scripting/source_workshop_unit_controls_tu15_generated.cpp',
        'tests/workshop_unit_controls_tests.cpp','tests/workshop_unit_controls_tests.py',
        'tests/fixtures/workshop_unit_controls_tu15.json','tests/fixtures/workshop_unit_controls_tu15.json.zlib',
        'src/campaign/scripting/source_workshop_script_builtins_tu15_generated.cpp',
        'include/halo2/source_workshop_unit_controls_tu15.hpp',
        'tools/validate_workshop_unit_controls_tu15.py']
    paths += ['tests/fixtures/workshop_player_action_reset_tu15.json',
        'tests/fixtures/workshop_player_action_reset_tu15.json.zlib']
    paths += [str(f.relative_to(root))for f in (root/'src/physics/runtime').glob('*.hpp')]
    pins={name:hashlib.sha256((root/name).read_bytes()).hexdigest()for name in paths}
    for name in paths:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(root/name,target)
    source=(project/paths[3]).read_text()
    changes=[
        ('wrong_enable_bit','(s.reg(0,4,0))|(0x2u)','(s.reg(0,4,0))|(0x4u)'),
        ('wrong_disable_bit','(s.reg(0,4,0))&(0xfffffffdu)','(s.reg(0,4,0))&(0xfffffffbu)'),
        ('wrong_flag_field','s.flags.plus(s.reg(1,4,0),0x134u,4)','s.flags.plus(s.reg(1,4,0),0x138u,4)'),
        ('wrong_jump_bit','s.flags.right(s.reg(1,4,0),0x1u,4,false)','s.flags.right(s.reg(1,4,0),0x2u,4,false)'),
        ('wrong_boolean_width','r(s.memory,(0x4u+s.gp[0]),1),1,0)','r(s.memory,(0x4u+s.gp[0]),4),1,0)')]
    for name,needle,replacement in changes:
        assert source.count(needle)==1,(name,source.count(needle))
        (project/(name+'.cpp')).write_text(source.replace(needle,replacement))
    old=(project/paths[8]).read_text().replace('source_workshop_script_builtins_tu15','source_workshop_unit_controls_tu15').replace(
        'source_run_workshop_script_builtins_tu15','source_run_workshop_unit_controls_tu15')
    (project/'missing_owner.cpp').write_text(old)
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run(sys.argv[1:],capture_output=True,text=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('Native unit-control','Missing complete native unit-control owner','Unit-control parser/completion ABI differs')),(r.returncode,r.stdout,r.stderr)
print('Expected original-backed rejection:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(WorkshopUnitControls LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(cmake/TestStorage.cmake)
if(HALO2_SANITIZERS)
add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
add_link_options(-fsanitize=address,undefined)
endif()
include(cmake/WorkshopUnitControls.cmake)
'''
    reset_changes=[
        ('wrong_reset_action_field','w(s.memory,s.observe,0x2b2d87u,(0x4u+s.gp[0]),4,s.reg(1,4,0));',
            'w(s.memory,s.observe,0x2b2d87u,(0xcu+s.gp[0]),4,s.reg(1,4,0));'),
        ('missing_reset_armed_store','w(s.memory,s.observe,0x2b2d8au,(0x8u+s.gp[0]),4,s.reg(1,4,0));',';')]
    for name,needle,replacement in reset_changes:
        assert source.count(needle)==1,(name,source.count(needle))
        (project/(name+'.cpp')).write_text(source.replace(needle,replacement))
    variants=[] if a.sanitizers else ['missing_owner']+[x[0]for x in changes+reset_changes]
    for name in variants:
        fixture='workshop_player_action_reset_tu15'if name in {row[0]for row in reset_changes}else 'workshop_unit_controls_tu15'
        cmake+=f'''add_executable({name} {name}.cpp tests/workshop_unit_controls_tests.cpp)
target_include_directories({name} PRIVATE include src)
target_compile_features({name} PRIVATE cxx_std_20)
target_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries({name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME workshop_unit_controls_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py"
 "${{Python3_EXECUTABLE}}" -B "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/workshop_unit_controls_tests.py"
 --fixture "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/{fixture}.json.zlib" --test-executable "$<TARGET_FILE:{name}>")
'''
    (project/'CMakeLists.txt').write_text(cmake)
    results=[]
    for compiler in (('clang++',)if a.sanitizers else ('g++','clang++')):
        command=[sys.executable,str(root/'tools/run_tests.py'),'--source',str(project),
            '--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),
            '--target','workshop_unit_controls_tests','--filter','^workshop_unit_controls',
            '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',
            '--retain-report','Testing/Temporary/LastTest.log']
        if a.sanitizers:command+=['--sanitizers']
        for name in variants:command+=['--target',name]
        run=subprocess.run(command);results.append(dict(compiler=compiler,command=command,exit_code=run.returncode))
    unchanged=all(hashlib.sha256((root/name).read_bytes()).hexdigest()==value for name,value in pins.items())
    report=dict(passed=all(r['exit_code']==0 for r in results)and unchanged,source_hashes=pins,
        sources_unchanged=unchanged,sanitizers=a.sanitizers,negative_controls=variants,results=results,
        runtime_enabled=False,scratch=str(scratch),retained_large_artifacts=[])
    a.output.mkdir(parents=True,exist_ok=True);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
