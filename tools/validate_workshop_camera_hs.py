"""Run real native camera HS/frame consumers against a completed frozen build.

Borrow immutable qualified libraries and headers without changing the cohort.
The original452 caller is explicit test input, using an actual stock clip.
All test objects, executable and world storage use run_test.py ownership; only
bounded reports survive. This does not admit MCC454 or qualify E3/GPU gameplay.
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
    parser.add_argument('--qualification',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    root=Path(__file__).resolve().parents[1];owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    qualification=json.loads(args.qualification.read_text())
    assert qualification['passed']and qualification['status']=='prepared'and qualification['qualified_candidate']
    source=Path(qualification['source_cohort']['source']).resolve(strict=True)
    build=Path(qualification['source_cohort']['build']).resolve(strict=True)
    assert source.name=='source'and build.parent==source.parent and source.is_relative_to(root/'.local/test-scratch')
    assert not args.output.resolve().is_relative_to(owner)
    sys.path.insert(0,str(root/'tools'));from build_launcher_engine import source_hashes
    package=(root/qualification['runtime']).parent
    declared=json.loads((package/'source-hashes.json').read_text());assert source_hashes(source)==declared
    assert digest(package/'halo2_native')==qualification['runtime_sha256']
    link=build/'CMakeFiles/native_mcc_scenery_animation_world_tests.dir/link.txt';link_pin=digest(link)
    words=shlex.split(link.read_text());arguments=words[words.index('-o')+2:];libraries={};links=[]
    for word in arguments:
        if word.endswith('.a')or('.so'in word and not word.startswith('-')):
            path=Path(word);path=path if path.is_absolute()else build/path
            path=path.resolve(strict=True);libraries[str(path)]=digest(path);links.append(str(path))
        else:
            assert word.startswith('-'),word;links.append(word)
    assert str(build/'libhalo2_systems.a')in libraries and str(build/'libhalo2_physics_world_runtime.a')in libraries
    test='tests/native_workshop_camera_hs_tests.cpp';test_pin=digest(root/test)
    project=owner/'project';project.mkdir();shutil.copyfile(root/test,project/'camera_hs.cpp')
    (project/'tools').symlink_to(source/'tools',target_is_directory=True)
    cmake='''cmake_minimum_required(VERSION 3.20)
project(WorkshopCameraHs LANGUAGES CXX)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 REQUIRED)
include('''+json.dumps(str(source/'cmake/TestStorage.cmake'))+''')
add_executable(camera_hs camera_hs.cpp)
target_compile_features(camera_hs PRIVATE cxx_std_20)
target_compile_options(camera_hs PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_include_directories(camera_hs PRIVATE '''+json.dumps(str(source/'include'))+' '+json.dumps(str(source/'src'))+''')
target_link_libraries(camera_hs PRIVATE nlohmann_json::nlohmann_json SDL2::SDL2
'''+''.join('  '+json.dumps(word)+'\n'for word in links)+')\n'
    cache=root/json.loads((package/'selection.json').read_text())['cache']
    map_path=cache/'halo2/h2_maps_win64_dx11/00a_introduction.map';map_pin=digest(map_path)
    cmake+='add_test(NAME native_workshop_camera_hs COMMAND camera_hs '+json.dumps(str(map_path))+' "${CMAKE_CURRENT_BINARY_DIR}/camera-hs.json")\n'
    cmake+='set_tests_properties(native_workshop_camera_hs PROPERTIES TIMEOUT 180)\n'
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,str(root/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(owner/'builds'),
        '--output',str(args.output),'--target','camera_hs','--filter','^native_workshop_camera_hs$',
        '--jobs','2','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log','--retain-report','camera-hs.json']
    result=subprocess.run(command)
    assert digest(root/test)==test_pin and digest(link)==link_pin and digest(map_path)==map_pin
    assert all(digest(Path(name))==pin for name,pin in libraries.items())
    assert source_hashes(source)==declared and digest(package/'halo2_native')==qualification['runtime_sha256']
    report=dict(scope=__doc__,passed=result.returncode==0,qualification=str(args.qualification),
        qualification_sha256=digest(args.qualification),source_inventory_sha256=qualification['source_cohort']['inventory_sha256'],
        test_source_sha256=test_pin,helper_sha256=digest(Path(__file__)),link_template_sha256=link_pin,libraries=libraries,
        map=str(map_path),map_sha256=map_pin,command=command,exit_code=result.returncode,
        original_native_dependencies_not_intercepted=True,MCC_admitted=False,owned_scratch=str(owner),retained_large_artifacts=[])
    args.output.mkdir(parents=True,exist_ok=True);(args.output/'consumer.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
