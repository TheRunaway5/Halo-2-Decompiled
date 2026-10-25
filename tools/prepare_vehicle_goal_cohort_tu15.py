#!/usr/bin/env python3
"""Register the complete vehicle investigation in an owned frozen build.

Use run_test.py and prepare the source with the weapon snapshot validator first.
Only unchanged-body build partitioning and test registration change the snapshot.
This preparation is not acceptance. Compilation remains in run_tests.py.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

from test_destruction_current_cohort import CASES as DESTRUCTION_CASES
from weapon_test_cache import sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepared', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--reuse-peer-stack', action='store_true',
                        help='Authenticate the same active owner\'s existing private dependency')
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    source = owner / 'source'
    prepared = json.loads(args.prepared.read_text())
    if Path(prepared['runner_scratch']).resolve() != owner:
        raise ValueError('Prepared source belongs to another scratch owner')
    expected = prepared['registered_snapshot_source_files_sha256']
    drift = [name for name, pin in expected.items() if sha(source / name) != pin]
    if drift:
        raise ValueError('Prepared source changed: ' + ', '.join(drift[:10]))
    args.output.mkdir(parents=True, exist_ok=False)
    changes = []
    for filename, target, unit in [
        ('cmake/PhysicsLiveWorldTu15.cmake', 'halo2_physics_world_runtime',
         'src/campaign/ai/source_ai_frame_tu15_generated.cpp')]:
        path = source / filename
        before = sha(path)
        text = path.read_text()
        needle = f'target_sources({target} PRIVATE {unit})'
        partition=f'halo2_partition_native_translation({target} {unit} 12)'
        if text.count(needle)==1:
            text=text.replace(needle,'include(cmake/NativeTranslationUnits.cmake)\n'+partition)
        elif text.count(partition)!=1:
            raise ValueError('Requalify native partition registration: ' + filename)
        path.write_text(text)
        changes.append(dict(path=filename, before=before, after=sha(path),
                            original_instruction_changes=[]))
    peer = Path.cwd() / 'build/_deps/halo2_datachannel-src'
    pin = subprocess.check_output(['git', '-C', str(peer), 'rev-parse', 'HEAD'], text=True).strip()
    if pin != '6b1e2e620f1e37f0eafeee702eaea0043cb305fd':
        raise ValueError('Borrowed peer dependency differs from the CMake pin')
    copied_peer = owner / 'peer-stack'
    if args.reuse_peer_stack:
        if copied_peer.is_symlink() or not copied_peer.is_dir():
            raise ValueError('Existing private peer dependency is absent')
    else:
        shutil.copytree(peer, copied_peer, ignore=shutil.ignore_patterns('.git'))
    peers = {str(p.relative_to(copied_peer)): sha(p)
             for p in copied_peer.rglob('*') if p.is_file()}
    if any(sha(peer / name) != digest for name, digest in peers.items()):
        raise ValueError('Borrowed peer dependency changed while copying')
    original_peers={str(p.relative_to(peer)):sha(p) for p in peer.rglob('*')
                    if p.is_file() and '.git' not in p.relative_to(peer).parts}
    if peers!=original_peers:raise ValueError('Private peer dependency inventory differs')
    (args.output / 'peer-inputs.json').write_text(json.dumps(
        dict(commit=pin, source=str(peer), copy=str(copied_peer), sha256=peers), indent=2) + '\n')
    data=source/'data'
    private_data={}
    if data.is_symlink():
        original=Path.cwd()/'data'
        if data.resolve()!=original.resolve():raise ValueError('Authored data alias has a foreign owner')
        if any(p.is_symlink() for p in original.rglob('*')):raise ValueError('Nested authored data alias')
        private_data={str(p.relative_to(original)):sha(p) for p in original.rglob('*') if p.is_file()}
        private=owner/'vehicle-private-data'
        shutil.copytree(original,private)
        if any(sha(private/name)!=pin or sha(original/name)!=pin for name,pin in private_data.items()):
            raise ValueError('Authored data changed during private copy')
        data.unlink();private.rename(data)
    else:
        private_data={str(p.relative_to(data)):sha(p) for p in data.rglob('*') if p.is_file()}
    maps = Path(prepared['runtime_cache_inputs']['maps_directory'])
    cmake = source / 'CMakeLists.txt'
    before = sha(cmake)
    text = '''\n# VEHICLE_GOAL_TU15
if(NOT TARGET destruction_runtime_tu15_tests)
  add_executable(destruction_runtime_tu15_tests tests/destruction_runtime_tu15_tests.cpp)
  target_link_libraries(destruction_runtime_tu15_tests PRIVATE halo2_systems halo2_authority_codec OpenSSL::Crypto)
endif()
'''
    for case, name in DESTRUCTION_CASES.items():
        text += (f'add_test(NAME vehicle_goal_destruction_{case} COMMAND '
                 '"${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" -- '
                 '$<TARGET_FILE:destruction_runtime_tu15_tests> '
                 f'"{maps / (name + ".map")}" "{maps}" "{case}" '
                 f'"${{CMAKE_CURRENT_BINARY_DIR}}/vehicle-destruction-{case}.json")\n'
                 f'set_tests_properties(vehicle_goal_destruction_{case} PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE '
                 'ENVIRONMENT "HALO2_DESTRUCTION_CONTINUATION=1;HALO2_DESTRUCTION_FIXED_CLOCK=1")\n')
    for case in ('ghost', 'banshee', 'scorpion', 'wraith', 'warthog-mg',
                 'warthog-gauss', 'spectre', 'h_turret_ap', 'c_turret_ap'):
        text += (f'add_test(NAME vehicle_goal_weapon_{case} COMMAND '
                 '"${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" -- '
                 '$<TARGET_FILE:vehicle_weapon_hud_world_tests> '
                 f'"{maps / "coagulation.map"}" "{maps}" native-mcc)\n'
                 f'set_tests_properties(vehicle_goal_weapon_{case} PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE '
                 f'ENVIRONMENT "HALO2_TEST_VEHICLE_WEAPON_HUD_CASE={case};HALO2_TEST_VEHICLE_WEAPON_FIRE=1;'
                 'HALO2_TEST_TURRET_SOUND_TRACE=1;'
                 f'HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/vehicle-weapon-{case}.json")\n')
    cmake.write_text(cmake.read_text() + text)
    changes.append(dict(path='CMakeLists.txt', before=before, after=sha(cmake),
                        original_instruction_changes=[]))
    report = dict(status='prepared', passed=False, validation_not_run=True,
                  owner=str(owner), source=str(source), prepared=str(args.prepared),
                  changes=changes, destruction_cases=DESTRUCTION_CASES,
                  private_data_sha256=private_data,
                  cmake_args=['-DFETCHCONTENT_SOURCE_DIR_HALO2_DATACHANNEL=' + str(copied_peer),
                              '-DHALO2_STAGE_BUILD_OUTPUT=OFF','-DHALO2_BUNDLE_MOVIES=OFF'],
                  native_execution='Compiled C++ native owners; original CPU execution is an offline oracle only',
                  tool_sha256=sha(Path(__file__)), large_artifacts_retained=[])
    (args.output / 'prepared.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(status='prepared', report=str(args.output / 'prepared.json'))))


if __name__ == '__main__':
    main()
