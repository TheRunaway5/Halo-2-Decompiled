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
CASES.update(coil_to_scenery='zanzibar', coil_to_crate='foundation')
CASES.update({case: 'coagulation' for case in (
    'warthog_chaingun', 'warthog_gauss_turret', 'scorpion_cannon_mp',
    'wraith_mortar_mp', 'wraith_minigun', 'spectre_plasma_turret')})
CASES['h_turret_ap'] = 'foundation'
CASES['campaign_c_turret_ap'] = '05a_deltaapproach'
CASES['campaign_h_turret_mp'] = '03a_oldmombasa'
CASES['campaign_plasma_cannon'] = '01b_spacestation'
CASES['campaign_scorpion_cannon'] = '05a_deltaapproach'
CASES['campaign_flood_clump'] = '04b_floodlab'
CASES['campaign_sentinel_emitter'] = '06a_sentinelwalls'
CASES['wraith_mortar'] = 'coagulation'
CASES['campaign_guntower_no_damage'] = '03a_oldmombasa'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mcc-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--case', action='append', choices=tuple(CASES))
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--prepare-only', action='store_true',
                        help='Freeze and register an owned cohort without claiming validation')
    parser.add_argument('--build-runtime', action='store_true',
                        help='Also build the native game and normal SDL/LAN acceptance executable')
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
    prepare += ['--copy-drift-retries', '2']
    report['prepare_command'] = prepare;save()
    subprocess.run(prepare, check=True)
    source = scratch / 'source'
    report['additional_frozen_inputs'] = {}
    def private_input(path):
        relative = path.relative_to(ROOT)
        destination = source / relative
        before = sha(path)
        if destination.is_symlink():
            destination.unlink()
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, destination)
        if sha(path) != before or sha(destination) != before:
            raise ValueError(f'Input changed while freezing {relative}')
        report['additional_frozen_inputs'][str(relative)] = before

    for name in ('mcc-gameplay-scalar-damage-profiles.json', 'mcc-destruction-effect-scalar-profiles.json',
                 'mcc-destruction-model-scalar-profile.json'):
        path = ROOT / 'tools' / name
        if path.stat().st_size > 1024 * 1024:
            raise ValueError('Reviewed scalar profile manifest exceeds its finite metadata budget')
        private_input(path)
    # These are small source/configuration inputs, not borrowed runtime assets.
    # Native variant bundling requires source-owned paths; helper headers must
    # not change underneath an in-progress C++ snapshot.
    for relative in ('data', 'tests/helpers'):
        destination = source / relative
        # Newer snapshot preparation already freezes helpers privately. Keep
        # that coherent input set instead of borrowing a later checkout edit.
        if destination.is_dir() and not destination.is_symlink():
            paths = sorted(path for path in destination.rglob('*') if path.is_file())
            if any(path.is_symlink() for path in destination.rglob('*')) or sum(
                    path.stat().st_size for path in paths) > 4 * 1024 * 1024:
                raise ValueError(f'Invalid already-frozen configuration/helper inputs: {relative}')
            for path in paths:
                report['additional_frozen_inputs'][str(path.relative_to(source))] = sha(path)
            continue
        if not destination.is_symlink() or destination.resolve() != ROOT / relative:
            raise ValueError(f'Unexpected borrowed input before private copy: {relative}')
        paths = sorted(path for path in (ROOT / relative).rglob('*') if path.is_file())
        if any(path.is_symlink() for path in paths) or sum(path.stat().st_size for path in paths) > 4 * 1024 * 1024:
            raise ValueError(f'Private configuration/helper inputs exceed their declared budget: {relative}')
        destination.unlink()
        destination.mkdir()
        for path in paths:
            private_input(path)
    private_input(ROOT / 'tests/fixtures/death_cycle_helpers_tu15.json')
    save()
    if args.partition_ai:
        path = source / 'cmake/PhysicsLiveWorldTu15.cmake'
        before = sha(path);text = path.read_text()
        needle = 'target_sources(halo2_physics_world_runtime PRIVATE src/campaign/ai/source_ai_frame_tu15_generated.cpp)'
        partition = 'halo2_partition_native_translation(halo2_physics_world_runtime src/campaign/ai/source_ai_frame_tu15_generated.cpp 12)'
        already_partitioned = text.count(needle) == 0 and text.count(partition) == 1
        if not already_partitioned and (text.count(needle) != 1 or partition in text):
            raise ValueError('The original single-unit AI registration changed; requalify partitioning')
        if not already_partitioned:
            path.write_text(text.replace(needle, partition))
        report['private_build_partition'] = {'path': str(path.relative_to(source)), 'before': before,
            'after': sha(path), 'original_instruction_bodies_changed': [], 'partitions': 12,
            'already_registered_by_source': already_partitioned}
        save()
    text = '''\nadd_executable(destruction_runtime_tu15_tests tests/destruction_runtime_tu15_tests.cpp)
target_link_libraries(destruction_runtime_tu15_tests PRIVATE halo2_systems halo2_authority_codec OpenSSL::Crypto)
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
        if name in ('foundation','coagulation'):
            text += (f'add_test(NAME destruction_model_cache_{name} COMMAND mcc_destruction_model_cache_tests '
                     '"${CMAKE_CURRENT_SOURCE_DIR}/tools/mcc-destruction-model-scalar-profile.json" '
                     f'"{maps / (name + ".map")}" "{maps}")\n')
    with (source / 'CMakeLists.txt').open('a') as stream:
        stream.write(text)
    report['registered_cmake_sha256'] = sha(source / 'CMakeLists.txt');save()
    command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--build-worker',
               '--source', str(source), '--target', 'destruction_runtime_tu15_tests',
               '--target', 'destruction_lan_identity_tu15_tests',
               '--target', 'source_bsp_breakable_visibility_tu15_tests',
               '--target', 'death_cycle_helpers_tu15_tests',
               '--target', 'mcc_destruction_model_scalar_tests', '--target', 'mcc_destruction_model_cache_tests',
               '--target', 'mcc_destruction_effect_scalar_tests', '--target', 'mcc_gameplay_scalar_tests',
               '--filter', '^(destruction_|original_death_cycle_helpers_tu15$|source_bsp_breakable_visibility_tu15$|mcc_gameplay_scalar_|mcc_destruction_(effect_|model_))',
               '--jobs', str(args.jobs), '--test-jobs', '1',
               '--cmake-arg=-DHALO2_DESTRUCTION_MCC_MAPS=' + str(maps),
               '--output', str(args.output / 'validation')]
    if args.build_runtime:
        command += ['--target', 'halo2_native', '--target', 'physics_multiplayer_acceptance']
    command += ['--retain-report', 'destruction-lan-identity.json']
    for case in cases:
        command += ['--retain-report', f'destruction-{case}.json']
    report['command'] = command;save()
    if args.prepare_only:
        report.update(status='prepared', validation_not_run=True, passed=False)
        save()
        print(json.dumps({'status': 'prepared', 'passed': False, 'report': str(receipt)}), flush=True)
        return 0
    # The runner already retains bounded build/test logs. Large generated-code
    # warnings must not fill an interactive coordinator's terminal transcript.
    print(json.dumps({'status': 'validating', 'report': str(args.output / 'validation/report.json')}), flush=True)
    code = subprocess.run(command, check=False, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT).returncode
    report['returncode'] = code
    report['passed'] = code == 0
    save()
    print(json.dumps({'passed': report['passed'], 'returncode': code, 'report': str(receipt)}), flush=True)
    return code


if __name__ == '__main__':
    raise SystemExit(main())
