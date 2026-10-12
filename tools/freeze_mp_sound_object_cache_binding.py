#!/usr/bin/env python3
"""Freeze original sound world owner composed with actual owned Cache metadata."""
import json
from pathlib import Path
from freeze_mp_native_sound_storage import sha, actual, relative, add, verify, copy

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/sound-object-cache-binding-20261009'
FROZEN=BASE/'frozen'
DEPS={
    'analysis/multiplayer/integration/sound-driver-startup-production-20261009/frozen/manifest.json':
        '9b5dcdff918c6e732aa75c22374022f48aea7eec80df4c4234773812306de15c',
    'analysis/multiplayer/integration/sound-object-request-production-20261008/frozen/manifest.json':
        'c7416290946b85c6ca3fab99a844a643450e065dfde0e82d11c28f3b66649c39',
    'analysis/multiplayer/integration/sound-metadata-provider-20261008/frozen/manifest.json':
        '0f32165d470e504c3612af876811053a708fe20bf5e079b49be6aa60aa0f9038',
}


def main():
    manifest_path=FROZEN/'manifest.json'
    if manifest_path.exists():
        manifest=json.loads(manifest_path.read_text())
        for group in ['artifact_pins','protected_prior_pins']:verify(manifest[group])
        print(json.dumps(dict(manifest_sha256=sha(manifest_path),audit='passed')))
        return
    prior=dict(DEPS);verify(prior)
    for name in DEPS:
        manifest=json.loads(actual(name).read_text())
        for group in ['artifact_pins','protected_prior_pins','owned_input_pins']:
            for path,value in manifest.get(group,{}).items():add(prior,path,value)
    verify(prior)
    names=['CMakeLists.txt','include/halo2/cache.hpp','include/halo2/original_data.hpp',
        'src/original_data.cpp','include/halo2/sound_object_request.hpp','src/sound_object_request.cpp',
        'include/halo2/fp_sound_request.hpp','src/fp_sound_request.cpp',
        'include/halo2/sound_metadata_store.hpp','src/sound_metadata_store.cpp',
        'tests/sound_object_request_tests.cpp','tests/sound_object_cache_binding_tests.cpp',
        'tools/freeze_mp_native_sound_storage.py','tools/freeze_mp_sound_object_cache_binding.py',
        'docs/MP_SOUND_OBJECT_CACHE_BINDING_RESEARCH.md']
    for name in names:copy(actual(name),FROZEN/'source'/name)
    for mode,directory in [('normal','build-animation-metadata-store-20261008'),
                           ('sanitizer','build-animation-event-asan-20261008')]:
        log=(BASE/'validation'/mode/'test.log').read_text()
        if '100% tests passed' not in log or '1/1' not in log or 'orderedStores=240 metadataFixtureCallbacks=0' not in log:
            raise ValueError('Actual world-owner Cache composition did not pass')
        build=ROOT/directory
        for path in sorted((BASE/'validation'/mode).rglob('*')):
            if path.is_file():copy(path,FROZEN/'validation'/mode/path.relative_to(BASE/'validation'/mode))
        copy(build/'libhalo2_cache.a',FROZEN/mode/'libhalo2_cache.a')
        test='sound_object_cache_binding_tests';copy(build/test,FROZEN/mode/test)
        for name in ['flags.make','link.txt']:
            copy(build/'CMakeFiles'/f'{test}.dir'/name,FROZEN/mode/'sound_object_cache_binding'/name)
        last=build/'Testing/Temporary/LastTest.log'
        if 'owned_sound_object_cache_binding' in last.read_text():copy(last,FROZEN/mode/'LastTest.log')
    artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
    manifest=dict(schema='sound-object-cache-binding-v1',artifact_pins=artifacts,
        protected_prior_pins=prior,dependency_manifests=DEPS,original_execution_image='1.1',
        supported_operand_images=2,original_cases_per_image=15,accepted_world_owner_requests=24,
        genuine_owner_rejections=6,exact_ordered_stores=240,metadata_fixture_callbacks=0,
        point_query_callbacks=0,world_and_local_owner_inputs='retained original fixture inputs',
        production_gcc_normal_and_asan_ubsan_passed=True,source_voice_admission=False,
        live_reload_playback=False,launcher_changed=False,source_assets_external=True,
        archive_claim='Observed current Cache archive; only the tested sound metadata/world-owner composition claimed.')
    manifest_path.write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
    verify(artifacts);verify(prior)
    print(json.dumps(dict(manifest_sha256=sha(manifest_path),artifacts=len(artifacts),
                         protected_prior=len(prior),mismatches=0)))


if __name__=='__main__':main()
