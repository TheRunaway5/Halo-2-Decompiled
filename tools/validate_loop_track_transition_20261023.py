#!/usr/bin/env python3
"""Compare the native transition leaf to401408 independent original executions."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    inputs={}
    for relative,destination in (
        ('include/halo2/source_loop_track_transition_tu15.hpp','tools/loop-track-include/halo2/source_loop_track_transition_tu15.hpp'),
        ('src/audio/source_loop_track_transition_tu15.cpp','tools/loop-track-transition.cpp'),
        ('tests/source_loop_track_transition_tu15_tests.cpp','tests/source_loop_track_transition_tu15_tests.cpp'),
        ('tests/fixtures/loop_track_transition_tu15.json','tests/fixtures/loop_track_transition_tu15.json')):
        path=source/destination;path.parent.mkdir(parents=True,exist_ok=True)
        path.write_bytes((ROOT/relative).read_bytes());inputs[relative]=sha(path)
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio/track-transition-native'
    attempt=2
    while out.exists():out=out.with_name('track-transition-native-'+str(attempt));attempt+=1
    out.mkdir()
    cmake=source/'CMakeLists.txt';before=cmake.read_bytes()
    cmake.write_bytes(before+b'''
add_executable(loop_track_transition_audit EXCLUDE_FROM_ALL tests/source_loop_track_transition_tu15_tests.cpp tools/loop-track-transition.cpp)
target_include_directories(loop_track_transition_audit PRIVATE tools/loop-track-include)
target_link_libraries(loop_track_transition_audit PRIVATE OpenSSL::Crypto nlohmann_json::nlohmann_json)
add_test(NAME original_loop_track_transition_audit COMMAND loop_track_transition_audit "${PROJECT_SOURCE_DIR}/tests/fixtures/loop_track_transition_tu15.json")
''')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','loop_track_transition_audit','--filter','^original_loop_track_transition_audit$',
        '--jobs','4','--test-jobs','1','--output',str(out)]
    try:
        with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    finally:cmake.write_bytes(before)
    receipt=dict(scope=__doc__,command=command,exit_code=code,inputs=inputs,
        root_inputs_unchanged=all(sha(ROOT/p)==value for p,value in inputs.items()),playback_verified=False)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
    return code
if __name__=='__main__':raise SystemExit(main())
