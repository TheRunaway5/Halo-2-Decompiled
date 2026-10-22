#!/usr/bin/env python3
"""Freeze actual owned-cache sound map composition and complete mix payloads."""
import json
from pathlib import Path
from freeze_mp_native_sound_storage import sha, actual, relative, add, verify, copy

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/sound-map-cache-binding-20261009'
FROZEN=BASE/'frozen'
DEPS={
    'analysis/multiplayer/integration/sound-map-activation-production-20261009/frozen/manifest.json':
        '61a73a55ad95ca9bd519d3c6f8ea2e084049739139719bc059f8316c25097a88',
    'analysis/multiplayer/integration/sound-map-cache-production-20261009/frozen/manifest.json':
        'dccf99823fdc7ab04c48c9dbe71c885092b7f53c773bc34555c3cacd8ac1ff5d',
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
    sources=[
        'CMakeLists.txt','include/halo2/cache.hpp',
        'include/halo2/source_tag_metadata.hpp','include/halo2/source_tag_metadata_store.hpp',
        'src/assets/source_tag_metadata_store.cpp','include/halo2/native_sound_storage.hpp',
        'src/audio/native_sound_storage.cpp','include/halo2/sound_map_activation.hpp',
        'src/audio/sound_map_activation.cpp','include/halo2/original_data.hpp','src/assets/original_data.cpp',
        'tests/sound_mix_metadata_tests.cpp','tests/sound_map_cache_binding_tests.cpp',
        'tests/sound_map_metadata_tests.cpp','tools/export_mp_sound_mix_metadata.py',
        'tools/inspect_multiplayer.py','tools/freeze_mp_native_sound_storage.py',
        'tools/freeze_mp_sound_map_cache_binding.py','docs/MP_SOUND_MAP_CACHE_BINDING_RESEARCH.md',
    ]
    for name in sources:copy(actual(name),FROZEN/'source'/name)
    provider=ROOT/'analysis/multiplayer/integration/sound-mix-metadata-provider-20261009'
    repeats=[]
    inputs={}
    for name in ['fixtures.tsv','report.json']:
        first,second=provider/'measured'/name,provider/'repeated'/name
        if sha(first)!=sha(second):raise ValueError('Independent complete snmx repeat differs')
        repeats.append([relative(first),relative(second),sha(first)])
        copy(first,FROZEN/'providers/measured'/name)
        copy(second,FROZEN/'providers/repeated'/name)
    report=json.loads((provider/'measured/report.json').read_text())
    add(inputs,report['inventory']['path'],report['inventory']['sha256'])
    for item in report['maps']:add(inputs,item['path'],item['sha256'])
    verify(inputs)
    tests=['sound_mix_metadata_tests','sound_map_cache_binding_tests','sound_map_metadata_tests']
    for mode,directory in [('normal','build-animation-metadata-store-20261008'),
                           ('sanitizer','build-animation-event-asan-20261008')]:
        log=(BASE/'validation'/mode/'test.log').read_text()
        if '100% tests passed' not in log or '3/3' not in log or 'exactArenaBytes=143360 orderedStores=19' not in log:
            raise ValueError('Actual cache binding checks did not all pass')
        build=ROOT/directory
        for path in sorted((BASE/'validation'/mode).rglob('*')):
            if path.is_file():copy(path,FROZEN/'validation'/mode/path.relative_to(BASE/'validation'/mode))
        copy(build/'libhalo2_cache.a',FROZEN/mode/'libhalo2_cache.a')
        for test in tests:
            copy(build/test,FROZEN/mode/test)
            for name in ['flags.make','link.txt']:
                copy(build/'CMakeFiles'/f'{test}.dir'/name,FROZEN/mode/test.replace('_tests','')/name)
        last=build/'Testing/Temporary/LastTest.log'
        if 'owned_sound_map_cache_binding' in last.read_text():copy(last,FROZEN/mode/'LastTest.log')
    artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
    manifest=dict(schema='sound-map-cache-binding-v1',artifact_pins=artifacts,owned_input_pins=inputs,
        protected_prior_pins=prior,dependency_manifests=DEPS,exact_repeat_pairs=repeats,
        complete_snmx_payloads=23,exact_snmx_bytes=2024,owned_maps=23,
        prior_matg_sncl_ugh_payloads=69,prior_exact_payload_bytes=5244048,
        original_execution_image='1.1',supported_operand_images=2,
        retained_original_startup_arena_bytes=143360,ordered_stores_per_image=19,
        metadata_and_tag_lookup_fixture_callbacks=0,source_clock='explicit retained measured input',
        source_driver_stop=dict(address='00002a0c',extent=4),
        production_normal_and_sanitizer_passed=True,
        source_map_admission=False,live_reload_playback=False,launcher_changed=False,
        source_assets_external=True,
        archive_claim='Observed current Cache archive; only tested providers and CPU prefix composition claimed.')
    manifest_path.write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
    verify(artifacts);verify(prior);verify(inputs)
    print(json.dumps(dict(manifest_sha256=sha(manifest_path),artifacts=len(artifacts),
                         protected_prior=len(prior),owned_inputs=len(inputs),mismatches=0)))


if __name__=='__main__':main()
