#!/usr/bin/env python3
"""Apply a declared flying/secondary-heavy update after this owner's Random checks pass."""
import argparse
import hashlib
import json
import os
from pathlib import Path
from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('prepared','validation','output'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    base=json.loads(args.prepared.read_text());validation=json.loads(args.validation.read_text())
    binding=validation.get('vehicle_choice_source_binding',{});source=owner/'source';pins=dict(base['source_hashes'])
    if(Path(base['owner'])!=owner or not validation['passed'] or
        binding.get('preparation_sha256')!=sha(args.prepared) or binding.get('source_unchanged_after_run')is not True):
        raise ValueError('Require completed Random acceptance from the same unchanged source epoch')
    if any(sha(source/name)!=pin for name,pin in pins.items()):raise ValueError('Frozen source changed before refresh')
    names=('src/assets/content.cpp','tests/vehicle_choice_runtime_tu15_tests.cpp',
        'tools/generate_physics_live_support_tu15.py')
    changes=[];replacement={}
    for name in names:
        path=ROOT/name;before=sha(path);raw=path.read_bytes()
        if sha(path)!=before:raise ValueError('Source changed during declared refresh')
        replacement[name]=raw;changes.append(dict(path=name,before=pins.get(name),after=before))
    cmake=(source/'CMakeLists.txt').read_text();maps=Path(base['maps_directory'])
    extra=('flying-off','flying-on','secondary-heavy')
    for family in extra:
        test='vehicle_choice_runtime_'+family.replace('-','_')
        if test in cmake:raise ValueError('Extra authored setting already registered')
        cmake+=f'add_test(NAME {test} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- $<TARGET_FILE:vehicle_choice_runtime_tu15_tests> "{maps / "coagulation.map"}" "{maps}" {family} "${{CMAKE_CURRENT_BINARY_DIR}}/vehicle-choice-{family}.json")\n'
        cmake+=f'set_tests_properties({test} PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE)\n'
    replacement['CMakeLists.txt']=cmake.encode()
    changes.append(dict(path='CMakeLists.txt',before=pins['CMakeLists.txt'],after=hashlib.sha256(cmake.encode()).hexdigest()))
    args.output.mkdir(parents=True,exist_ok=False)
    for name,raw in replacement.items():
        (source/name).write_bytes(raw);pins[name]=sha(source/name)
    if any(sha(source/name)!=pin for name,pin in pins.items()):raise ValueError('Another frozen owner changed during refresh')
    report=dict(base,status='prepared',passed=False,validation_not_run=True,source_hashes=pins,
        changes=changes,previous_prepared_sha256=sha(args.prepared),previous_validation_sha256=sha(args.validation),
        tests=base['tests']+['vehicle_choice_runtime_'+family.replace('-','_')for family in extra],
        runtime_cases=['light','heavy','turret',*extra],full_vehicle_parity=False,
        tool_sha256=sha(Path(__file__)))
    (args.output/'prepared.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status='prepared',changed_files=len(changes),tests=len(report['tests']))),flush=True)


if __name__=='__main__':main()
