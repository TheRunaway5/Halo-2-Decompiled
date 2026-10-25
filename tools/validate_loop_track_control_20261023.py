#!/usr/bin/env python3
"""Compare native loop-control arithmetic with unchanged original CPU owners."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source';inputs={}
    for rel,dest in (
        ('include/halo2/source_loop_track_control_tu15.hpp','tools/loop-control-include/halo2/source_loop_track_control_tu15.hpp'),
        ('src/audio/source_loop_track_control_tu15.cpp','tools/loop-control.cpp'),
        ('tests/source_loop_track_control_tu15_tests.cpp','tests/source_loop_track_control_tu15_tests.cpp'),
        ('tests/fixtures/loop_track_control_tu15.json','tests/fixtures/loop_track_control_tu15.json')):
        data=(ROOT/rel).read_bytes();p=source/dest;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
        inputs[rel]=hashlib.sha256(data).hexdigest()
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio/loop-control-native';attempt=2
    while out.exists():out=out.with_name('loop-control-native-'+str(attempt));attempt+=1
    out.mkdir();cmake=source/'CMakeLists.txt';before=cmake.read_bytes()
    cmake.write_bytes(before+b'''
add_executable(loop_control_audit EXCLUDE_FROM_ALL tests/source_loop_track_control_tu15_tests.cpp tools/loop-control.cpp)
target_include_directories(loop_control_audit PRIVATE tools/loop-control-include)
target_link_libraries(loop_control_audit PRIVATE nlohmann_json::nlohmann_json)
target_compile_options(loop_control_audit PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME original_loop_control_audit COMMAND loop_control_audit "${PROJECT_SOURCE_DIR}/tests/fixtures/loop_track_control_tu15.json")
''')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','loop_control_audit','--filter','^original_loop_control_audit$',
        '--jobs','4','--test-jobs','1','--output',str(out)]
    try:
        with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    finally:cmake.write_bytes(before)
    receipt=dict(scope=__doc__,command=command,exit_code=code,inputs=inputs,native_PCM_verified=False)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
    return code
if __name__=='__main__':raise SystemExit(main())
