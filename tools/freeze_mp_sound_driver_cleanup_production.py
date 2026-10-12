#!/usr/bin/env python3
"""Freeze actual whole cleanup composition against original empty witnesses."""
import json
from pathlib import Path
from freeze_mp_native_sound_storage import sha, actual, relative, add, verify, copy

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT/'analysis/multiplayer/integration/sound-driver-cleanup-production-20261009'
FROZEN = BASE/'frozen'
DEPS = {
    'analysis/multiplayer/integration/sound-driver-owners-production-20261009/frozen/manifest.json':
        'ff12a190ca91168cca46ca8d1cfb0f5e29d7f6f11c7480ccf4c382b77b512d15',
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
        for key in ['artifact_pins', 'protected_prior_pins', 'owned_input_pins']:
            for path, digest in dependency.get(key, {}).items():
                add(prior, path, digest)
    verify(prior)
    names = ['CMakeLists.txt', 'include/halo2/sound_driver_cleanup.hpp',
             'src/sound_driver_cleanup.cpp', 'tests/sound_driver_cleanup_tests.cpp',
             'include/halo2/sound_driver_owners.hpp', 'src/sound_driver_owners.cpp',
             'include/halo2/sound_driver_startup.hpp', 'include/halo2/sound_map_activation.hpp',
             'include/halo2/native_sound_storage.hpp', 'src/native_sound_storage.cpp',
             'include/halo2/sound_cache_startup.hpp', 'tools/run_owned_fixture_test.py',
             'tools/export_mp_sound_driver_cleanup.py',
             'docs/MP_SOUND_DRIVER_CLEANUP_PRODUCTION_RESEARCH.md',
             'tools/freeze_mp_sound_driver_cleanup_production.py',
             'tools/freeze_mp_native_sound_storage.py']
    for name in names:
        copy(actual(name), FROZEN/'source'/name)
    fixture_report = json.loads((BASE/'measured/report.json').read_text())
    if fixture_report['cases'] != 2 or fixture_report['stores'] != 6 or fixture_report['complete_output_bytes'] != 311344:
        raise ValueError('Whole original cleanup witness coverage changed')
    if sha(BASE/'measured/fixtures.tsv') != fixture_report['fixture_sha256']:
        raise ValueError('Whole original cleanup fixture changed')
    for path in sorted((BASE/'measured').rglob('*')):
        if path.is_file():
            copy(path, FROZEN/'measured'/path.relative_to(BASE/'measured'))
    for mode, directory in [('normal', 'build-animation-metadata-store-20261008'),
                            ('sanitizer', 'build-animation-event-asan-20261008')]:
        validation = BASE/'validation'/mode
        commands = json.loads((validation/'commands.json').read_text())
        if any(row['exit_code'] != 0 for row in commands.values()):
            raise ValueError('Cleanup configure/build/test command failed')
        log = (validation/'test.log').read_text()
        for required in ['100% tests passed out of 2',
                         'cleanupCases=2 orderedStores=6 completeOutputBytes=311344',
                         '1138 cases, 514 exact ordered writes']:
            if required not in log:
                raise ValueError('Production cleanup comparison did not pass: '+required)
        for path in sorted(validation.rglob('*')):
            if path.is_file():
                copy(path, FROZEN/'validation'/mode/path.relative_to(validation))
        build = ROOT/directory
        copy(build/'libhalo2_cache.a', FROZEN/mode/'libhalo2_cache.a')
        for test in ['sound_driver_cleanup_tests', 'sound_driver_owners_tests']:
            copy(build/test, FROZEN/mode/test)
            for name in ['flags.make', 'link.txt']:
                copy(build/'CMakeFiles'/f'{test}.dir'/name, FROZEN/mode/f'{test}-build'/name)
    artifacts = {relative(path): sha(path) for path in sorted(FROZEN.rglob('*')) if path.is_file()}
    manifest = dict(schema='sound-driver-cleanup-production-v1', artifact_pins=artifacts,
                    protected_prior_pins=prior, dependency_manifests=DEPS,
                    whole_cleanup_cases=2, ordered_cleanup_stores=6, complete_output_bytes=311344,
                    fixture_helper_callbacks=0, platform_returns=0,
                    normal_and_asan_ubsan_passed=True, nonempty_voice_execution=False,
                    live_audio=False, launcher_changed=False, source_assets_external=True,
                    archive_claim='Observed Cache archives; only named cleanup/driver-owner comparisons claimed.')
    destination.write_text(json.dumps(manifest, sort_keys=True, indent=2)+'\n')
    verify(artifacts)
    verify(prior)
    print(json.dumps(dict(manifest_sha256=sha(destination), artifacts=len(artifacts),
                         protected_prior=len(prior), mismatches=0)))


if __name__ == '__main__':
    main()
