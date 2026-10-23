#!/usr/bin/env python3
"""Freeze an already linked cohort for focused native Audio regressions.

Only immutable archives from a completed build are imported. Audio's complete
translation unit is compiled explicitly before those archives; its old archive
member is not selected. This is component evidence, not a fresh full-game build.
Run in an active tools/run_test.py owner; all archive copies are disposable.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--parent-report',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    parent=json.loads(args.parent_report.read_text());build=Path(parent['build'])
    assert any(row['name']=='build' and row.get('exit_code')==0 for row in parent['steps']),parent
    source=scratch/'source';source.mkdir()
    frozen=build.parent/'source'
    shutil.copytree(frozen/'include',source/'include')
    (source/'src/audio').mkdir(parents=True)
    for path in (frozen/'src/audio').glob('*.hpp'):shutil.copyfile(path,source/'src/audio'/path.name)
    core='src/audio/multiplayer_audio.cpp';test='tests/mcc_sound_rate_tests.cpp'
    # Initially this is the exact Audio source of the completed parent link.
    assert sha(ROOT/core)==sha(frozen/core),'Declare a subsequent Audio refresh separately'
    shutil.copyfile(frozen/core,source/core)
    (source/'tests').mkdir();shutil.copyfile(ROOT/test,source/test)
    (source/'tools').mkdir();shutil.copyfile(ROOT/'tools/run_test.py',source/'tools/run_test.py')
    words=shlex.split((build/'CMakeFiles/mcc_sound_rate_tests.dir/link.txt').read_text())
    start=words.index('-o')+2
    pins={};copied={}
    def copy(path):
        path=path.resolve(strict=True)
        if path in copied:return copied[path]
        relative=path.relative_to(build) if path.is_relative_to(build) else Path('external')/path.name
        target=source/'archives'/relative;target.parent.mkdir(parents=True,exist_ok=True)
        before=sha(path);shutil.copyfile(path,target)
        assert before==sha(path)==sha(target),'Archive changed during its frozen copy'
        pins[str(path)]=dict(sha256=before,bytes=path.stat().st_size,copy=str(target))
        copied[path]=target;return target
    libraries=[]
    for word in words[start:]:
        if word.endswith('.a'):
            path=Path(word);libraries.append(str(copy(path if path.is_absolute() else build/path)))
        elif word.startswith('-Wl,@'):
            libraries.append('-Wl,@'+str(copy(Path(word[5:]))))
        else:libraries.append(word)
    cmake='''cmake_minimum_required(VERSION 3.20)
project(Halo2AudioComponent LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(SDL2 REQUIRED)
find_package(nlohmann_json REQUIRED)
include(CTest)
add_executable(mcc_sound_rate_tests tests/mcc_sound_rate_tests.cpp src/audio/multiplayer_audio.cpp)
target_include_directories(mcc_sound_rate_tests PRIVATE include src/audio)
target_compile_options(mcc_sound_rate_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(mcc_sound_rate_tests PRIVATE nlohmann_json::nlohmann_json SDL2::SDL2
'''+''.join('  "'+word.replace('\\','/')+'"\n'for word in libraries)+''')
add_test(NAME mcc_sound_codec_rates COMMAND "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" -- "$<TARGET_FILE:mcc_sound_rate_tests>")
set_tests_properties(mcc_sound_codec_rates PROPERTIES TIMEOUT 120 ENVIRONMENT "SDL_AUDIODRIVER=dummy")
'''
    (source/'CMakeLists.txt').write_text(cmake)
    args.output.mkdir(parents=True,exist_ok=True)
    report=dict(status='prepared',passed=False,validation_not_run=True,scope=__doc__,
        source=str(source),parent_report=str(args.parent_report.resolve()),parent_validation_status=parent['status'],
        archives=pins,source_files={name:sha(source/name)for name in (core,test,'CMakeLists.txt')},
        tool_sha256=sha(Path(__file__)),large_artifacts_retained=[])
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status='prepared',archives=len(pins),bytes=sum(row['bytes']for row in pins.values()),
        source=str(source),passed=False)),flush=True)


if __name__=='__main__':main()
