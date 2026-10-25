#!/usr/bin/env python3
"""Prepare a canonical owned native build for actual per-birth vehicle-choice acceptance."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepared',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    prepared=json.loads(args.prepared.read_text());source=owner/'source'
    if Path(prepared['runner_scratch'])!=owner:raise ValueError('Source owner differs')
    expected=prepared['registered_snapshot_source_files_sha256']
    if any(sha(source/name)!=pin for name,pin in expected.items()):raise ValueError('Frozen source changed')
    manifest=json.loads((args.prepared.parent/'source-manifest.json').read_text())['source_files_sha256']
    observation=json.loads((args.prepared.parent/'vehicle-projectile-birth-retirement-observation.json').read_text())
    replacements={}
    for row in observation['private_generated_owner_instrumentation']:
        path=source/row['path'];text=path.read_text();prefix='#include "halo2/source_physics_entry_observation.hpp"\n'
        if sha(path)!=row['observed_sha256'] or not text.startswith(prefix):raise ValueError('Diagnostic preimage changed')
        text=text[len(prefix):]
        for entry in row['entries']:
            needle=f'\nSourcePhysicsEntryObservation diagnostic(s.memory,0x{int(entry,16):x}u,[&]{{return s.save();}});'
            if text.count(needle)!=1:raise ValueError('Private observation hook changed')
            text=text.replace(needle,'',1)
        raw=text.encode()
        if hashlib.sha256(raw).hexdigest()!=row['original_sha256']:raise ValueError('Canonical original source differs')
        replacements[row['path']]=raw
    raw=(source/'CMakeLists.txt').read_bytes();digest=hashlib.sha256();end=0
    for line in raw.splitlines(keepends=True):
        end+=len(line);digest.update(line)
        if digest.hexdigest()==manifest['CMakeLists.txt']:break
    else:raise ValueError('Frozen canonical CMake prefix is absent')
    canonical_cmake=raw[:end]
    name='tests/vehicle_choice_runtime_tu15_tests.cpp';test=ROOT/name;pin=sha(test);data=test.read_bytes()
    if sha(test)!=pin:raise ValueError('Runtime acceptance source changed')
    replacements[name]=data
    cmake='''\n# Actual authored Random vehicle births and completed template binding.
add_executable(vehicle_choice_runtime_tu15_tests tests/vehicle_choice_runtime_tu15_tests.cpp)
target_link_libraries(vehicle_choice_runtime_tu15_tests PRIVATE halo2_systems halo2_authority_codec OpenSSL::Crypto)
'''
    maps=Path(prepared['runtime_cache_inputs']['maps_directory'])
    for family in ('light','heavy','turret'):
        cmake+=f'add_test(NAME vehicle_choice_runtime_{family} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- $<TARGET_FILE:vehicle_choice_runtime_tu15_tests> "{maps / "coagulation.map"}" "{maps}" {family} "${{CMAKE_CURRENT_BINARY_DIR}}/vehicle-choice-{family}.json")\n'
        cmake+=f'set_tests_properties(vehicle_choice_runtime_{family} PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE)\n'
    replacements['CMakeLists.txt']=canonical_cmake+cmake.encode()
    peer=ROOT/'build/_deps/halo2_datachannel-src';copy=owner/'peer-stack'
    commit=subprocess.check_output(['git','-C',str(peer),'rev-parse','HEAD'],text=True).strip()
    if commit!='6b1e2e620f1e37f0eafeee702eaea0043cb305fd':raise ValueError('Peer reference is not the source pin')
    shutil.copytree(peer,copy,ignore=shutil.ignore_patterns('.git'))
    peer_pins={str(path.relative_to(copy)):sha(path)for path in copy.rglob('*')if path.is_file()}
    if any(sha(peer/name)!=pin for name,pin in peer_pins.items()):raise ValueError('Peer copy differs')
    args.output.mkdir(parents=True,exist_ok=False)
    changes=[]
    for name,data in replacements.items():
        path=source/name;before=sha(path)if path.exists()else None;path.write_bytes(data)
        changes.append(dict(path=name,before=before,after=sha(path)));expected[name]=sha(path)
    if any(sha(source/name)!=pin for name,pin in expected.items()):raise ValueError('Another frozen source file changed')
    report=dict(status='prepared',passed=False,validation_not_run=True,owner=str(owner),source=str(source),
        changes=changes,source_hashes=expected,canonical_source_hashes=manifest,canonical_cmake_sha256=manifest['CMakeLists.txt'],
        source_instruction_changes=[],private_diagnostic_hooks_removed=True,peer_commit=commit,
        peer_hashes=peer_pins,peer_copy=str(copy),maps_directory=str(maps),
        tests=['vehicle_choice_runtime_'+family for family in ('light','heavy','turret')],
        prepared_sha256=sha(args.prepared),full_vehicle_parity=False,tool_sha256=sha(Path(__file__)))
    (args.output/'prepared.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status='prepared',owner=str(owner),source_files=len(expected))),flush=True)


if __name__=='__main__':main()
