#!/usr/bin/env python3
"""Retain actual production sound-payload containment comparisons."""
import json
from pathlib import Path
from freeze_mp_native_sound_storage import sha, actual, relative, add, verify, copy

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT/'analysis/multiplayer/integration/sound-metadata-payload-bounds-20261009'
FROZEN = BASE/'frozen'
DEPS = {
    'analysis/multiplayer/integration/sound-object-cache-binding-20261009/frozen/manifest.json':
        'a29846dc1dfed25d67c52bed5b6e516cd60e431ccfb90b39af0bc40844a7c541',
    'analysis/multiplayer/integration/sound-map-cache-binding-20261009/frozen/manifest.json':
        'd8cfeb1650134f4808c7f44d4f0144e0db0c0ffb1a1bce023d218e2c9a23bcaa',
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
    names = ['CMakeLists.txt', 'include/halo2/cache.hpp',
             'include/halo2/source_tag_metadata.hpp',
             'include/halo2/source_tag_metadata_store.hpp', 'src/assets/source_tag_metadata_store.cpp',
             'include/halo2/sound_metadata_store.hpp', 'src/audio/sound_metadata_store.cpp',
             'tests/sound_metadata_store_tests.cpp', 'tests/sound_object_cache_binding_tests.cpp',
             'tests/sound_object_request_tests.cpp',
             'docs/MP_SOUND_METADATA_PAYLOAD_BOUNDS_RESEARCH.md',
             'tools/freeze_mp_sound_metadata_payload_bounds.py',
             'tools/freeze_mp_native_sound_storage.py']
    for name in names:
        copy(actual(name), FROZEN/'source'/name)
    for mode, directory in [('normal', 'build-animation-metadata-store-20261008'),
                            ('sanitizer', 'build-animation-event-asan-20261008')]:
        validation = BASE/'validation'/mode
        log = (validation/'test.log').read_text()
        for required in ['100% tests passed out of 2',
                         'ownedMaps=23 fullSoundDefinitions=29780 fullClassRecords=1242',
                         'actualCacheWorldOwnerCases=30', 'orderedStores=240']:
            if required not in log:
                raise ValueError('Actual sound metadata comparison did not pass: '+required)
        for path in sorted(validation.rglob('*')):
            if path.is_file():
                copy(path, FROZEN/'validation'/mode/path.relative_to(validation))
        build = ROOT/directory
        copy(build/'libhalo2_cache.a', FROZEN/mode/'libhalo2_cache.a')
        for test in ['sound_metadata_store_tests', 'sound_object_cache_binding_tests']:
            copy(build/test, FROZEN/mode/test)
            for name in ['flags.make', 'link.txt']:
                copy(build/'CMakeFiles'/f'{test}.dir'/name, FROZEN/mode/f'{test}-build'/name)
    artifacts = {relative(path): sha(path) for path in sorted(FROZEN.rglob('*')) if path.is_file()}
    manifest = dict(schema='sound-metadata-payload-bounds-v1', artifact_pins=artifacts,
                    protected_prior_pins=prior, dependency_manifests=DEPS,
                    maps=23, sound_definitions=29780, class_records=1242,
                    object_owner_comparisons=30, ordered_object_stores=240,
                    normal_and_asan_ubsan_passed=True, live_audio=False,
                    launcher_changed=False, source_assets_external=True,
                    archive_claim='Observed Cache archives; only named metadata and object-owner comparisons claimed.')
    destination.write_text(json.dumps(manifest, sort_keys=True, indent=2)+'\n')
    verify(artifacts)
    verify(prior)
    print(json.dumps(dict(manifest_sha256=sha(destination), artifacts=len(artifacts),
                         protected_prior=len(prior), mismatches=0)))


if __name__ == '__main__':
    main()
