#!/usr/bin/env python3
"""Build the real campaign eligibility regression and reject presence-only logic.

Private metadata mutations and all binaries belong to run_test.py scratch.
No original executable or gameplay is run by this registry policy test.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();args.output.mkdir(parents=True,exist_ok=True)
    header=ROOT/'src/audio/native_tu15_sound_campaign_audio.hpp';registry=ROOT/'src/audio/native_tu15_sound_kernels_campaign_audio.cpp'
    channels=ROOT/'src/audio/native_tu15_sound_kernels_campaign_channels.cpp';test=ROOT/'tests/native_campaign_audio_gate_tests.cpp'
    sources=[header,registry,channels,test,ROOT/'src/audio/native_campaign_audio_ducking_adapter.hpp',ROOT/'src/audio/native_campaign_audio_fades_adapter.hpp',ROOT/'src/audio/native_campaign_audio_retirement_adapter.hpp',ROOT/'src/audio/native_campaign_audio_stop_callback_adapter.hpp',ROOT/'src/core/native_tu15_data_array_delete.cpp',ROOT/'src/audio/native_tu15_sound_kernels_campaign_playback.cpp',Path(__file__)]
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(p.relative_to(ROOT)):sha(p)for p in sources})
    project=scratch/'project';project.mkdir();mutated=project/'mutated/audio';mutated.mkdir(parents=True)
    text=header.read_text();needle='!match->run || !match->semantic_original_proven'
    if text.count(needle)!=1:raise ValueError('Semantic eligibility mutation location changed')
    (mutated/header.name).write_text(text.replace(needle,'!match->run'))
    negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and 'Presence alone falsely qualified' in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint('Presence-only campaign qualification mutation rejected:',r.stderr.strip())\n")
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(campaign_audio_gate LANGUAGES CXX)\ninclude(CTest)\n'
    for target,includes in (('gate',ROOT/'src'),('gate_presence_only',project/'mutated')):
        cmake+=f'add_executable({target} "{test}" "{channels}" "{ROOT}/src/core/native_tu15_data_array_delete.cpp")\ntarget_include_directories({target} BEFORE PRIVATE "{includes}" "{ROOT}/src" "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\n'
        command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
        if target!='gate':command+=f'"{sys.executable}" "{negative}" '
        cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}>)\n'
    cmake+=f'add_library(registry_syntax OBJECT "{registry}")\ntarget_include_directories(registry_syntax PRIVATE "{ROOT}/include" "{ROOT}/src")\ntarget_compile_features(registry_syntax PRIVATE cxx_std_20)\n'
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),
             '--target','gate','--target','gate_presence_only','--target','registry_syntax','--filter','^gate(_presence_only)?$','--jobs','2','--test-jobs','1']
    result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode,
        source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),
        gate_policy='Every exact required entry must have a unique non-null callable and explicit semantic original proof',
        qualified_real_candidates=11,qualified_required_candidates=4,qualified_dependency_candidates=7,required_original_addresses=40,negative_control='Drop semantic-original proof from the shared registry predicate')
    report['passed']=result.returncode==0 and report['source_hashes_unchanged'];(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in ('passed','qualified_real_candidates','required_original_addresses')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
