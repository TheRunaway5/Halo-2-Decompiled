#!/usr/bin/env python3
"""Freeze the registered source sound map owner and immutable operand provider."""
import json
from pathlib import Path
from freeze_mp_native_sound_storage import sha, actual, relative, add, verify, copy

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/sound-map-activation-production-20261009'
FROZEN=BASE/'frozen'
DEPS={
    'analysis/multiplayer/integration/native-sound-storage-20261009/frozen/manifest.json':
        '893146d869aed77ba12ff9711c8523b0f9811cf28e2eb6f6cb5c3b618fad7a27',
    'analysis/multiplayer/integration/sound-map-activation-native/final-manifest.json':
        '8ae77db086ac162da8cde4fa17814e443d8043d460343842774bfd70d084eea0',
}


def main():
    manifest_path=FROZEN/'manifest.json'
    if manifest_path.exists():
        manifest=json.loads(manifest_path.read_text())
        for group in ['artifact_pins','protected_prior_pins']: verify(manifest[group])
        print(json.dumps(dict(manifest_sha256=sha(manifest_path),audit='passed')))
        return
    prior=dict(DEPS)
    verify(prior)
    for name in DEPS:
        manifest=json.loads(actual(name).read_text())
        for group in ['artifact_pins','protected_prior_pins','owned_input_pins',
                      'owned_current_files','evidence_files','original_inputs','prior_pins']:
            for path,value in manifest.get(group,{}).items():add(prior,path,value)
    verify(prior)
    for name in ['CMakeLists.txt','include/halo2/original_data.hpp','src/original_data.cpp',
                 'include/halo2/sound_map_activation.hpp','src/sound_map_activation.cpp',
                 'tests/sound_map_activation_tests.cpp','tests/sound_map_activation_constants_tests.cpp',
                 'tools/export_mp_sound_map_activation_constants.py',
                 'tools/export_mp_unit_constructor_constants.py',
                 'tools/freeze_mp_native_sound_storage.py','tools/freeze_mp_sound_map_activation_production.py',
                 'docs/MP_SOUND_MAP_ACTIVATION_PRODUCTION_RESEARCH.md']:
        copy(actual(name),FROZEN/'source'/name)
    provider=ROOT/'analysis/multiplayer/integration/sound-map-activation-constants-provider-20261009'
    repeats=[]
    for name in ['constants.tsv','report.json']:
        first,second=provider/'measured'/name,provider/'repeated'/name
        if sha(first)!=sha(second):raise ValueError('Independent map operand repeat differs')
        repeats.append([relative(first),relative(second),sha(first)])
        copy(first,FROZEN/'providers/measured'/name)
        copy(second,FROZEN/'providers/repeated'/name)
    for mode,directory in [('normal','build-animation-metadata-store-20261008'),
                           ('sanitizer','build-animation-event-asan-20261008')]:
        log=(BASE/'validation'/mode/'test.log').read_text()
        if '100% tests passed' not in log or '2/2' not in log:
            raise ValueError('Registered map owner/operand tests did not both pass')
        build=ROOT/directory
        for path in sorted((BASE/'validation'/mode).rglob('*')):
            if path.is_file():copy(path,FROZEN/'validation'/mode/path.relative_to(BASE/'validation'/mode))
        copy(build/'libhalo2_cache.a',FROZEN/mode/'libhalo2_cache.a')
        for test in ['sound_map_activation_tests','sound_map_activation_constants_tests']:
            copy(build/test,FROZEN/mode/test)
            for name in ['flags.make','link.txt']:
                copy(build/'CMakeFiles'/f'{test}.dir'/name,FROZEN/mode/test.replace('_tests','')/name)
        last=build/'Testing/Temporary/LastTest.log'
        if 'original_sound_map_activation_constants' in last.read_text():
            copy(last,FROZEN/mode/'LastTest.log')
    artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
    manifest=dict(schema='sound-map-activation-production-v1',artifact_pins=artifacts,
                  protected_prior_pins=prior,dependency_manifests=DEPS,exact_repeat_pairs=repeats,
                  source_original_prefix_cases=3,source_ordered_writes=38,
                  source_arena_bytes_per_case=143360,imported_immutable_operand_bytes=16,
                  production_gcc_normal_and_asan_ubsan_passed=True,
                  original_whole_source_platform_return_supplied=False,
                  mutable_environment_saved_setting_defaults=False,
                  source_map_admission=False,source_voice_admission=False,launcher_changed=False,
                  shared_archive_scope='Current Cache archive observed; only tested map CPU owner and immutable operand provider claimed.',
                  source_assets_external=True)
    manifest_path.write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
    verify(artifacts);verify(prior)
    print(json.dumps(dict(manifest_sha256=sha(manifest_path),artifacts=len(artifacts),
                         protected_prior=len(prior),mismatches=0)))


if __name__=='__main__':main()
