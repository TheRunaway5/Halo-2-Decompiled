"""Verify actual Workshop empty-prop prepasses with owned preparation storage.

Normal CTest mode uses its built executable. Frozen component mode borrows only
authenticated immutable archive inputs, then compiles the real factory source
through run_tests.py. All map copies, compiler objects and executables remain
owned; only bounded diagnostics survive. This is not gameplay qualification.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys


def digest(path):
    with path.open('rb')as file:return hashlib.file_digest(file,'sha256').hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--frozen-source',type=Path);p.add_argument('--frozen-build',type=Path)
    p.add_argument('--test-executable',type=Path);p.add_argument('--base-cache',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    root=Path(__file__).resolve().parents[1];owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not a.output.resolve().is_relative_to(owner)
    component=a.test_executable is None
    assert (a.frozen_source is not None and a.frozen_build is not None)if component else a.frozen_source is None and a.frozen_build is None
    tools=(a.frozen_source if component else root)/'tools';sys.path.insert(0,str(tools))
    import steam_workshop as workshop
    catalog=workshop.scan();mod=next((v for v in catalog['mods']if v['id']=='3000901245'),None)
    if mod is None or not a.base_cache.is_dir():return 77
    assert not catalog['errors']and not mod['error']
    cases=[]
    for name,count in [('convergence',41),('solitude',1)]:
        source=workshop.source_owned(Path(mod['directory']),'maps/'+name+'.map');pin=workshop.pin(source)
        prepared=workshop.prepare_cached(Path(mod['directory']),name,a.base_cache,owner/(name+'-prepared'))
        assert workshop.pin(source)==pin
        cases.append(dict(name=name,props=count,empty=1,source=str(source),source_pin=pin,prepared=prepared))
    pins={};libraries=[];result=None;project=owner/'project'
    if component:
        source=a.frozen_source.resolve(strict=True);build=a.frozen_build.resolve(strict=True)
        assert source.name=='source'and source.parent==build.parent and source.is_relative_to(root/'.local/test-scratch')
        project.mkdir();shutil.copytree(source/'include',project/'include')
        shutil.copytree(source/'src/physics/runtime',project/'src/physics/runtime')
        for name in ['src/gameplay/objects/prop_sandbox_definition.cpp','tests/workshop_empty_prop_content_tests.cpp']:
            original=root/name;target=project/name;target.parent.mkdir(parents=True,exist_ok=True)
            pins[name]=digest(original);shutil.copyfile(original,target);assert digest(target)==pins[name]
        (project/'tools').symlink_to(source/'tools',target_is_directory=True)
        (project/'cmake').mkdir();shutil.copyfile(source/'cmake/TestStorage.cmake',project/'cmake/TestStorage.cmake')
        (project/'libraries').mkdir()
        for word in shlex.split((build/'CMakeFiles/workshop_mod_probe.dir/link.txt').read_text()):
            if word.endswith('.a'):
                original=build/word;pin=digest(original);target=project/'libraries'/Path(word).name
                shutil.copyfile(original,target);assert digest(original)==digest(target)==pin
                libraries.append(dict(path=str(original),sha256=pin,link=str(target)))
            elif '.so' in word and Path(word).is_file():libraries.append(dict(path=word,link=word,sha256=digest(Path(word))))
        assert libraries and any(Path(v['path']).name=='libhalo2_cache.a'for v in libraries)
        cmake='''cmake_minimum_required(VERSION 3.20)
project(WorkshopEmptyProp LANGUAGES CXX)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
include(cmake/TestStorage.cmake)
add_executable(empty_prop tests/workshop_empty_prop_content_tests.cpp src/gameplay/objects/prop_sandbox_definition.cpp)
target_compile_features(empty_prop PRIVATE cxx_std_20)
target_include_directories(empty_prop PRIVATE include src)
target_compile_options(empty_prop PRIVATE -fno-fast-math -ffp-contract=off)
target_link_libraries(empty_prop PRIVATE nlohmann_json::nlohmann_json
'''+''.join('  '+json.dumps(v['link'])+'\n'for v in libraries)+')\n'
        for case in cases:
            prepared=case['prepared'];name=case['name']
            args=[prepared['map'],prepared['shared_maps'],prepared['prepared_root'],str(case['props']),'1']
            cmake+='add_test(NAME empty_prop_'+name+' COMMAND empty_prop '+ ' '.join(json.dumps(str(v))for v in args)+' "${CMAKE_CURRENT_BINARY_DIR}/'+name+'.json")\n'
        (project/'CMakeLists.txt').write_text(cmake)
        command=[sys.executable,str(root/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(owner/'builds'),
            '--output',str(a.output.with_suffix('')),'--target','empty_prop','--filter','^empty_prop_',
            '--jobs','2','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log']
        for case in cases:command+=['--retain-report',case['name']+'.json']
        result=subprocess.run(command);passed=result.returncode==0
    else:
        passed=True
        for case in cases:
            prepared=case['prepared'];report=owner/(case['name']+'.json')
            command=[str(a.test_executable),prepared['map'],prepared['shared_maps'],prepared['prepared_root'],str(case['props']),'1',str(report)]
            run=subprocess.run(command,capture_output=True,text=True,timeout=180);case['run']=dict(exit_code=run.returncode,stdout=run.stdout[-2000:],stderr=run.stderr[-2000:])
            if report.is_file():case['content']=json.loads(report.read_text())
            passed&=run.returncode==0
    assert all(workshop.pin(Path(c['source']))==c['source_pin']for c in cases)
    assert all(digest(root/name)==pin for name,pin in pins.items())
    for case in cases:case.pop('prepared')
    report=dict(scope=__doc__,passed=passed,component=component,source_hashes=pins,libraries=libraries,cases=cases,
        build_exit_code=None if result is None else result.returncode,owned_scratch=str(owner),retained_large_artifacts=[])
    a.output.parent.mkdir(parents=True,exist_ok=True);data=json.dumps(report,indent=2)+'\n';assert len(data.encode())<=1024*1024
    a.output.write_text(data);print(json.dumps(dict(passed=passed,cases=len(cases),report=str(a.output))))
    return 0 if passed else 1


if __name__=='__main__':raise SystemExit(main())
