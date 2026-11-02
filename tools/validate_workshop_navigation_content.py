"""Exercise actual installed high-index Workshop navigation through Cache.

The fixture pins the unchanged authored maps and sampled vertex bits. Normal
CTest mode runs its real linked executable. Frozen component mode replaces only
content.cpp against copied immutable Cache archives. Preparation, compiler and
map outputs are owned and removed; this is not AI or gameplay qualification.
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
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--frozen-source',type=Path);parser.add_argument('--frozen-build',type=Path)
    parser.add_argument('--test-executable',type=Path);parser.add_argument('--base-cache',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    root=Path(__file__).resolve().parents[1];owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.output.resolve().is_relative_to(owner)
    component=args.test_executable is None
    assert (args.frozen_source is not None and args.frozen_build is not None)if component else args.frozen_source is None and args.frozen_build is None
    tools=(args.frozen_source if component else root)/'tools';sys.path.insert(0,str(tools))
    import steam_workshop as workshop
    catalog=workshop.scan();mod=next((v for v in catalog['mods']if v['id']=='3000901245'),None)
    if mod is None or not args.base_cache.is_dir():return 77
    assert not catalog['errors']and not mod['error']
    fixture=root/'tests/fixtures/workshop_navigation_content.json'
    cases=json.loads(fixture.read_text())['rows'];prepared={};pins={};libraries=[]
    for case in cases:
        name=case['map'];source=workshop.source_owned(Path(mod['directory']),'maps/'+name+'.map')
        assert workshop.pin(source)==case['source_pin'],'Authored navigation fixture source changed'
        if name not in prepared:
            prepared[name]=workshop.prepare_cached(Path(mod['directory']),name,args.base_cache,owner/(name+'-prepared'))
        case['source']=str(source);case['expectation']=str(owner/(name+'-'+str(case['bsp'])+'-expected.json'))
        Path(case['expectation']).write_text(json.dumps(case)+'\n')
    result=None
    if component:
        source=args.frozen_source.resolve(strict=True);build=args.frozen_build.resolve(strict=True)
        assert source.name=='source'and source.parent==build.parent and source.is_relative_to(root/'.local/test-scratch')
        project=owner/'project';project.mkdir();shutil.copytree(source/'include',project/'include')
        for name,original in [
            ('src/assets/content.cpp',root/'src/assets/content.cpp'),
            ('tests/workshop_navigation_content_tests.cpp',root/'tests/workshop_navigation_content_tests.cpp'),
            ('src/gameplay/vehicles/native_vehicle_variant_symbols_tu15.hpp',source/'src/gameplay/vehicles/native_vehicle_variant_symbols_tu15.hpp')]:
            target=project/name;target.parent.mkdir(parents=True,exist_ok=True)
            pins[str(original)]=digest(original);shutil.copyfile(original,target);assert digest(target)==pins[str(original)]
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
project(WorkshopNavigation LANGUAGES CXX)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
include(cmake/TestStorage.cmake)
add_executable(navigation_content tests/workshop_navigation_content_tests.cpp src/assets/content.cpp)
target_compile_features(navigation_content PRIVATE cxx_std_20)
target_include_directories(navigation_content PRIVATE include src)
target_compile_options(navigation_content PRIVATE -fno-fast-math -ffp-contract=off)
target_link_libraries(navigation_content PRIVATE nlohmann_json::nlohmann_json
'''+''.join('  '+json.dumps(v['link'])+'\n'for v in libraries)+')\n'
        names=[]
        for case in cases:
            name=case['map']+'_'+str(case['bsp']);names.append(name);cache=prepared[case['map']]
            argv=[cache['map'],cache['shared_maps'],cache['prepared_root'],case['expectation']]
            cmake+='add_test(NAME navigation_'+name+' COMMAND navigation_content '+ ' '.join(json.dumps(str(v))for v in argv)+' "${CMAKE_CURRENT_BINARY_DIR}/'+name+'.json")\n'
        (project/'CMakeLists.txt').write_text(cmake)
        command=[sys.executable,str(root/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(owner/'builds'),
            '--output',str(args.output.with_suffix('')),'--target','navigation_content','--filter','^navigation_',
            '--jobs','2','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log']
        for name in names:command+=['--retain-report',name+'.json']
        result=subprocess.run(command);passed=result.returncode==0
    else:
        passed=True
        for case in cases:
            cache=prepared[case['map']];report=owner/(case['map']+'-'+str(case['bsp'])+'.json')
            command=[str(args.test_executable),cache['map'],cache['shared_maps'],cache['prepared_root'],case['expectation'],str(report)]
            run=subprocess.run(command,capture_output=True,text=True,timeout=180)
            case['run']=dict(exit_code=run.returncode,stdout=run.stdout[-2000:],stderr=run.stderr[-2000:])
            if report.is_file():case['content']=json.loads(report.read_text())
            passed&=run.returncode==0
    assert all(workshop.pin(Path(case['source']))==case['source_pin']for case in cases)
    assert all(digest(Path(name))==pin for name,pin in pins.items())
    for case in cases:case.pop('expectation')
    report=dict(scope=__doc__,passed=passed,component=component,source_hashes=pins,libraries=libraries,
        fixture_sha256=digest(fixture),cases=cases,build_exit_code=None if result is None else result.returncode,
        owned_scratch=str(owner),retained_large_artifacts=[])
    args.output.parent.mkdir(parents=True,exist_ok=True);data=json.dumps(report,indent=2)+'\n';assert len(data.encode())<=1024*1024
    args.output.write_text(data);print(json.dumps(dict(passed=passed,cases=len(cases),report=str(args.output))))
    return 0 if passed else 1


if __name__=='__main__':raise SystemExit(main())
