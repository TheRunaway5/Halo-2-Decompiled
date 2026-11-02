"""Qualify complete native BD790 with original-backed ABI and fault controls.

Use the active frozen engine's headers, not an unrelated dirty checkout ABI.
All expanded references, compiler trees and binaries remain owned. Optional
component publication retains source and a small required regression fixture;
it does not route World dispatch or admit missing script features.
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
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--frozen-source',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--publish-component',action='store_true');args=parser.parse_args()
    root=Path(__file__).resolve().parents[1];owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=args.frozen_source.resolve(strict=True)
    assert not source.is_symlink() and source.name=='source' and source.is_relative_to(root/'.local/test-scratch')
    assert not args.output.resolve().is_relative_to(owner) and not(args.sanitizers and args.publish_component)
    project=owner/'project';project.mkdir();shutil.copytree(source/'include',project/'include')
    files=['include/halo2/source_object_name_lookup_tu15.hpp','tests/object_name_lookup_tu15_tests.cpp',
        'tests/workshop_unit_controls_tests.py','tools/generate_source_object_name_lookup_tu15.py',
        'tools/oracle_object_name_lookup_tu15.py','tools/validate_object_name_lookup_tu15.py',
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
    python=root/'.tools/python/bin/python3';generated=project/'lookup.cpp';directory=owner/'generation'
    generate=[str(python),'-B',str(root/'tools/generate_source_object_name_lookup_tu15.py'),
        '--output',str(generated),'--directory',str(directory)]
    subprocess.run(generate,check=True)
    fixture=project/'tests/fixtures/object_name_lookup_tu15.json.zlib';fixture.parent.mkdir(parents=True)
    oracle=[str(python),'-B',str(root/'tools/oracle_object_name_lookup_tu15.py'),'--output',str(fixture)]
    subprocess.run(oracle,check=True)
    text=generated.read_text();start=text.index('static void run_bd790(WorldState &s) {\n')
    terminator='throw std::runtime_error("Original solver fell outside authenticated CFG");\n}\n'
    body=text[start:text.index(terminator,start)+len(terminator)]
    mutations=[
        ('wrong_signed_width','s.flags.logic((s.reg(0,2,0))&(s.reg(0,2,0)),2);','s.flags.logic((s.reg(0,4,0))&(s.reg(0,4,0)),4);'),
        ('wrong_bound','s.flags.minus(s.reg(0,2,0),0x280u,2);','s.flags.minus(s.reg(0,2,0),0x281u,2);'),
        ('wrong_stride','s.gp[0]*4u','s.gp[0]*8u'),
        ('wrong_return_width','s.set(0,r(s.memory,(0x0u+s.gp[1]+s.gp[0]*4u),4),4,0);','s.set(0,r(s.memory,(0x0u+s.gp[1]+s.gp[0]*4u),4),2,0);'),
        ('eager_bank_read','static void run_bd790(WorldState &s) {\n','static void run_bd790(WorldState &s) {\n(void)r(s.memory,0x4eaba0u,4);')]
    variants=[]
    if not args.sanitizers:
        for name,needle,replacement in mutations:
            assert body.count(needle)==1,(name,body)
            (project/(name+'.cpp')).write_text(text.replace(body,body.replace(needle,replacement)));variants.append(name)
        assert body.count('s.gp[4]+=0u;return;')==2
        (project/'wrong_ret.cpp').write_text(text.replace(body,body.replace('s.gp[4]+=0u;return;','s.gp[4]+=4u;return;')));variants.append('wrong_ret')
        needle='    abi=s.save();\n}\n';assert text.count(needle)==1
        (project/'erase_fp.cpp').write_text(text.replace(needle,'    abi=s.save();abi.x87_depth=0;\n}\n'));variants.append('erase_fp')
        needle='case 0xbd790u:return true;';assert text.count(needle)==1
        (project/'missing_owner.cpp').write_text(text.replace(needle,'case 0xbd790u:return false;'));variants.append('missing_owner')
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run(sys.argv[1:],capture_output=True,text=True)
assert r.returncode!=0 and any(s in r.stderr for s in ('Object-name','object-name','map::at')),(r.returncode,r.stdout,r.stderr)
print('Expected original-backed rejection:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(ObjectNameLookup LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(cmake/TestStorage.cmake)
if(HALO2_SANITIZERS)
add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
add_link_options(-fsanitize=address,undefined)
endif()
'''
    for name in ['lookup']+variants:
        cmake+=f'''add_executable({name} {name}.cpp tests/object_name_lookup_tu15_tests.cpp)
target_include_directories({name} PRIVATE include src)
target_compile_features({name} PRIVATE cxx_std_20)
target_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries({name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME object_name_{name} COMMAND '''+(''if name=='lookup'else'"${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/negative.py" ')+f'''"${{Python3_EXECUTABLE}}" -B "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/workshop_unit_controls_tests.py"
 --fixture "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/object_name_lookup_tu15.json.zlib" --test-executable "$<TARGET_FILE:{name}>")
'''
    (project/'CMakeLists.txt').write_text(cmake);results=[]
    for compiler in (('clang++',)if args.sanitizers else('g++','clang++')):
        command=[sys.executable,str(root/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(owner/'builds'),
            '--output',str(args.output/compiler),'--filter','^object_name_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
            '--jobs','2','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log']
        for name in ['lookup']+variants:command+=['--target',name]
        if args.sanitizers:command+=['--sanitizers']
        run=subprocess.run(command);results.append(dict(compiler=compiler,command=command,exit_code=run.returncode))
    unchanged=all(digest(root/name)==value for name,value in pins.items())and all(digest(source/name)==value for name,value in headers.items())
    report=dict(scope=__doc__,passed=unchanged and all(r['exit_code']==0 for r in results),sources_unchanged=unchanged,
        source_hashes=pins,frozen_source=str(source),frozen_header_count=len(headers),
        frozen_header_inventory_sha256=hashlib.sha256(json.dumps(headers,sort_keys=True).encode()).hexdigest(),
        sanitizers=args.sanitizers,negative_controls=variants,results=results,generation_command=generate,
        generation=json.loads((directory/'generation.json').read_text()),oracle_command=oracle,
        fixture=json.loads(fixture.with_suffix('').read_text()),owned_scratch=str(owner),
        runtime_enabled=False,engine_consumers_qualified=False,published_component=False,retained_large_artifacts=[])
    if args.publish_component and report['passed']:
        destination=root/'src/gameplay/objects/source_object_name_lookup_tu15_generated.cpp'
        assert not destination.exists()
        shutil.copyfile(generated,destination)
        for path in (fixture,fixture.with_suffix('')):shutil.copyfile(path,root/'tests/fixtures'/path.name)
        report.update(published_component=True,published_module_sha256=digest(destination))
    args.output.mkdir(parents=True,exist_ok=True)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
