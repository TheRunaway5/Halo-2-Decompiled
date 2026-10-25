#!/usr/bin/env python3
"""Qualify native detail arithmetic against complete original CPU executions."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source';inputs={}
    for rel,dest in (
        ('include/halo2/source_loop_detail_control_tu15.hpp','tools/loop-detail-include/halo2/source_loop_detail_control_tu15.hpp'),
        ('src/audio/source_loop_detail_control_tu15.cpp','tools/loop-detail.cpp'),
        ('tests/source_loop_detail_control_tu15_tests.cpp','tools/loop-detail-tests.cpp')):
        data=(ROOT/rel).read_bytes();inputs[rel]=hashlib.sha256(data).hexdigest()
        path=source/dest;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
    fixture=ROOT/'tests/fixtures/loop_detail_control_tu15.json';inputs[str(fixture.relative_to(ROOT))]=hashlib.sha256(fixture.read_bytes()).hexdigest()
    base=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio';out=base/'loop-detail-native';attempt=2
    while out.exists():out=base/('loop-detail-native-'+str(attempt));attempt+=1
    out.mkdir();cmake=source/'CMakeLists.txt';saved=cmake.read_bytes()
    cmake.write_bytes(saved+b'''
add_executable(loop_detail_control_audit EXCLUDE_FROM_ALL tools/loop-detail.cpp tools/loop-detail-tests.cpp)
target_include_directories(loop_detail_control_audit PRIVATE tools/loop-detail-include include)
target_link_libraries(loop_detail_control_audit PRIVATE halo2_glm nlohmann_json::nlohmann_json)
target_compile_options(loop_detail_control_audit PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME original_loop_detail_control_audit COMMAND loop_detail_control_audit "${PROJECT_SOURCE_DIR}/tests/fixtures/loop_detail_control_tu15.json")
''')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','loop_detail_control_audit','--filter','^original_loop_detail_control_audit$',
        '--jobs','4','--test-jobs','1','--output',str(out)]
    try:
        with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    finally:cmake.write_bytes(saved)
    receipt=dict(scope=__doc__,inputs=inputs,command=command,exit_code=code,
        source_ready_or_callee_results_supplied=False,detail_admission_or_playback_verified=False)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
    return code
if __name__=='__main__':raise SystemExit(main())
