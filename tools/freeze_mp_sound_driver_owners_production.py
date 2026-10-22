#!/usr/bin/env python3
"""Freeze production registration of complete original sound-driver helpers."""
import json
from pathlib import Path
from freeze_mp_native_sound_storage import sha, actual, relative, add, verify, copy

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT/'analysis/multiplayer/integration/sound-driver-owners-production-20261009'
FROZEN = BASE/'frozen'
DEPS = {
    'analysis/multiplayer/integration/sound-driver-owners-native/final-manifest.json':
        '0c6ed26b546fb0ceee6ec17900af768dee80e9ab2dace026d25f42f68487b04f',
    'analysis/multiplayer/integration/sound-metadata-payload-bounds-20261009/frozen/manifest.json':
        '39091b775d7e8090945a1a16e8acef9ccd0c022bfddf1a2fa4bb65ca094d2c93',
}


def main():
    destination = FROZEN/'manifest.json'
    if destination.exists():
        manifest = json.loads(destination.read_text())
        for key in ['artifact_pins', 'protected_prior_pins']:
            verify(manifest[key])
        print(json.dumps(dict(manifest_sha256=sha(destination), audit='passed')))
        return
    prior = dict(DEPS)
    verify(prior)
    for name in DEPS:
        dependency = json.loads(actual(name).read_text())
        for key in ['artifact_pins', 'protected_prior_pins', 'owned_input_pins',
                    'owned_current_files', 'evidence_files', 'original_inputs', 'prior_pins']:
            for path, digest in dependency.get(key, {}).items():
                add(prior, path, digest)
    verify(prior)
    names = ['CMakeLists.txt', 'include/halo2/sound_driver_owners.hpp',
             'src/audio/sound_driver_owners.cpp', 'tests/sound_driver_owners_tests.cpp',
             'include/halo2/sound_driver_startup.hpp', 'src/audio/sound_driver_startup.cpp',
             'tests/sound_driver_startup_tests.cpp', 'include/halo2/native_sound_storage.hpp',
             'src/audio/native_sound_storage.cpp', 'include/halo2/sound_cache_startup.hpp',
             'include/halo2/sound_map_activation.hpp', 'tools/run_owned_fixture_test.py',
             'docs/MP_SOUND_DRIVER_OWNERS_PRODUCTION_RESEARCH.md',
             'tools/freeze_mp_sound_driver_owners_production.py',
             'tools/freeze_mp_native_sound_storage.py']
    for name in names:
        copy(actual(name), FROZEN/'source'/name)
    controls = json.loads((BASE/'validation/owned-fixture-runner-controls.json').read_text())
    if [row['exit_code'] for row in controls] != [77, 1, 0]:
        raise ValueError('Missing-data runner did not preserve real failures')
    copy(BASE/'validation/owned-fixture-runner-controls.json', FROZEN/'validation/owned-fixture-runner-controls.json')
    for mode, directory in [('normal', 'build-animation-metadata-store-20261008'),
                            ('sanitizer', 'build-animation-event-asan-20261008')]:
        validation = BASE/'validation'/mode
        log = (validation/'test.log').read_text()
        for required in ['100% tests passed out of 2',
                         '1138 cases, 514 exact ordered writes',
                         '8 cases, 21968 exact ordered stores']:
            if required not in log:
                raise ValueError('Production driver-owner comparison did not pass: '+required)
        for path in sorted(validation.rglob('*')):
            if path.is_file():
                copy(path, FROZEN/'validation'/mode/path.relative_to(validation))
        build = ROOT/directory
        copy(build/'libhalo2_cache.a', FROZEN/mode/'libhalo2_cache.a')
        for test in ['sound_driver_owners_tests', 'sound_driver_startup_tests']:
            copy(build/test, FROZEN/mode/test)
            for name in ['flags.make', 'link.txt']:
                copy(build/'CMakeFiles'/f'{test}.dir'/name, FROZEN/mode/f'{test}-build'/name)
    artifacts = {relative(path): sha(path) for path in sorted(FROZEN.rglob('*')) if path.is_file()}
    manifest = dict(schema='sound-driver-owners-production-v1', artifact_pins=artifacts,
                    protected_prior_pins=prior, dependency_manifests=DEPS,
                    owner_cases=1138, ordered_owner_stores=514,
                    startup_cases=8, ordered_startup_stores=21968,
                    normal_and_asan_ubsan_passed=True, source_device_or_voice=False,
                    live_audio=False, launcher_changed=False, source_assets_external=True,
                    archive_claim='Observed Cache archives; only the named driver-owner and startup-prefix comparisons claimed.')
    destination.write_text(json.dumps(manifest, sort_keys=True, indent=2)+'\n')
    verify(artifacts)
    verify(prior)
    print(json.dumps(dict(manifest_sha256=sha(destination), artifacts=len(artifacts),
                         protected_prior=len(prior), mismatches=0)))


if __name__ == '__main__':
    main()
