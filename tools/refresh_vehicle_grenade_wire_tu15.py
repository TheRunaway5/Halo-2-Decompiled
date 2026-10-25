#!/usr/bin/env python3
"""Install the qualified original-range snapshot codec into the same owned vehicle build."""
import argparse
import json
import os
from pathlib import Path
from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('prepared','validation','qualification','output'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    base=json.loads(args.prepared.read_text());v=json.loads(args.validation.read_text());q=json.loads(args.qualification.read_text())
    source=owner/'source';pins=dict(base['source_hashes']);binding=v.get('vehicle_choice_source_binding',{})
    if(Path(base['owner'])!=owner or not v['passed'] or binding.get('preparation_sha256')!=sha(args.prepared) or
        not binding.get('source_unchanged_after_run') or not q['passed'] or not q['source_binding']['source_unchanged']):
        raise ValueError('Require the completed source epoch and exhaustive original-range codec qualification')
    if any(sha(source/name)!=pin for name,pin in pins.items()):raise ValueError('Frozen source changed')
    network='src/network/multiplayer_network.cpp';protocol='include/halo2/multiplayer_datagrams.hpp'
    header='include/halo2/native_grenade_wire.hpp';test='tests/native_grenade_wire_tests.cpp'
    for name in(header,test):
        if sha(ROOT/name)!=q['source_binding']['source_hashes'][name]:raise ValueError('Qualified codec source changed')
    replacements={name:(ROOT/name).read_text()for name in(network,protocol,header,test)}
    original=(source/network).read_text();candidate=replacements[network]
    a=original.index('    if (u.grenades[0] > 2');b=original.index('    for(auto b:u.grenade_action.source)',a)
    old_encode=original[a:b]
    a=original.index('    const auto grenades = r.get(1);');b=original.index('    for(auto&b:u.grenade_action.source)',a)
    old_decode=original[a:b]
    encoded='    put(p,encode_native_grenades(u.grenades,u.grenade_current),1);\n'
    decoded='    const auto grenades=decode_native_grenades(std::uint8_t(r.get(1)));\n    u.grenades=grenades.counts;u.grenade_current=grenades.selected;\n'
    include='#include "halo2/native_grenade_wire.hpp"\n'
    if any(candidate.count(needle)!=1 for needle in(encoded,decoded,include)):raise ValueError('Codec integration seam changed')
    restored=candidate.replace(include,'').replace(encoded,old_encode).replace(decoded,old_decode)
    if restored!=original:raise ValueError('An unrelated network owner would change')
    current=replacements[protocol]
    restored=current.replace('//103 preserves the original maximum of four grenades per type in snapshots.\n','').replace(
        'inline constexpr unsigned native_multiplayer_protocol = 103;',
        'inline constexpr unsigned native_multiplayer_protocol = 102;')
    if restored!=(source/protocol).read_text():raise ValueError('An unrelated protocol contract would change')
    cmake=(source/'CMakeLists.txt').read_text()
    cmake+='\nadd_executable(native_grenade_wire_goal_tests tests/native_grenade_wire_tests.cpp)\n'
    cmake+='target_include_directories(native_grenade_wire_goal_tests PRIVATE include)\n'
    cmake+='target_compile_features(native_grenade_wire_goal_tests PRIVATE cxx_std_20)\n'
    cmake+='add_test(NAME vehicle_grenade_wire_original_counts COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" -- $<TARGET_FILE:native_grenade_wire_goal_tests>)\n'
    replacements['CMakeLists.txt']=cmake;args.output.mkdir(parents=True,exist_ok=False);changes=[]
    for name,text in replacements.items():
        path=source/name;before=sha(path)if path.exists()else None;path.write_text(text);pins[name]=sha(path)
        changes.append(dict(path=name,before=before,after=pins[name]))
    if any(sha(source/name)!=pin for name,pin in pins.items()):raise ValueError('Source changed during codec refresh')
    report=dict(base,status='prepared',passed=False,validation_not_run=True,source_hashes=pins,changes=changes,
        previous_prepared_sha256=sha(args.prepared),previous_validation_sha256=sha(args.validation),
        qualification_sha256=sha(args.qualification),protocol=103,
        tests=base['tests']+['vehicle_grenade_wire_original_counts'],
        extra_targets=base['extra_targets']+['native_grenade_wire_goal_tests'],full_vehicle_parity=False,tool_sha256=sha(Path(__file__)))
    (args.output/'prepared.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status='prepared',tests=len(report['tests']),protocol=103)),flush=True)


if __name__=='__main__':main()
