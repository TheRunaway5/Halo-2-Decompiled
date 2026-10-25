#!/usr/bin/env python3
"""Register already implemented vehicle controls in an authenticated fresh build.

No gameplay or network source is refreshed. This avoids replaying historical
production fixes merely to register their tests in a newer source epoch.
"""
import argparse
import json
import os
from pathlib import Path
from weapon_test_cache import sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepared',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    base=json.loads(args.prepared.read_text());source=owner/'source'
    if Path(base['owner'])!=owner:raise ValueError('Use the same managed source owner')
    pins=dict(base['source_hashes'])
    if any(sha(source/name)!=pin for name,pin in pins.items()):raise ValueError('Undeclared source drift')
    tests=list(base['tests']);families=list(base.get('runtime_cases',['light','heavy','turret']))
    cmake=(source/'CMakeLists.txt').read_text();before=sha(source/'CMakeLists.txt')
    maps=Path(base['maps_directory'])
    for family in ('flying-off','flying-on','flying-random','secondary-heavy'):
        test='vehicle_choice_runtime_'+family.replace('-','_')
        if test in cmake:raise ValueError('Vehicle control already registered')
        cmake+=f'add_test(NAME {test} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- $<TARGET_FILE:vehicle_choice_runtime_tu15_tests> "{maps / "coagulation.map"}" "{maps}" {family} "${{CMAKE_CURRENT_BINARY_DIR}}/vehicle-choice-{family}.json")\n'
        cmake+=f'set_tests_properties({test} PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE)\n'
        tests.append(test);families.append(family)
    inputs=('tests/mcc_multiplayer_vehicle_table_tests.cpp','tests/native_grenade_wire_tests.cpp')
    if any(not (source/name).is_file() or name not in pins for name in inputs):
        raise ValueError('Fresh source omitted an existing regression input')
    for target in ('mcc_vehicle_table_goal_tests','native_grenade_wire_goal_tests'):
        if target in cmake:raise ValueError('Regression target already registered')
    cmake+='''
add_executable(mcc_vehicle_table_goal_tests tests/mcc_multiplayer_vehicle_table_tests.cpp)
target_link_libraries(mcc_vehicle_table_goal_tests PRIVATE halo2_systems)
add_test(NAME vehicle_table_original_tuple COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" -- $<TARGET_FILE:mcc_vehicle_table_goal_tests>)
add_executable(native_grenade_wire_goal_tests tests/native_grenade_wire_tests.cpp)
target_include_directories(native_grenade_wire_goal_tests PRIVATE include)
target_compile_features(native_grenade_wire_goal_tests PRIVATE cxx_std_20)
add_test(NAME vehicle_grenade_wire_original_counts COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" -- $<TARGET_FILE:native_grenade_wire_goal_tests>)
'''
    args.output.mkdir(parents=True,exist_ok=False)
    (source/'CMakeLists.txt').write_text(cmake);pins['CMakeLists.txt']=sha(source/'CMakeLists.txt')
    if any(sha(source/name)!=pin for name,pin in pins.items()):raise ValueError('Source changed during registration')
    report=dict(base,source_hashes=pins,status='prepared',passed=False,validation_not_run=True,
                tests=tests+['vehicle_table_original_tuple','vehicle_grenade_wire_original_counts'],
                runtime_cases=families,extra_targets=['mcc_vehicle_table_goal_tests','native_grenade_wire_goal_tests'],
                changes=[dict(path='CMakeLists.txt',before=before,after=pins['CMakeLists.txt'])],
                previous_prepared_sha256=sha(args.prepared),production_bodies_changed=False,
                tool_sha256=sha(Path(__file__)))
    (args.output/'prepared.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status='prepared',tests=len(report['tests']),production_bodies_changed=False)),flush=True)


if __name__=='__main__':main()
