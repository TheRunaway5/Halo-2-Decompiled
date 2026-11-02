"""Qualify additive native scenery-start wrapper generation and ABI behavior.

Compiler trees, expanded original references and binaries use owned storage.
Only bounded reports and, with --publish-component, the validated native source
and small original regression fixture leave scratch. No script admission or
animation effect qualification is performed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--sanitizers',action='store_true');p.add_argument('--publish-component',action='store_true');a=p.parse_args()
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not a.output.resolve().is_relative_to(scratch)
    assert not(a.publish_component and a.sanitizers)
    project=scratch/'project';project.mkdir();shutil.copytree(root/'include',project/'include')
    module='src/campaign/scripting/source_workshop_script_builtins_tu15_generated.cpp'
    paths=[module,'tests/workshop_scenery_start_tests.cpp','tests/workshop_unit_controls_tests.py',
        'tools/generate_workshop_script_builtins_tu15.py','tools/oracle_workshop_scenery_start_tu15.py',
        'tools/validate_workshop_scenery_start_tu15.py','tools/run_test.py','cmake/TestStorage.cmake']
    paths +=[str(f.relative_to(root))for f in(root/'src/physics/runtime').glob('*.hpp')]
    pins={name:digest(root/name)for name in paths}
    for name in paths:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(root/name,target)
    audit=json.loads((root/'build/test-results/workshop-goal-20261101/mcc-script-registry-inventory.json').read_text())
    assert audit['declaration_count']==918
    row=next(r for r in audit['rows']if r['mcc_opcode']==185)
    assert row['name']=='scenery_animation_start'and row['return_type']==4 and row['arguments']==[55,36,11]
    generation_audit=scratch/'audit.json'
    generation_audit.write_text(json.dumps(dict(xbox_tu15_sha256=audit['xbe_sha256'],
        declaration_candidates=[dict(row,matches=row['xbox_signature_candidates'])])))
    generated=project/'scenery.cpp';generation=scratch/'generation'
    python=root/'.tools/python/bin/python3'
    generate=[str(python),'-B',str(root/'tools/generate_workshop_script_builtins_tu15.py'),
        '--audit-report',str(generation_audit),'--output',str(generated),'--directory',str(generation),
        '--preserve-module',str(project/module),'--names','scenery_animation_start']
    subprocess.run(generate,check=True)
    receipt=json.loads((generation/'generation.json').read_text())
    assert receipt['all_existing_bodies_unchanged']and len(receipt['preserved_body_sha256'])in(13,14,15)
    assert len(receipt['complete_native_entries'])in(14,15)
    fixture=project/'tests/fixtures/workshop_scenery_start_tu15.json.zlib';fixture.parent.mkdir(parents=True)
    oracle=[str(python),'-B',str(root/'tools/oracle_workshop_scenery_start_tu15.py'),'--output',str(fixture)]
    subprocess.run(oracle,check=True)
    source=generated.read_text();start=source.index('static void run_2accf0(WorldState &s) {\n')
    terminator='throw std::runtime_error("Original solver fell outside authenticated CFG");\n}\n'
    body=source[start:source.index(terminator,start)+len(terminator)]
    changes=[
        ('wrong_graph','s.set(0,r(s.memory,(0x4u+s.gp[0]),4),4,0);','s.set(0,r(s.memory,(0x8u+s.gp[0]),4),4,0);'),
        ('wrong_start_frame','s.push(0x2acd27u,0x0u);','s.push(0x2acd27u,0x1u);'),
        ('wrong_hold','s.push(0x2acd23u,0x1u);','s.push(0x2acd23u,0x0u);'),
        ('wrong_loop','s.push(0x2acd21u,0x0u);','s.push(0x2acd21u,0x1u);'),
        ('wrong_anchor','s.push(0x2acd25u,0xffffffffu);','s.push(0x2acd25u,0x0u);'),
        ('missing_void_completion','s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);',';')]
    for name,needle,replacement in changes:
        assert body.count(needle)==1,(name,body)
        (project/(name+'.cpp')).write_text(source.replace(body,body.replace(needle,replacement)))
    (project/'missing_owner.cpp').write_text((project/module).read_text())
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run(sys.argv[1:],capture_output=True,text=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('Scenery wrapper','Missing complete native scenery wrapper')),(r.returncode,r.stdout,r.stderr)
print('Expected original-backed rejection:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(WorkshopSceneryStart LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(cmake/TestStorage.cmake)
if(HALO2_SANITIZERS)
add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
add_link_options(-fsanitize=address,undefined)
endif()
'''
    variants=[]if a.sanitizers else['missing_owner']+[r[0]for r in changes]
    if '002accf0'in receipt['preserved_body_sha256']and'missing_owner'in variants:variants.remove('missing_owner')
    for name in ['scenery']+variants:
        cmake+=f'''add_executable({name} {name}.cpp tests/workshop_scenery_start_tests.cpp)
target_include_directories({name} PRIVATE include src)
target_compile_features({name} PRIVATE cxx_std_20)
target_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries({name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME scenery_start_{name} COMMAND '''+(''if name=='scenery'else'"${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/negative.py" ')+f'''"${{Python3_EXECUTABLE}}" -B "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/workshop_unit_controls_tests.py"
 --fixture "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/workshop_scenery_start_tu15.json.zlib" --test-executable "$<TARGET_FILE:{name}>")
'''
    (project/'CMakeLists.txt').write_text(cmake);results=[]
    for compiler in (('clang++',)if a.sanitizers else('g++','clang++')):
        command=[sys.executable,str(root/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
            '--output',str(a.output/compiler),'--filter','^scenery_start_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
            '--jobs','2','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log']
        for name in ['scenery']+variants:command+=['--target',name]
        if a.sanitizers:command+=['--sanitizers']
        run=subprocess.run(command);results.append(dict(compiler=compiler,command=command,exit_code=run.returncode))
    unchanged=all(digest(root/name)==value for name,value in pins.items())
    report=dict(scope=__doc__,passed=all(r['exit_code']==0 for r in results)and unchanged,source_hashes=pins,
        sources_unchanged=unchanged,sanitizers=a.sanitizers,negative_controls=variants,results=results,
        generation_command=generate,generation=receipt,oracle_command=oracle,fixture=json.loads(fixture.with_suffix('').read_text()),
        runtime_enabled=False,engine_effects_qualified=False,owned_scratch=str(scratch),retained_large_artifacts=[],published_component=False)
    if a.publish_component and report['passed']:
        for source_file,target in[(generated,root/module),(fixture,root/'tests/fixtures'/fixture.name),
            (fixture.with_suffix(''),root/'tests/fixtures'/fixture.with_suffix('').name)]:
            shutil.copyfile(source_file,target)
        report['published_component']=True;report['published_module_sha256']=digest(root/module)
    a.output.mkdir(parents=True,exist_ok=True);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
