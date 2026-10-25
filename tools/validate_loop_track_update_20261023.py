#!/usr/bin/env python3
"""Compare native track-request decision prefixes with actual original CPU execution."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--complete',action='store_true');args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source';inputs={}
    test='source_loop_track_decision_tu15_tests.cpp' if args.complete else 'source_loop_track_update_tu15_tests.cpp'
    fixture_name='loop_track_update_tu15.json' if args.complete else 'loop_track_update_prefix_tu15.json'
    for rel,dest in (
        ('include/halo2/source_loop_track_update_tu15.hpp','tools/loop-update-include/halo2/source_loop_track_update_tu15.hpp'),
        ('include/halo2/source_loop_track_transition_tu15.hpp','tools/loop-update-include/halo2/source_loop_track_transition_tu15.hpp'),
        ('src/audio/source_loop_track_update_tu15.cpp','tools/loop-update.cpp'),
        ('src/audio/source_loop_track_transition_tu15.cpp','tools/loop-update-transition.cpp'),
        ('tests/'+test,'tests/'+test)):
        data=(ROOT/rel).read_bytes();p=source/dest;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
        inputs[rel]=hashlib.sha256(data).hexdigest()
    fixture=ROOT/'tests/fixtures'/fixture_name;inputs[str(fixture.relative_to(ROOT))]=hashlib.sha256(fixture.read_bytes()).hexdigest()
    label='loop-decision-native' if args.complete else 'loop-update-native'
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio'/label;attempt=2
    while out.exists():out=out.with_name(label+'-'+str(attempt));attempt+=1
    out.mkdir();cmake=source/'CMakeLists.txt';before=cmake.read_bytes()
    cmake.write_bytes(before+('''
add_executable(loop_update_audit EXCLUDE_FROM_ALL tests/'''+test+''' tools/loop-update.cpp tools/loop-update-transition.cpp)
target_include_directories(loop_update_audit PRIVATE tools/loop-update-include include)
target_link_libraries(loop_update_audit PRIVATE nlohmann_json::nlohmann_json halo2_glm)
add_test(NAME original_loop_update_prefix_audit COMMAND loop_update_audit "${PROJECT_SOURCE_DIR}/tests/fixtures/'''+fixture_name+'''")
''').encode())
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','loop_update_audit','--filter','^original_loop_update_prefix_audit$',
        '--jobs','4','--test-jobs','1','--output',str(out)]
    try:
        with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    finally:cmake.write_bytes(before)
    receipt=dict(scope=__doc__,command=command,exit_code=code,inputs=inputs,native_PCM_verified=False,
        backend_results_supplied=args.complete,backend_result_scope='Explicit component playback ABI only' if args.complete else 'None',
        completed_loop_update_verified=args.complete and code==0,native_playback_verified=False)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
    return code
if __name__=='__main__':raise SystemExit(main())
