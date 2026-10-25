#!/usr/bin/env python3
"""Verify queued native packet volumes, including a private omission control."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--omit',action='store_true');args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source';inputs={}
    for rel,dest in (
        ('include/halo2/sdl_sound_platform.hpp','tools/loop-platform-include/halo2/sdl_sound_platform.hpp'),
        ('src/audio/sdl_sound_platform.cpp','tools/loop-platform.cpp'),
        ('tests/sdl_sound_platform_tests.cpp','tools/loop-platform-tests.cpp'),
        ('tests/sdl_sound_continuation_tests.cpp','tools/loop-platform-continuation.cpp')):
        data=(ROOT/rel).read_bytes();inputs[rel]=hashlib.sha256(data).hexdigest()
        if args.omit and rel.endswith('/sdl_sound_platform.cpp'):
            needle=b'const auto &matrix = packet?*packet:stream.parameters.matrix;'
            assert data.count(needle)==1;data=data.replace(needle,b'const auto &matrix = stream.parameters.matrix;')
        path=source/dest;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
    base=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio'
    label='loop-packet-platform-omit' if args.omit else 'loop-packet-platform'
    out=base/label;attempt=2
    while out.exists():out=base/(label+'-'+str(attempt));attempt+=1
    out.mkdir();cmake=source/'CMakeLists.txt';before=cmake.read_bytes()
    cmake.write_bytes(before+b'''
add_library(loop_packet_platform_audit STATIC tools/loop-platform.cpp)
target_include_directories(loop_packet_platform_audit PUBLIC tools/loop-platform-include)
target_link_libraries(loop_packet_platform_audit PUBLIC SDL2::SDL2 Threads::Threads)
add_executable(loop_packet_platform_audit_tests EXCLUDE_FROM_ALL tools/loop-platform-tests.cpp)
target_link_libraries(loop_packet_platform_audit_tests PRIVATE loop_packet_platform_audit)
add_executable(loop_packet_platform_continuation_audit EXCLUDE_FROM_ALL tools/loop-platform-continuation.cpp)
target_link_libraries(loop_packet_platform_continuation_audit PRIVATE loop_packet_platform_audit)
add_test(NAME loop_packet_platform_output COMMAND loop_packet_platform_audit_tests)
add_test(NAME loop_packet_platform_restore COMMAND loop_packet_platform_continuation_audit)
''')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','loop_packet_platform_audit_tests','--target','loop_packet_platform_continuation_audit',
        '--filter','^loop_packet_platform_(output|restore)$','--jobs','4','--test-jobs','1','--output',str(out)]
    try:
        with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    finally:cmake.write_bytes(before)
    receipt=dict(scope=__doc__,private_packet_volume_omitted=args.omit,command=command,exit_code=code,
        inputs=inputs,gameplay_verified=False,original_platform_parity_verified=False)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
    return code
if __name__=='__main__':raise SystemExit(main())
