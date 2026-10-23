#!/usr/bin/env python3
"""Refresh named turret owners in an active, authenticated disposable cohort.

Prepare with validate_weapon_material_snapshot_tu15.py --vehicle-weapon-fire
--owned-build-worker --prepare-only --release-only inside a run_test.py owner.
Only the declared files change. Builds and CTests use tools/run_tests.py.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from weapon_test_cache import sha, require_mcc_map

ROOT = Path(__file__).resolve().parents[1]
CASES = {'human-fixed': ('h_turret_ap', False), 'covenant-fixed': ('c_turret_ap', False),
         'warthog-mg': ('warthog-mg', False), 'warthog-gauss': ('warthog-gauss', False),
         'scorpion-cannon': ('scorpion', False), 'scorpion-mg': ('scorpion', True)}
FILES = ('src/assets/mcc/mcc_gameplay_scalar.cpp',
         'src/audio/multiplayer_audio.cpp',
         'src/input/multiplayer_kbm.cpp', 'include/halo2/multiplayer_kbm.hpp',
         'tests/multiplayer_kbm_tests.cpp',
         'cmake/SourceContrailGraphics.cmake',
         'tests/mcc_destruction_effect_scalar_tests.cpp',
         'tests/mcc_gameplay_reference_tests.cpp',
         'tests/vehicle_weapon_hud_world_tests.cpp',
         'tools/mcc-turret-effect-scalar-profiles.json', 'tools/run_turret_audio_tu15.py')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--prepared', type=Path, required=True)
    ap.add_argument('--partitions', type=Path)
    ap.add_argument('--previous-refresh', type=Path)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--case', choices=tuple(CASES), action='append')
    ap.add_argument('--jobs', type=int, default=5)
    ap.add_argument('--fire-only', action='store_true')
    ap.add_argument('--audio', action='store_true')
    args = ap.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        ap.error('Use the same active run_test.py owner as the prepared cohort')
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    source = owner / 'source'
    prior = json.loads(args.prepared.read_text())
    if Path(prior['runner_scratch']).resolve() != owner:
        raise ValueError('Prepared cohort belongs to another source/build owner')
    expected = dict(prior['registered_snapshot_source_files_sha256'])
    if args.partitions:
        for row in json.loads(args.partitions.read_text()):
            if expected[row['path']] != row['before']:
                raise ValueError('Partition does not match the prepared cohort')
            expected[row['path']] = row['after']
    refreshed = {}
    if args.previous_refresh:
        previous = json.loads(args.previous_refresh.read_text())
        if previous['scratch'] != str(owner):
            raise ValueError('Previous refresh belongs to another owner')
        expected.update(previous['refreshed_source_sha256'])
        refreshed.update(previous['refreshed_source_sha256'])
    drift = [name for name, digest in expected.items() if sha(source / name) != digest]
    if drift:
        raise ValueError('Frozen source changed outside declared refresh: ' + ', '.join(drift[:10]))
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    changes = []
    for name in FILES:
        target = source / name
        before = sha(target) if target.exists() else None
        current = sha(ROOT / name)
        shutil.copyfile(ROOT / name, target)
        refreshed[name] = sha(target)
        if refreshed[name] != current or sha(ROOT / name) != current:
            raise ValueError('Workspace source changed during named refresh: ' + name)
        changes.append({'path': name, 'before': before, 'after': refreshed[name]})
    maps = Path(prior['runtime_cache_inputs']['maps_directory'])
    map_ = require_mcc_map(maps / 'coagulation.map')
    cases = list(dict.fromkeys(args.case or CASES))
    cmake = source / 'CMakeLists.txt'
    text = cmake.read_text()
    # Replacements are contained in this owned registration block only.
    marker = '\n# TURRET_COHORT_TU15\n'
    if marker in text:
        text = text.split(marker)[0]
    text += marker
    text += '''add_test(NAME turret_effect_profiles COMMAND mcc_destruction_effect_scalar_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/mcc-turret-effect-scalar-profiles.json")
'''
    text += ('add_test(NAME turret_effect_cache COMMAND mcc_destruction_effect_scalar_tests '
             '"${CMAKE_CURRENT_SOURCE_DIR}/tools/mcc-turret-effect-scalar-profiles.json" '
             f'"{map_}" "{maps}")\n')
    for case in cases:
        selected, secondary = CASES[case]
        if args.audio:
            text += (f'add_test(NAME turret_mcc_{case} COMMAND "${{Python3_EXECUTABLE}}" '
                '"${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" -- "${Python3_EXECUTABLE}" '
                '"${CMAKE_CURRENT_SOURCE_DIR}/tools/run_turret_audio_tu15.py" '
                '$<TARGET_FILE:vehicle_weapon_hud_runtime_tests> '
                f'"{map_}" "{selected}" "${{CMAKE_CURRENT_BINARY_DIR}}/turret-{case}.json" '
                + ('secondary' if secondary else '') + ')\n'
                f'set_tests_properties(turret_mcc_{case} PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE '
                f'ENVIRONMENT "HALO2_TURRET_GRAPHICS_CACHE={owner / "turret-graphics-cache"}")\n')
            continue
        text += (f'add_test(NAME turret_mcc_{case} COMMAND "${{Python3_EXECUTABLE}}" '
                 '"${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" -- '
                 '$<TARGET_FILE:vehicle_weapon_hud_world_tests> '
                 f'"{map_}" "{maps}" native-mcc)\n'
                 f'set_tests_properties(turret_mcc_{case} PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE '
                 f'ENVIRONMENT "SDL_AUDIODRIVER=dummy;HALO2_TEST_VEHICLE_WEAPON_HUD_CASE={selected};'
                 'HALO2_TEST_VEHICLE_WEAPON_FIRE=1;HALO2_TEST_TURRET_SOUND_TRACE=1;'
                 + ('HALO2_TEST_TURRET_TARGET_DAMAGE=1;' if not args.fire_only else '')
                 + ('HALO2_TEST_VEHICLE_WEAPON_SECONDARY=1;' if secondary else '')
                 + f'HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/turret-{case}.json")\n')
    cmake.write_text(text)
    refreshed['CMakeLists.txt'] = sha(cmake)
    command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--build-worker',
               '--source', str(source), '--target',
               'vehicle_weapon_hud_runtime_tests' if args.audio else 'vehicle_weapon_hud_world_tests',
               '--target', 'mcc_destruction_effect_scalar_tests',
               '--target', 'multiplayer_kbm_tests',
               '--filter', '^(turret_mcc_|turret_effect_|multiplayer_kbm_bindings$)',
               '--jobs', str(args.jobs), '--test-jobs', '1', '--output', str(args.output / 'validation')]
    for case in cases:
        command += ['--retain-report', f'turret-{case}.json']
    report = {'passed': False, 'scratch': str(owner), 'changes': changes,
              'tool_sha256': sha(Path(__file__)),
              'refreshed_source_sha256': refreshed, 'cases': cases, 'command': command,
              'fire_only': args.fire_only, 'audio': args.audio, 'mcc_map_sha256': sha(map_),
              'full_tu15_parity': False, 'large_artifacts_retained': []}
    receipt = args.output / 'refresh.json'
    receipt.write_text(json.dumps(report, indent=2) + '\n')
    with (args.output / 'command.log').open('w') as log:
        code = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=False).returncode
    report.update(passed=code == 0, returncode=code,
                  frozen_source_drift_after_validation=[name for name, digest in (expected | refreshed).items()
                      if sha(source / name) != digest])
    report['passed'] &= not report['frozen_source_drift_after_validation']
    receipt.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'passed': report['passed'], 'report': str(receipt)}), flush=True)
    return code or int(not report['passed'])


if __name__ == '__main__':
    raise SystemExit(main())
