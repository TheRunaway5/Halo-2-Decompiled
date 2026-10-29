#!/usr/bin/env python3
"""Validate the real speech queue CMake module and reference-admission failures."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'module-project';project.mkdir()
    sources=[ROOT/'cmake/CampaignNativeAiSpeechQueue.cmake',ROOT/'include/halo2/native_campaign_ai_speech_queue_tu15.hpp',ROOT/'src/campaign/ai/native_campaign_ai_speech_queue_tu15.cpp',ROOT/'tools/validate_native_campaign_ai_speech_queue_tu15.py',Path(__file__).resolve(),ROOT/'cmake/NativeTu15DataArrayDelete.cmake',ROOT/'src/core/native_tu15_data_array_delete.cpp']
    pins={str(path.relative_to(ROOT)):sha(path)for path in sources};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[])
    for name in ('src','include','cmake','tools','tests'):(project/name).symlink_to(ROOT/name,target_is_directory=True)
    (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.24)
project(NativeCampaignSpeechQueueModule LANGUAGES CXX)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(cmake/TestStorage.cmake)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeAiSpeechQueue.cmake)
''')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--target','halo2_physics_world_runtime','--target','native_campaign_ai_speech_queue_tu15_tests','--filter','^native_campaign_ai_speech_queue_tu15$','--jobs','2','--test-jobs','1','--retain-report','campaign-ai-speech-queue/report.json']
    result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
    spec=importlib.util.spec_from_file_location('actual_speech_validation',ROOT/'tools/validate_native_campaign_ai_speech_queue_tu15.py');module=importlib.util.module_from_spec(spec);sys.path.insert(0,str(ROOT/'tools'));spec.loader.exec_module(module)
    previous_argv=sys.argv
    try:
        module.ORIGINAL=scratch/'missing-owned-reference.xbe';missing=scratch/'missing-report';sys.argv=[str(spec.origin),'--output',str(missing)]
        code=module.main();admission=json.loads((missing/'report.json').read_text());report['missing_reference']=dict(exit_code=code,report=admission)
        bad=scratch/'incorrect-owned-reference.xbe';bad.write_bytes(b'not the authenticated owned reference');module.ORIGINAL=bad;sys.argv=[str(spec.origin),'--output',str(scratch/'bad-pin-report')]
        try:module.main()
        except ValueError as error:report['pin_mismatch']=dict(rejected='image differs' in str(error),error=str(error))
        else:report['pin_mismatch']=dict(rejected=False)
    finally:sys.argv=previous_argv
    report['source_hashes_unchanged']=all(sha(ROOT/name)==pin for name,pin in pins.items())
    report['passed']=result.returncode==0 and report['source_hashes_unchanged'] and code==77 and admission['passed']is False and admission['validation_not_run']is True and report['pin_mismatch']['rejected']
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
