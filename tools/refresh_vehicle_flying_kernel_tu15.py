#!/usr/bin/env python3
"""Admit only the four-family qualified native selector into this owned build."""
import argparse
import json
import os
from pathlib import Path
from native_vehicle_choice_kernel_tu15 import strip_native_vehicle_choice
from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('prepared','validation','qualification','regeneration','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    base=json.loads(args.prepared.read_text());validation=json.loads(args.validation.read_text())
    qualification=json.loads(args.qualification.read_text());regeneration=json.loads(args.regeneration.read_text())
    binding=validation.get('vehicle_choice_source_binding',{})
    if (Path(base['owner'])!=owner or not validation['passed'] or
        binding.get('preparation_sha256')!=sha(args.prepared) or
        not binding.get('source_unchanged_after_run') or not qualification['passed'] or
        not qualification['production_route_installed'] or qualification['cases']!=3444 or
        qualification['comparisons']!=6888 or not qualification['source_unchanged'] or
        not regeneration['passed'] or not regeneration['source_unchanged'] or
        {row['entry'] for row in regeneration['selectors'] if row['equal']}!={'15cb60','15cc70','15cd30','15cdf0'}):
        raise ValueError('Require complete current native epochs, four installed selectors and exact regeneration')
    source=owner/'source';pins=dict(base['source_hashes'])
    if any(sha(source/name)!=pin for name,pin in pins.items()):raise ValueError('Frozen source changed')
    module='src/physics/physics_live_support_tu15_generated.cpp'
    kernel='src/gameplay/objects/source_vehicle_choice_native_tu15_internal.hpp'
    if sha(ROOT/module)!=qualification['literal_module_sha256'] or sha(ROOT/module)!=regeneration['source_sha256']:
        raise ValueError('Qualified complete module changed')
    if sha(ROOT/kernel)!=qualification['native_kernel_sha256']:
        raise ValueError('Qualified kernel changed')
    if strip_native_vehicle_choice((source/module).read_text())!=strip_native_vehicle_choice((ROOT/module).read_text()):
        raise ValueError('Another original owner would change')
    names=(module,kernel,'tools/generate_physics_live_support_tu15.py',
           'tools/native_vehicle_choice_kernel_tu15.py','tests/source_vehicle_choice_native_tu15_tests.cpp',
           'tests/vehicle_choice_runtime_tu15_tests.cpp')
    replacement={name:(ROOT/name).read_bytes() for name in names}
    expected={module:qualification['literal_module_sha256'],kernel:qualification['native_kernel_sha256'],
              **regeneration['dependency_hashes'],
              'tests/source_vehicle_choice_native_tu15_tests.cpp':qualification['source_hashes']['tests/source_vehicle_choice_native_tu15_tests.cpp'],
              'tests/vehicle_choice_runtime_tu15_tests.cpp':sha(ROOT/'tests/vehicle_choice_runtime_tu15_tests.cpp')}
    import hashlib
    if any(hashlib.sha256(raw).hexdigest()!=expected[name] or sha(ROOT/name)!=expected[name]
           for name,raw in replacement.items()):raise ValueError('A qualified dependency changed')
    tests=list(base['tests']);families=list(base['runtime_cases'])
    if 'flying-random' not in families:
        cmake=(source/'CMakeLists.txt').read_text();maps=Path(base['maps_directory'])
        test='vehicle_choice_runtime_flying_random'
        if test in cmake:raise ValueError('Flying Random registration differs from its manifest')
        cmake+=f'add_test(NAME {test} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- $<TARGET_FILE:vehicle_choice_runtime_tu15_tests> "{maps / "coagulation.map"}" "{maps}" flying-random "${{CMAKE_CURRENT_BINARY_DIR}}/vehicle-choice-flying-random.json")\n'
        cmake+=f'set_tests_properties({test} PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE)\n'
        replacement['CMakeLists.txt']=cmake.encode();tests.append(test);families.append('flying-random')
    args.output.mkdir(parents=True,exist_ok=False);changes=[]
    for name,raw in replacement.items():
        path=source/name;before=sha(path);path.write_bytes(raw);pins[name]=sha(path)
        changes.append(dict(path=name,before=before,after=pins[name]))
    if any(sha(source/name)!=pin for name,pin in pins.items()):raise ValueError('Source drift during scoped refresh')
    canonical=dict(base['canonical_source_hashes']);canonical.update({name:pins[name] for name in names})
    report=dict(base,source_hashes=pins,canonical_source_hashes=canonical,status='prepared',passed=False,
                validation_not_run=True,changes=changes,tests=tests,runtime_cases=families,
                original_literal_owners_preserved=True,
                previous_prepared_sha256=sha(args.prepared),previous_validation_sha256=sha(args.validation),
                installed_kernel_qualification_sha256=sha(args.qualification),
                selector_regeneration_sha256=sha(args.regeneration),tool_sha256=sha(Path(__file__)))
    (args.output/'prepared.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status='prepared',changed_files=len(changes),tests=len(report['tests']))),flush=True)


if __name__=='__main__':main()
