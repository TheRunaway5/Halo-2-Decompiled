#!/usr/bin/env python3
"""Freeze real native sound-storage comparisons and prior source evidence."""
import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT/'analysis/multiplayer/integration/native-sound-storage-20261009'
FROZEN = BASE/'frozen'
DEPS = {
    'analysis/multiplayer/integration/sound-map-cache-production-20261009/frozen/manifest.json':
        'dccf99823fdc7ab04c48c9dbe71c885092b7f53c773bc34555c3cacd8ac1ff5d',
    'analysis/multiplayer/integration/sound-map-activation-native/final-manifest.json':
        '8ae77db086ac162da8cde4fa17814e443d8043d460343842774bfd70d084eea0',
    'analysis/multiplayer/integration/spawn-source/unit-channel-request/manifest.json':
        'e845e406fc333b6a73a4e87e044f60c422081ee831bec9181bc56d5b964994fd',
    'analysis/multiplayer/sandbox-source/power-body-disposal/final/manifest.json':
        '77b9e7917a017bbe23f56b55bd233ca179202c93562750d923628a754f2660d8',
}


def sha(path):
    out = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024*1024), b''):
            out.update(chunk)
    return out.hexdigest()


def actual(name):
    path = Path(name)
    return path if path.is_absolute() else ROOT/path


def relative(path):
    return str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)


def add(pins, name, value):
    digest = value if isinstance(value, str) else value['sha256']
    if name in pins and pins[name] != digest:
        raise ValueError('Protected evidence disagrees: '+name)
    pins[name] = digest


def verify(pins):
    for name, digest in pins.items():
        if not actual(name).is_file() or sha(actual(name)) != digest:
            raise ValueError('Protected evidence changed: '+name)


def copy(source, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        if sha(source) != sha(destination):
            raise ValueError('Immutable native snapshot would change: '+str(destination))
    else:
        shutil.copy2(source, destination)


def main():
    manifest_path = FROZEN/'manifest.json'
    if manifest_path.exists():
        manifest = json.loads(manifest_path.read_text())
        for group in ['artifact_pins', 'protected_prior_pins']:
            verify(manifest[group])
        print(json.dumps(dict(manifest_sha256=sha(manifest_path),audit='passed')))
        return
    prior = dict(DEPS)
    verify(prior)
    for name in DEPS:
        manifest = json.loads(actual(name).read_text())
        for group in ['artifact_pins','protected_prior_pins','owned_input_pins',
                      'owned_current_files','evidence_files','original_inputs','prior_pins',
                      'files','new_source_pins','dependency_pins']:
            for path, value in manifest.get(group, {}).items():
                add(prior,path,value)
    verify(prior)

    names = ['CMakeLists.txt','include/halo2/native_sound_storage.hpp','src/audio/native_sound_storage.cpp',
             'tests/native_sound_storage_tests.cpp','include/halo2/sound_cache_startup.hpp',
             'src/audio/sound_cache_startup.cpp','docs/MP_NATIVE_SOUND_STORAGE_RESEARCH.md',
             'tools/freeze_mp_native_sound_storage.py']
    for name in names:
        copy(actual(name),FROZEN/'source'/name)
    for path in sorted((BASE/'validation').rglob('*')):
        if path.is_file(): copy(path,FROZEN/'validation'/path.relative_to(BASE/'validation'))
    for name in ['normal','sanitizer','clang-normal','clang-sanitizer']:
        text = (BASE/'validation'/name/'test.log').read_text()
        if name in ['normal','sanitizer']:
            if '100% tests passed' not in text or 'native_sound_storage' not in text:
                raise ValueError('Production native storage comparison did not pass')
        elif 'originalCases=6 exactOutputBytes=172032 orderedOriginalStores=4090' not in text:
            raise ValueError('Isolated native storage comparison did not pass')
    if 'originalCases=6 exactOutputBytes=172032 orderedOriginalStores=4090' not in (
            BASE/'validation/windows-wine-owned-prefix/test.log').read_text():
        raise ValueError('Actual Windows allocation/protection path did not pass')

    for mode, directory in [('normal','build-animation-metadata-store-20261008'),
                            ('sanitizer','build-animation-event-asan-20261008')]:
        build=ROOT/directory
        copy(build/'native_sound_storage_tests',FROZEN/mode/'native_sound_storage_tests')
        copy(build/'libhalo2_cache.a',FROZEN/mode/'libhalo2_cache.a')
        for name in ['flags.make','link.txt']:
            copy(build/'CMakeFiles/native_sound_storage_tests.dir'/name,FROZEN/mode/name)
        log=build/'Testing/Temporary/LastTest.log'
        if 'native_sound_storage' in log.read_text():
            copy(log,FROZEN/mode/'LastTest.log')

    artifacts={relative(path):sha(path) for path in sorted(FROZEN.rglob('*')) if path.is_file()}
    manifest=dict(schema='native-sound-storage-production-v1',artifact_pins=artifacts,
                  protected_prior_pins=prior,dependency_manifests=DEPS,
                  original_cases=6,exact_constructor_output_bytes=172032,ordered_original_stores=4090,
                  fixture_allocation_or_protection_callbacks=0,
                  production_gcc_normal_and_asan_ubsan_passed=True,
                  isolated_clang_normal_and_asan_ubsan_passed=True,
                  actual_win32_under_wine_passed=True,native_windows_hardware=False,
                  real_host_allocations_and_protection=True,
                  original_arena_allocator_lifecycle_or_fields_generated=False,
                  source_map_device_voice_admission=False,live_launcher_changed=False,
                  scope='Actual native raw OS backing and exact complete cache CPU constructor; clocks, driver, map/plugin/effects and original allocator lifecycle remain separate.',
                  shared_archive_scope='Current Cache archive observed; only tested native storage/cache constructor members claimed.',
                  source_assets_external=True)
    manifest_path.write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
    verify(artifacts)
    verify(prior)
    print(json.dumps(dict(manifest_sha256=sha(manifest_path),artifacts=len(artifacts),
                         protected_prior=len(prior),mismatches=0)))


if __name__ == '__main__':
    main()
