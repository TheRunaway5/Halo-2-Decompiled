"""Qualify an additive camera wrapper in the existing native dispatch module.

Preserve every old complete routine exactly and compare the added body with
the independently qualified standalone wrapper. Original ABI fixtures, compiler
and sanitizer checks remain component evidence. Do not admit MCC or camera
effects here. All generated sources and compiler outputs use owned storage.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--frozen-source',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--publish-component',action='store_true');args=parser.parse_args()
    root=Path(__file__).resolve().parents[1];owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=args.frozen_source.resolve(strict=True)
    assert source.name=='source'and source.is_relative_to(root/'.local/test-scratch')
    assert not args.output.resolve().is_relative_to(owner)
    module='src/campaign/scripting/source_workshop_script_builtins_tu15_generated.cpp'
    paths=[module,'src/campaign/scripting/source_workshop_camera_start_tu15_generated.cpp',
        'tests/workshop_camera_start_tests.cpp','tests/workshop_scenery_start_tests.cpp',
        'tests/workshop_unit_controls_tests.py','tests/fixtures/workshop_camera_start_tu15.json.zlib',
        'tests/fixtures/workshop_scenery_start_tu15.json.zlib',
        'tests/fixtures/workshop_camera_start_tu15.json','tests/fixtures/workshop_scenery_start_tu15.json',
        'tools/generate_workshop_script_builtins_tu15.py','tools/validate_workshop_camera_shared_tu15.py']
    pins={name:digest(root/name)for name in paths}
    assert digest(source/module)==pins[module],'Shared module differs from the declared frozen cohort'
    project=owner/'project';project.mkdir();shutil.copytree(source/'include',project/'include')
    shutil.copytree(source/'src/physics/runtime',project/'src/physics/runtime')
    for name in paths:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(root/name,target)
    (project/'tools/run_test.py').symlink_to(source/'tools/run_test.py')
    (project/'cmake').mkdir();shutil.copyfile(source/'cmake/TestStorage.cmake',project/'cmake/TestStorage.cmake')
    audit=json.loads((root/'build/test-results/workshop-goal-20261101/mcc-script-registry-inventory-2.json').read_text())
    row=next(v for v in audit['rows']if v['mcc_opcode']==454)
    assert audit['declaration_count']==918 and row['name']=='camera_set_animation'and row['arguments']==[36,11]
    generation_audit=owner/'audit.json';generation_audit.write_text(json.dumps(dict(
        xbox_tu15_sha256=audit['xbe_sha256'],declaration_candidates=[dict(row,matches=row['xbox_signature_candidates'])])))
    generated=project/'shared.cpp';generation=owner/'generation';python=root/'.tools/python/bin/python3'
    command=[str(python),'-B',str(root/'tools/generate_workshop_script_builtins_tu15.py'),
        '--audit-report',str(generation_audit),'--output',str(generated),'--directory',str(generation),
        '--preserve-module',str(project/module),'--names','camera_set_animation']
    subprocess.run(command,check=True)
    receipt=json.loads((generation/'generation.json').read_text())
    assert receipt['all_existing_bodies_unchanged']and len(receipt['preserved_body_sha256'])in(14,15)
    assert len(receipt['complete_native_entries'])==15
    # Body extraction is textual only; the generator itself reauthenticates
    # all instruction bytes and descriptor fields against the original image.
    def body(text):
        start=text.index('static void run_2b27b0(WorldState &s) {\n')
        end='throw std::runtime_error("Original solver fell outside authenticated CFG");\n}\n'
        return text[start:text.index(end,start)+len(end)]
    assert body(generated.read_text())==body((project/paths[1]).read_text())
    (project/'missing.cpp').write_text((project/module).read_text())
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run(sys.argv[1:],capture_output=True,text=True)
assert r.returncode!=0 and 'Missing complete native camera wrapper'in r.stderr,(r.returncode,r.stdout,r.stderr)
print('Expected missing-dispatch rejection:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(WorkshopSharedCamera LANGUAGES CXX)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
include(cmake/TestStorage.cmake)
if(HALO2_SANITIZERS)
add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
add_link_options(-fsanitize=address,undefined)
endif()
'''
    for target,cpp,test in [('camera','shared.cpp','workshop_camera_start_tests'),
            ('scenery','shared.cpp','workshop_scenery_start_tests'),('missing','missing.cpp','workshop_camera_start_tests')]:
        cmake+=f'''add_executable({target} {cpp} tests/{test}.cpp)
target_include_directories({target} PRIVATE include src)
target_compile_features({target} PRIVATE cxx_std_20)
target_compile_definitions({target} PRIVATE HALO2_CAMERA_SHARED_MODULE=1)
target_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME shared_{target} COMMAND '''+('"${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/negative.py" 'if target=='missing'else'')+f'''"${{Python3_EXECUTABLE}}" -B "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/workshop_unit_controls_tests.py"
 --fixture "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/workshop_{'scenery'if target=='scenery'else'camera'}_start_tu15.json.zlib"
 --test-executable $<TARGET_FILE:{target}>)
'''
    (project/'CMakeLists.txt').write_text(cmake);results=[]
    for name,compiler,sanitizers in [('gcc','g++',False),('clang','clang++',False),('clang-sanitized','clang++',True)]:
        output=args.output/name;run=[sys.executable,str(root/'tools/run_tests.py'),'--source',str(project),
            '--scratch-root',str(owner/'builds'),'--output',str(output),'--target','camera','--target','scenery',
            '--target','missing','--filter','^shared_','--jobs','2','--retain-report','Testing/Temporary/LastTest.log',
            '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
        if sanitizers:run.append('--sanitizers')
        result=subprocess.run(run);results.append(dict(name=name,exit_code=result.returncode,command=run,report=str(output/'report.json')))
    assert all(digest(root/name)==pin for name,pin in pins.items())
    report=dict(scope=__doc__,passed=all(v['exit_code']==0 for v in results),generation=receipt,
        generation_command=command,source_hashes=pins,results=results,camera_body_identical_to_standalone=True,
        native_camera_effects_qualified=False,MCC_admitted=False,published_component=False,
        owned_scratch=str(owner),retained_large_artifacts=[])
    if args.publish_component and report['passed']:
        shutil.copyfile(generated,root/module);report.update(published_component=True,published_module_sha256=digest(root/module))
    args.output.mkdir(parents=True,exist_ok=True);(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
