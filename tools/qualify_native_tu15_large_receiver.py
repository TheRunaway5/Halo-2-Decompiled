#!/usr/bin/env python3
"""Freeze and compare whole native large-sphere BSP preparation in owned CMake storage."""
import argparse,hashlib,json,os,shlex,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
CPP=['src/rendering/world/source_portal_visibility.cpp','src/rendering/world/source_portal_frustum.cpp',
    'src/rendering/world/source_portal_clip.cpp','src/rendering/world/source_frustum_camera.cpp',
    'src/rendering/world/source_visibility.cpp','src/rendering/shadows/source_shadow_view.cpp','src/core/transform.cpp',
    'src/rendering/world/source_bsp_admission.cpp','src/rendering/effects/effect_light_receiver.cpp',
    'src/rendering/effects/effect_light_receiver_flags.cpp','src/gameplay/objects/object_light_spatial.cpp',
    'src/gameplay/objects/source_object_location.cpp','src/rendering/world/bsp_atmosphere.cpp','src/assets/cache.cpp']
NAMES=CPP+['tests/effect_light_receiver_tests.cpp','tools/oracle_effect_light_large_receiver.py',
    'tools/oracle_effect_light_snapshot.py','tools/qualify_native_tu15_large_receiver.py']


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--gcc-only',action='store_true');a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=scratch/'source';source.mkdir();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=True)
    sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
    shutil.copytree(ROOT/'include',source/'include')
    for name in NAMES:
        target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    (source/'analysis').symlink_to(ROOT/'analysis',target_is_directory=True)
    dependencies=set(NAMES)
    for name in CPP+['tests/effect_light_receiver_tests.cpp']:
        cmd=['g++','-std=c++20','-DHALO2_EFFECT_RECEIVER_BANK_PROBE','-MM','-I'+str(source/'include'),str(source/name)]
        result=subprocess.run(cmd,capture_output=True,text=True,check=True)
        for token in shlex.split(result.stdout.replace('\\\n',' ').split(':',1)[1]):
            path=Path(token).resolve()
            if path.is_relative_to(source):dependencies.add(path.relative_to(source).as_posix())
    pins={name:sha(source/name)for name in sorted(dependencies)}
    (source/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(NativeLargeReceiver LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include("'''+str(ROOT/'cmake/TestStorage.cmake')+'''")
add_executable(receiver_probe tests/effect_light_receiver_tests.cpp '''+' '.join(CPP)+''')
target_include_directories(receiver_probe PRIVATE include)
target_link_libraries(receiver_probe PRIVATE nlohmann_json::nlohmann_json)
target_compile_features(receiver_probe PRIVATE cxx_std_20)
target_compile_definitions(receiver_probe PRIVATE HALO2_EFFECT_RECEIVER_BANK_PROBE)
target_compile_options(receiver_probe PRIVATE -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off)
target_link_options(receiver_probe PRIVATE -Wl,--gc-sections)
if(HALO2_SANITIZERS)
 target_compile_options(receiver_probe PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
 target_link_options(receiver_probe PRIVATE -fsanitize=address,undefined)
endif()
add_test(NAME original_large_receiver_banks COMMAND "'''+str(ROOT/'.tools/python/bin/python')+'''"
 "${CMAKE_CURRENT_SOURCE_DIR}/tools/oracle_effect_light_large_receiver.py" --native $<TARGET_FILE:receiver_probe>
 --native-banks --report "${CMAKE_CURRENT_BINARY_DIR}/large-receiver-banks.json")
set_tests_properties(original_large_receiver_banks PROPERTIES TIMEOUT 600)
''')
    results=[]
    for kind,compiler,sanitizers in [('gcc','g++',False)]+([]if a.gcc_only else[('clang-sanitizers','clang++',True)]):
        cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--scratch-root',str(scratch/'builds'),
            '--output',str(a.output/kind),'--target','receiver_probe','--filter','^original_large_receiver_banks$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
            '--retain-report','large-receiver-banks.json','--retain-report','Testing/Temporary/LastTest.log']
        if sanitizers:cmd.append('--sanitizers')
        result=subprocess.run(cmd);results.append(dict(kind=kind,command=cmd,exit_code=result.returncode))
        if result.returncode:break
    unchanged=all(sha(ROOT/n)==h for n,h in pins.items())
    report=dict(passed=unchanged and len(results)==(1 if a.gcc_only else 2)and all(r['exit_code']==0 for r in results),
        results=results,source_sha256=pins,source_unchanged=unchanged,owned_scratch=str(scratch),retained_large_artifacts=[],
        scope='Native combined receiver camera/portal/BSP admission prefix compared with unchanged original calls. '
            'Every consumed BSP field comes from retained RAM. Containing cluster and original fog result are explicit inputs. '
            'Cache admission, dynamic objects, live World and rendered frames remain separate.')
    (a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
