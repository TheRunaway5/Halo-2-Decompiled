#!/usr/bin/env python3
"""Freeze current source and exercise MCC destruction through ordinary weapons.

Run through tools/run_test.py; its owned source/build are always disposable.
CMake compilation/CTest use tools/run_tests.py. Retain only bounded reports and
logs. Constructor/scene inputs are declared; no damage or physics result is set.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from weapon_test_cache import mcc_runtime_inputs, sha
from mcc_content import directory
from mcc_native_map_owner_publication import derive_owner_index, verify_owner_index

ROOT = Path(__file__).resolve().parents[1]
CASES = {'warthog': 'coagulation', 'warthog_gauss': 'coagulation', 'ghost': 'coagulation',
         'banshee': 'coagulation', 'scorpion': 'coagulation', 'wraith': 'coagulation', 'spectre': 'coagulation',
         'power_module': 'lockout', 'fusion_coil': 'foundation', 'gas_container_full': 'colossus',
         'terrain_glass': 'zanzibar'}
CASES.update(coil_to_warthog='headlong', warthog_to_coil='headlong')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mcc-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--case', action='append', choices=tuple(CASES))
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--partition-ai', action='store_true',
                        help='Use the existing unchanged-body partitioner for the large AI translation unit')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    args.output = args.output.resolve();args.output.mkdir(parents=True, exist_ok=True)
    cases = list(dict.fromkeys(args.case or CASES))
    # Admission precedes the expensive source freeze/build. Derivation verifies
    # retained legacy banks too; verification alone accepts an absent old index.
    cache = directory(args.mcc_root).parent
    provenance = json.loads((cache / 'provenance.json').read_bytes())
    native = provenance.get('native_shader_library')
    if not isinstance(native, dict) or not native.get('maps'):
        raise ValueError('Destruction runtime requires a prepared MCC import with native shader bank proofs')
    verify_owner_index(cache, provenance)
    owner_index, _ = derive_owner_index(cache, provenance)
    by_map = {row['map']: row for row in owner_index['maps']}
    owners = {}
    for name in sorted({CASES[case] for case in cases}):
        relative = f'halo2/h2_maps_win64_dx11/{name}.map'
        if relative not in by_map:
            raise ValueError(f'Prepared MCC import lacks verified scenario owner: {relative}')
        owners[relative] = by_map[relative]
    maps, inputs = mcc_runtime_inputs(args.mcc_root)
    inputs['prepared_native_owners'] = owners
    inputs['owner_admission_is_read_only'] = True
    for case in cases:
        path = maps / (CASES[case] + '.map')
        inputs.setdefault('destruction_maps', {})[str(path)] = sha(path)
    report = {'passed': False, 'scratch': str(scratch), 'runtime_inputs': inputs,
              'cases': cases, 'tool_sha256': sha(Path(__file__)), 'large_artifacts_retained': []}
    receipt = args.output / 'cohort.json'
    def save():
        receipt.write_text(json.dumps(report, indent=2) + '\n')
    save()
    prepare = [sys.executable, str(ROOT / 'tools/validate_weapon_material_snapshot_tu15.py'),
               '--mcc-root', str(args.mcc_root.resolve()), '--output', str(args.output / 'source'),
               '--owned-build-worker', '--prepare-only', '--release-only']
    report['prepare_command'] = prepare;save()
    subprocess.run(prepare, check=True)
    source = scratch / 'source'
    report['additional_frozen_inputs'] = {}
    for name in ('mcc-gameplay-scalar-damage-profiles.json', 'mcc-destruction-effect-scalar-profiles.json'):
        path = ROOT / 'tools' / name
        if path.stat().st_size > 1024 * 1024:
            raise ValueError('Reviewed scalar profile manifest exceeds its finite metadata budget')
        shutil.copyfile(path, source / 'tools' / name)
        report['additional_frozen_inputs']['tools/' + name] = sha(source / 'tools' / name)
    save()
    if args.partition_ai:
        path = source / 'cmake/PhysicsLiveWorldTu15.cmake'
        before = sha(path);text = path.read_text()
        needle = 'target_sources(halo2_physics_world_runtime PRIVATE src/campaign/ai/source_ai_frame_tu15_generated.cpp)'
        if text.count(needle) != 1:
            raise ValueError('The original single-unit AI registration changed; requalify partitioning')
        text = text.replace(needle,
            'halo2_partition_native_translation(halo2_physics_world_runtime src/campaign/ai/source_ai_frame_tu15_generated.cpp 12)')
        path.write_text(text)
        report['private_build_partition'] = {'path': str(path.relative_to(source)), 'before': before,
            'after': sha(path), 'original_instruction_bodies_changed': [], 'partitions': 12}
        save()
    text = '''\nadd_executable(destruction_runtime_tu15_tests tests/destruction_runtime_tu15_tests.cpp)
target_link_libraries(destruction_runtime_tu15_tests PRIVATE halo2_systems)
'''
    for case in cases:
        text += (f'add_test(NAME destruction_mcc_{case} COMMAND ${{Python3_EXECUTABLE}} '
                 '"${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" -- '
                 '$<TARGET_FILE:destruction_runtime_tu15_tests> '
                 f'"{maps / (CASES[case] + ".map")}" "{maps}" "{case}" '
                 f'"${{CMAKE_CURRENT_BINARY_DIR}}/destruction-{case}.json")\n'
                 f'set_tests_properties(destruction_mcc_{case} PROPERTIES TIMEOUT 300)\n')
    for name in sorted({CASES[case] for case in cases}):
        text += (f'add_test(NAME destruction_effect_cache_{name} COMMAND '
                 '$<TARGET_FILE:mcc_destruction_effect_scalar_tests> '
                 '"${CMAKE_CURRENT_SOURCE_DIR}/tools/mcc-destruction-effect-scalar-profiles.json" '
                 f'"{maps / (name + ".map")}" "{maps}")\n')
    with (source / 'CMakeLists.txt').open('a') as stream:
        stream.write(text)
    report['registered_cmake_sha256'] = sha(source / 'CMakeLists.txt');save()
    command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--build-worker',
               '--source', str(source), '--target', 'destruction_runtime_tu15_tests',
               '--target', 'source_bsp_breakable_visibility_tu15_tests',
               '--target', 'mcc_destruction_effect_scalar_tests', '--target', 'mcc_gameplay_scalar_tests',
               '--filter', '^(destruction_|source_bsp_breakable_visibility_tu15$|mcc_gameplay_scalar_|mcc_destruction_effect_)',
               '--jobs', str(args.jobs), '--test-jobs', '1',
               '--output', str(args.output / 'validation')]
    for case in cases:
        command += ['--retain-report', f'destruction-{case}.json']
    report['command'] = command;save()
    code = subprocess.run(command, check=False).returncode
    report['returncode'] = code
    report['passed'] = code == 0
    save()
    return code


if __name__ == '__main__':
    raise SystemExit(main())
