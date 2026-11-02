"""Qualify the complete original camera HS wrapper with explicit ABI inputs.

Use frozen engine headers and owned compiler/reference storage. Optional
publication retains only qualified component source and its small reference;
it never routes the World owner or changes MCC script admission.
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
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--frozen-source',type=Path,required=True)
    p.add_argument('--sanitizers',action='store_true');p.add_argument('--publish-component',action='store_true');a=p.parse_args()
    root=Path(__file__).resolve().parents[1];owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=a.frozen_source.resolve(strict=True)
    assert source.name=='source'and source.is_relative_to(root/'.local/test-scratch')
    assert not a.output.resolve().is_relative_to(owner)and not(a.sanitizers and a.publish_component)
    project=owner/'project';project.mkdir();shutil.copytree(source/'include',project/'include')
    files=['include/halo2/source_workshop_camera_start_tu15.hpp','tests/workshop_camera_start_tests.cpp',
        'tests/workshop_unit_controls_tests.py','tools/generate_workshop_camera_start_tu15.py',
        'tools/oracle_workshop_camera_start_tu15.py','tools/validate_workshop_camera_start_tu15.py',
        'tools/generate_physics_live_support_tu15.py','tools/oracle_smg_firing_1_1.py',
        'tools/research_mp_sandbox_update_functions.py','tools/reference/halo2-decompiled/tools/functions.py',
        'tools/run_test.py','cmake/TestStorage.cmake']
    pins={name:digest(root/name)for name in files}
    for name in files:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(root/name,target)
    headers={}
    for folder in ('include','src/physics/runtime'):
        for path in (source/folder).rglob('*.hpp'):
            name=path.relative_to(source).as_posix();headers[name]=digest(path)
            if folder!='include':
                target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target)
    audit=json.loads((root/'build/test-results/workshop-goal-20261101/mcc-script-registry-inventory-2.json').read_text())
    row=next(v for v in audit['rows']if v['mcc_opcode']==454)
    assert audit['declaration_count']==918 and row['name']=='camera_set_animation'
    assert row['return_type']==4 and row['arguments']==[36,11]
    assert row['xbox_signature_candidates']==[dict(xbox_opcode=452,xbox_descriptor='0x458e90',
        xbox_return_type=4,xbox_arguments=[36,11],xbox_flags=0,xbox_evaluator='0x2b27b0')]
    python=root/'.tools/python/bin/python3';generated=project/'camera.cpp';directory=owner/'generation'
    generate=[str(python),'-B',str(root/'tools/generate_workshop_camera_start_tu15.py'),
        '--output',str(generated),'--directory',str(directory)]
    subprocess.run(generate,check=True)
    fixture=project/'tests/fixtures/workshop_camera_start_tu15.json.zlib';fixture.parent.mkdir(parents=True)
    oracle=[str(python),'-B',str(root/'tools/oracle_workshop_camera_start_tu15.py'),'--output',str(fixture)]
    subprocess.run(oracle,check=True)
    text=generated.read_text();start=text.index('static void run_2b27b0(WorldState &s) {\n')
    terminator='throw std::runtime_error("Original solver fell outside authenticated CFG");\n}\n'
    body=text[start:text.index(terminator,start)+len(terminator)]
    mutations=[
        ('unsigned_index','sign_extend(r(s.memory,(0x4u+s.gp[4]),2),2)','r(s.memory,(0x4u+s.gp[4]),2)'),
        ('wrong_graph','s.set(0,r(s.memory,(0x0u+s.gp[0]),4),4,0);','s.set(0,r(s.memory,(0x4u+s.gp[0]),4),4,0);'),
        ('wrong_name','s.set(3,r(s.memory,(0x4u+s.gp[0]),4),4,0);','s.set(3,r(s.memory,(0x0u+s.gp[0]),4),4,0);'),
        ('wrong_flag','s.push(0x2b27dfu,0xffffffffu);','s.push(0x2b27dfu,0x0u);'),
        ('wrong_object','s.push(0x2b27e1u,0xffffffffu);','s.push(0x2b27e1u,0x0u);'),
        ('missing_void','s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);',';'),
        ('wrong_ret','s.gp[4]+=12u;return;','s.gp[4]+=8u;return;')]
    variants=[]
    if not a.sanitizers:
        for name,needle,replacement in mutations:
            assert body.count(needle)==1,(name,body)
            (project/(name+'.cpp')).write_text(text.replace(body,body.replace(needle,replacement)));variants.append(name)
        needle='    abi=s.save();\n}\n';assert text.count(needle)==1
        (project/'erase_fp.cpp').write_text(text.replace(needle,'    abi=s.save();abi.x87_depth=0;\n}\n'));variants.append('erase_fp')
        needle='case 0x2b27b0u:return true;';assert text.count(needle)==1
        (project/'missing_owner.cpp').write_text(text.replace(needle,'case 0x2b27b0u:return false;'));variants.append('missing_owner')
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run(sys.argv[1:],capture_output=True,text=True)
assert r.returncode!=0 and any(s in r.stderr for s in ('Camera wrapper','camera wrapper','map::at')),(r.returncode,r.stdout,r.stderr)
print('Expected original-backed rejection:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(WorkshopCameraStart LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(cmake/TestStorage.cmake)
if(HALO2_SANITIZERS)
add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
add_link_options(-fsanitize=address,undefined)
endif()
'''
    for name in ['camera']+variants:
        cmake+=f'''add_executable({name} {name}.cpp tests/workshop_camera_start_tests.cpp)
target_include_directories({name} PRIVATE include src)
target_compile_features({name} PRIVATE cxx_std_20)
target_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries({name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME camera_start_{name} COMMAND '''+(''if name=='camera'else'"${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/negative.py" ')+f'''"${{Python3_EXECUTABLE}}" -B "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/workshop_unit_controls_tests.py"
 --fixture "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/workshop_camera_start_tu15.json.zlib" --test-executable "$<TARGET_FILE:{name}>")
'''
    (project/'CMakeLists.txt').write_text(cmake);results=[]
    for compiler in (('clang++',)if a.sanitizers else('g++','clang++')):
        command=[sys.executable,str(root/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(owner/'builds'),
            '--output',str(a.output/compiler),'--filter','^camera_start_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
            '--jobs','2','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log']
        for name in ['camera']+variants:command+=['--target',name]
        if a.sanitizers:command+=['--sanitizers']
        run=subprocess.run(command);results.append(dict(compiler=compiler,command=command,exit_code=run.returncode))
    unchanged=all(digest(root/name)==value for name,value in pins.items())and all(digest(source/name)==value for name,value in headers.items())
    report=dict(scope=__doc__,passed=unchanged and all(r['exit_code']==0 for r in results),sources_unchanged=unchanged,
        source_hashes=pins,frozen_source=str(source),frozen_header_count=len(headers),
        frozen_header_inventory_sha256=hashlib.sha256(json.dumps(headers,sort_keys=True).encode()).hexdigest(),
        sanitizers=a.sanitizers,negative_controls=variants,results=results,generation_command=generate,
        generation=json.loads((directory/'generation.json').read_text()),oracle_command=oracle,
        fixture=json.loads(fixture.with_suffix('').read_text()),mcc_declaration=row,
        runtime_enabled=False,engine_consumers_qualified=False,published_component=False,owned_scratch=str(owner),retained_large_artifacts=[])
    if a.publish_component and report['passed']:
        destination=root/'src/campaign/scripting/source_workshop_camera_start_tu15_generated.cpp'
        assert not destination.exists();shutil.copyfile(generated,destination)
        for path in (fixture,fixture.with_suffix('')):shutil.copyfile(path,root/'tests/fixtures'/path.name)
        report.update(published_component=True,published_module_sha256=digest(destination))
    a.output.mkdir(parents=True,exist_ok=True);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
