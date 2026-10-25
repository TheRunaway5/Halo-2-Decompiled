#!/usr/bin/env python3
"""Admit only the authored vehicle-table projection into this qualified owned source epoch."""
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
    source=owner/'source';pins=dict(base['source_hashes']);binding=validation.get('vehicle_choice_source_binding',{})
    if(Path(base['owner'])!=owner or not validation['passed'] or binding.get('preparation_sha256')!=sha(args.prepared) or
        binding.get('source_unchanged_after_run')is not True):raise ValueError('Require the same completed source epoch')
    if any(sha(source/name)!=pin for name,pin in pins.items()):raise ValueError('Source drift before projection refresh')
    header='include/halo2/mcc_gameplay_reference.hpp';module='src/assets/mcc/mcc_gameplay_reference.cpp';cache='src/assets/mcc_cache.cpp'
    replacements={name:(ROOT/name).read_text()for name in (header,module,cache,
        'tests/vehicle_choice_runtime_tu15_tests.cpp','tests/mcc_multiplayer_vehicle_table_tests.cpp')}
    old=replacements[header];a=old.index('struct MccMultiplayerVehicleTableReceipt');b=old.index('struct MccBspInstanceTransformReceipt',a)
    if old[:a]+old[b:]!=(source/header).read_text():raise ValueError('An unrelated reference contract would change')
    old=replacements[module];a=old.index('std::optional<MccMultiplayerVehicleTableReceipt> convert_mcc_multiplayer_vehicle_table(')
    b=old.index('std::optional<MccBspInstanceTransformReceipt> convert_mcc_bsp_instance_transform(',a)
    if old[:a]+old[b:]!=(source/module).read_text():raise ValueError('An unrelated reference converter would change')
    call='    for(const auto& t:tags_)if(t.address && t.size && t.type()==tag_groups::multiplayer_globals)\n        (void)convert_mcc_multiplayer_vehicle_table(t.type(),t.name,t.address,reference_access);\n'
    if replacements[cache].count(call)!=1 or replacements[cache].replace(call,'')!=(source/cache).read_text():
        raise ValueError('An unrelated MCC loader owner would change')
    cmake=(source/'CMakeLists.txt').read_text()
    cmake+='\nadd_executable(mcc_vehicle_table_goal_tests tests/mcc_multiplayer_vehicle_table_tests.cpp)\n'
    cmake+='target_link_libraries(mcc_vehicle_table_goal_tests PRIVATE halo2_cache)\n'
    cmake+='add_test(NAME vehicle_table_original_tuple COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" -- $<TARGET_FILE:mcc_vehicle_table_goal_tests>)\n'
    replacements['CMakeLists.txt']=cmake
    changes=[];args.output.mkdir(parents=True,exist_ok=False)
    for name,text in replacements.items():
        data=text.encode();path=source/name;before=sha(path)if path.exists()else None
        path.write_bytes(data);after=sha(path)
        if after!=hashlib.sha256(data).hexdigest():raise ValueError('Refresh bytes differ')
        pins[name]=after;changes.append(dict(path=name,before=before,after=after))
    if any(sha(source/name)!=pin for name,pin in pins.items()):raise ValueError('Source drift during projection refresh')
    report=dict(base,status='prepared',passed=False,validation_not_run=True,source_hashes=pins,changes=changes,
        previous_prepared_sha256=sha(args.prepared),previous_validation_sha256=sha(args.validation),
        tests=base['tests']+['vehicle_table_original_tuple'],extra_targets=['mcc_vehicle_table_goal_tests'],
        original_table_census_sha256=sha(ROOT/'analysis/vehicles-tu15-goal-20261023/vehicle-runtime-table-census-v1.json'),
        full_vehicle_parity=False,tool_sha256=sha(Path(__file__)))
    (args.output/'prepared.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status='prepared',changed_files=len(changes),tests=len(report['tests']))),flush=True)


if __name__=='__main__':main()
