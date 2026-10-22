#!/usr/bin/env python3
"""Freeze registered original driver caller and immutable-only operand import."""
import json
from pathlib import Path
from freeze_mp_native_sound_storage import sha, actual, relative, add, verify, copy

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/sound-driver-startup-production-20261009'
FROZEN=BASE/'frozen'
DEPS={
    'analysis/multiplayer/integration/sound-driver-startup-native/final-manifest.json':
        'e75f3b1fabc796cf5ddf7608fbcc7786ce6504f97ff180e93994d322349ab228',
    'analysis/multiplayer/integration/sound-map-cache-binding-20261009/frozen/manifest.json':
        'd8cfeb1650134f4808c7f44d4f0144e0db0c0ffb1a1bce023d218e2c9a23bcaa',
}


def main():
    manifest_path=FROZEN/'manifest.json'
    if manifest_path.exists():
        manifest=json.loads(manifest_path.read_text())
        for group in ['artifact_pins','protected_prior_pins','owned_input_pins']:verify(manifest[group])
        print(json.dumps(dict(manifest_sha256=sha(manifest_path),audit='passed')))
        return
    prior=dict(DEPS);verify(prior)
    for name in DEPS:
        manifest=json.loads(actual(name).read_text())
        for group in ['artifact_pins','protected_prior_pins','owned_input_pins',
                      'owned_current_files','evidence_files','original_inputs','prior_pins']:
            for path,value in manifest.get(group,{}).items():add(prior,path,value)
    verify(prior)
    sources=['CMakeLists.txt','include/halo2/original_data.hpp','src/assets/original_data.cpp',
        'include/halo2/sound_driver_startup.hpp','src/audio/sound_driver_startup.cpp',
        'include/halo2/sound_driver_startup_operands.hpp','tests/sound_driver_startup_tests.cpp',
        'tests/sound_driver_startup_operands_tests.cpp','tools/export_mp_sound_driver_startup_operands.py',
        'tools/export_mp_unit_constructor_constants.py','tools/freeze_mp_native_sound_storage.py',
        'tools/freeze_mp_sound_driver_startup_production.py','docs/MP_SOUND_DRIVER_STARTUP_PRODUCTION_RESEARCH.md']
    for name in sources:copy(actual(name),FROZEN/'source'/name)
    provider=ROOT/'analysis/multiplayer/integration/sound-driver-startup-operands-provider-20261009'
    repeats=[]
    for name in ['operands.tsv','report.json']:
        first,second=provider/'measured'/name,provider/'repeated'/name
        if sha(first)!=sha(second):raise ValueError('Independent driver immutable export differs')
        repeats.append([relative(first),relative(second),sha(first)])
        copy(first,FROZEN/'providers/measured'/name);copy(second,FROZEN/'providers/repeated'/name)
    report=json.loads((provider/'measured/report.json').read_text());inputs={}
    add(inputs,report['authority']['path'],report['authority']['sha256'])
    for item in report['inputs']:add(inputs,item['path'],item['sha256'])
    verify(inputs)
    for mode,directory in [('normal','build-animation-metadata-store-20261008'),
                           ('sanitizer','build-animation-event-asan-20261008')]:
        log=(BASE/'validation'/mode/'test.log').read_text()
        if '100% tests passed' not in log or '2/2' not in log or '21968 exact ordered stores' not in log:
            raise ValueError('Registered original driver/operand tests did not both pass')
        build=ROOT/directory
        for path in sorted((BASE/'validation'/mode).rglob('*')):
            if path.is_file():copy(path,FROZEN/'validation'/mode/path.relative_to(BASE/'validation'/mode))
        copy(build/'libhalo2_cache.a',FROZEN/mode/'libhalo2_cache.a')
        for test in ['sound_driver_startup_tests','sound_driver_startup_operands_tests']:
            copy(build/test,FROZEN/mode/test)
            for name in ['flags.make','link.txt']:
                copy(build/'CMakeFiles'/f'{test}.dir'/name,FROZEN/mode/test.replace('_tests','')/name)
        last=build/'Testing/Temporary/LastTest.log'
        if 'original_sound_driver_startup_operands' in last.read_text():copy(last,FROZEN/mode/'LastTest.log')
    artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
    manifest=dict(schema='sound-driver-startup-production-v1',artifact_pins=artifacts,
        protected_prior_pins=prior,owned_input_pins=inputs,dependency_manifests=DEPS,exact_repeat_pairs=repeats,
        original_prefix_cases=8,original_ordered_writes=21968,complete_arena_bytes_per_case=143360,
        exact_immutable_operand_bytes=56,verified_forwarding_pointer_bytes=16,
        supported_operand_images=2,base_unverified_operands_absent=True,
        mutable_plugin_identity_in_operand_type=False,mutable_environment_or_saved_settings_imported=False,
        production_gcc_normal_and_asan_ubsan_passed=True,original_whole_device_return_supplied=False,
        source_voice_admission=False,live_reload_playback=False,launcher_changed=False,
        source_assets_external=True,archive_claim='Observed Cache archive; only registered driver CPU prefix and immutable operand provider claimed.')
    manifest_path.write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
    verify(artifacts);verify(prior);verify(inputs)
    print(json.dumps(dict(manifest_sha256=sha(manifest_path),artifacts=len(artifacts),
        protected_prior=len(prior),owned_inputs=len(inputs),mismatches=0)))


if __name__=='__main__':main()
