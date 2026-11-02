"""Verify actual installed E3 camera resources using immutable native Cache libs.

Prepare each declared E3 map with its real package/shared ownership, then load
every authored camera graph/clip and sample its decoded resource. Compiler,
preparations and decoded/captured data live under run_test.py ownership. This
does not admit camera scripts or qualify native HS, camera frames or rendering.
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
    parser.add_argument('--preparation',type=Path,required=True)
    parser.add_argument('--base-cache',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    root=Path(__file__).resolve().parents[1];owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.output.resolve().is_relative_to(owner)
    prep=json.loads(args.preparation.read_text());source=Path(prep['source']).resolve(strict=True);build=source.parent/'build'
    assert source.name=='source'and source.is_relative_to(root/'.local/test-scratch')
    sys.path.insert(0,str(source/'tools'));import steam_workshop as workshop
    from build_launcher_engine import source_hashes
    inventory=source_hashes(source)
    assert hashlib.sha256(json.dumps(inventory,sort_keys=True).encode()).hexdigest()==prep['production_inventory_sha256']
    catalog=workshop.scan();mod=next(m for m in catalog['mods']if m['id']=='3360515088')
    assert not mod['error']and not catalog['errors']
    item=Path(mod['directory']);cases=[]
    for name,count in [('earthcity_e3',55),('earthcity_e3_potato',55),('earthcity_playground',0)]:
        path=workshop.source_owned(item,'maps/'+name+'.map');pin=workshop.pin(path)
        prepared=workshop.prepare_cached(item,name,args.base_cache,owner/(name+'-prepared'))
        assert workshop.pin(path)==pin
        cases.append(dict(name=name,calls=count,source=str(path),source_pin=pin,prepared=prepared))
    link=build/'CMakeFiles/workshop_mod_probe.dir/link.txt';link_pin=digest(link)
    words=shlex.split(link.read_text());words=words[words.index('-o')+2:];libraries={};links=[]
    for word in words:
        if word.endswith('.a')or('.so'in word and not word.startswith('-')):
            path=Path(word);path=path if path.is_absolute()else build/path;path=path.resolve(strict=True)
            libraries[str(path)]=digest(path);links.append(str(path))
        else:assert word.startswith('-');links.append(word)
    assert str(build/'libhalo2_cache.a')in libraries
    project=owner/'project';project.mkdir()
    test=root/'tests/workshop_camera_content_tests.cpp';test_pin=digest(test);shutil.copyfile(test,project/'camera_content.cpp')
    (project/'tools').symlink_to(source/'tools',target_is_directory=True)
    cmake='''cmake_minimum_required(VERSION 3.25)
project(WorkshopCameraContent LANGUAGES CXX)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
include('''+json.dumps(str(source/'cmake/TestStorage.cmake'))+''')
add_executable(camera_content camera_content.cpp)
target_compile_features(camera_content PRIVATE cxx_std_20)
target_compile_options(camera_content PRIVATE -fno-fast-math -ffp-contract=off)
target_include_directories(camera_content PRIVATE '''+json.dumps(str(source/'include'))+''')
target_link_libraries(camera_content PRIVATE nlohmann_json::nlohmann_json
'''+''.join(' '+json.dumps(word)+'\n'for word in links)+')\n'
    for case in cases:
        p=case['prepared'];argv=[p['map'],p['shared_maps'],p['prepared_root'],str(case['calls'])]
        cmake+='add_test(NAME workshop_camera_content_'+case['name']+' COMMAND camera_content '+ ' '.join(json.dumps(str(v))for v in argv)+' "${CMAKE_CURRENT_BINARY_DIR}/'+case['name']+'.json")\n'
        cmake+='set_tests_properties(workshop_camera_content_'+case['name']+' PROPERTIES TIMEOUT 180)\n'
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(root/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(owner/'builds'),
        '--output',str(args.output),'--target','camera_content','--filter','^workshop_camera_content_',
        '--jobs','2','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log']
    for case in cases:command+=['--retain-report',case['name']+'.json']
    result=subprocess.run(command)
    assert digest(test)==test_pin and digest(link)==link_pin and source_hashes(source)==inventory
    assert all(digest(Path(path))==pin for path,pin in libraries.items())
    assert all(workshop.pin(Path(case['source']))==case['source_pin']for case in cases)
    for case in cases:case.pop('prepared')
    report=dict(scope=__doc__,passed=result.returncode==0,exit_code=result.returncode,
        preparation=str(args.preparation),preparation_sha256=digest(args.preparation),
        source_inventory_sha256=prep['production_inventory_sha256'],test_sha256=test_pin,
        helper_sha256=digest(Path(__file__)),libraries=libraries,link_template_sha256=link_pin,cases=cases,
        camera_admission_changed=False,owned_scratch=str(owner),retained_large_artifacts=[])
    args.output.mkdir(parents=True,exist_ok=True);(args.output/'content.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
