#!/usr/bin/env python3
"""Snapshot tested sound-map/cache providers while preserving earlier evidence."""
import hashlib
import json
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/integration/sound-map-cache-production-20261009/frozen'
DEPS = {
    'analysis/multiplayer/integration/sdl-sound-platform-20261008/frozen/manifest.json':
        '576cf1a7ef93cf63c2ac7acf59537ee88acf0c1fd3cbf94fd2de3aa4f27d52fb',
    'analysis/multiplayer/integration/sound-cache-startup-native/final-manifest.json':
        'da6e6f3c7bed6abebe8f9b518e36aeeff866cd11eb76238c5770b6ffb30b5777',
    'analysis/multiplayer/sandbox-source/power-body-grouping/final/manifest.json':
        '8d14894ae02d9486f89b882eb855b69d052fe5b9665055a9b0d95f2d45b05d7d',
    'analysis/multiplayer/integration/spawn-source/unit-local-transition/manifest.json':
        'e458afc99c4bd900630046d99b53479cb14b751ef367ccea4f3bebf82232e3ce',
}


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def actual(name):
    path = Path(name)
    return path if path.is_absolute() else ROOT / path


def label(path):
    return str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)


def add(pins, name, value):
    digest = value if isinstance(value, str) else value['sha256']
    if name in pins and pins[name] != digest:
        raise ValueError('Conflicting preserved evidence: ' + name)
    pins[name] = digest


def verify(pins):
    for name, digest in pins.items():
        if not actual(name).is_file() or sha(actual(name)) != digest:
            raise ValueError('Frozen evidence changed: ' + name)


def copy(source, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        if sha(source) != sha(destination):
            raise ValueError('Immutable snapshot would change: ' + str(destination))
    else:
        shutil.copy2(source, destination)


def main():
    if (BASE / 'manifest.json').exists():
        manifest = json.loads((BASE / 'manifest.json').read_text())
        for group in ['artifact_pins', 'protected_prior_pins', 'owned_input_pins']:
            verify(manifest[group])
        print(json.dumps(dict(manifest_sha256=sha(BASE/'manifest.json'), audit='passed')))
        return

    prior = dict(DEPS)
    verify(DEPS)
    groups = ['artifact_pins', 'protected_prior_pins', 'owned_input_pins',
              'owned_current_files', 'evidence_files', 'original_inputs', 'prior_pins',
              'files', 'new_source_pins', 'dependency_pins']
    for name in DEPS:
        manifest = json.loads(actual(name).read_text())
        for group in groups:
            for path, value in manifest.get(group, {}).items():
                add(prior, path, value)
    verify(prior)

    sources = [
        'CMakeLists.txt', 'include/halo2/original_data.hpp', 'src/original_data.cpp',
        'include/halo2/sound_cache_startup.hpp', 'src/sound_cache_startup.cpp',
        'include/halo2/sound_startup.hpp', 'src/sound_startup.cpp',
        'include/halo2/source_tag_metadata.hpp',
        'include/halo2/source_tag_metadata_store.hpp', 'src/source_tag_metadata_store.cpp',
        'include/halo2/cache.hpp', 'src/cache.cpp',
        'tests/sound_cache_startup_tests.cpp', 'tests/sound_cache_startup_constants_tests.cpp',
        'tests/sound_map_metadata_tests.cpp', 'tools/export_mp_sound_cache_startup_constants.py',
        'tools/export_mp_sound_map_metadata.py', 'tools/export_mp_unit_constructor_constants.py',
        'tools/inspect_multiplayer.py', 'tools/freeze_mp_sound_map_cache_production.py',
        'docs/MP_SOUND_MAP_CACHE_PRODUCTION_RESEARCH.md',
    ]
    for name in sources:
        copy(actual(name), BASE/'source'/name)

    inputs = {}
    repeats = []
    for provider, names in [
        ('sound-cache-startup-constants-provider-20261009', ['constants.tsv', 'report.json']),
        ('sound-map-metadata-provider-20261009', ['fixtures.tsv', 'report.json']),
    ]:
        folder = ROOT/'analysis/multiplayer/integration'/provider
        for name in names:
            first, second = folder/'measured'/name, folder/'repeated'/name
            if sha(first) != sha(second):
                raise ValueError('Independent provider repeat differs: '+provider+'/'+name)
            repeats.append([label(first), label(second), sha(first)])
            copy(first, BASE/'providers'/provider/'measured'/name)
            copy(second, BASE/'providers'/provider/'repeated'/name)
        report = json.loads((folder/'measured/report.json').read_text())
        if 'maps' in report:
            for item in report['maps']:
                add(inputs, item['path'], item['sha256'])
        else:
            for item in report['inputs']:
                add(inputs, item['path'], item['sha256'])
    verify(inputs)

    tests = ['sound_cache_startup_tests', 'sound_cache_startup_constants_tests', 'sound_map_metadata_tests']
    for configuration, directory in [
        ('normal', 'build-animation-metadata-store-20261008'),
        ('sanitizer', 'build-animation-event-asan-20261008'),
    ]:
        build = ROOT/directory
        log = build/'Testing/Temporary/LastTest.log'
        content = log.read_text()
        for test in tests:
            if test not in content:
                raise ValueError('Missing production test observation: '+test)
        if content.count('Test Passed.') != 3 or 'Test Failed.' in content:
            raise ValueError('Production comparison did not pass all three owners')
        copy(log, BASE/configuration/'LastTest.log')
        copy(build/'libhalo2_cache.a', BASE/configuration/'libhalo2_cache.a')
        for test in tests:
            copy(build/test, BASE/configuration/test)
            for name in ['flags.make', 'link.txt']:
                copy(build/'CMakeFiles'/f'{test}.dir'/name, BASE/configuration/test.replace('_tests','')/name)
        symbols = subprocess.check_output(['nm', '-C', str(build/'libhalo2_cache.a')], text=True)
        wanted = '\n'.join(line for line in symbols.splitlines()
                           if any(name in line for name in ['source_sound_cache_startup(',
                                                           'source_sound_startup(',
                                                           'SourceTagMetadataStore::providers()',
                                                           'OriginalData::load_xbe('])) + '\n'
        (BASE/configuration/'observed-library-symbols.txt').write_text(wanted)

    artifacts = {label(path):sha(path) for path in sorted(BASE.rglob('*')) if path.is_file()}
    manifest = dict(schema='sound-map-cache-production-provider-v1', artifact_pins=artifacts,
                    protected_prior_pins=prior, owned_input_pins=inputs, dependency_manifests=DEPS,
                    exact_repeat_pairs=repeats, exact_imported_callback_bytes=16,
                    original_constructor_instruction_bytes=20, owned_maps=23,
                    complete_opaque_metadata_payloads=69, exact_metadata_bytes=5244048,
                    source_cache_constructor_stores=4090,
                    registered_normal_and_asan_ubsan_tests_passed=3,
                    composition='Actual CPU startup plus cache constructor, complete original output backing at genuine platform request',
                    native_source_ready_or_io_success_substitutes=[], launcher_changed=False,
                    live_map_activation=False, live_reload_sound_consumer=False,
                    source_admitted_voice=False, physical_hardware_audibility=False,
                    archive_scope='Fresh current Cache archive observed; only the three named providers/cache-startup composition tested. No full runtime or uncalled-member review.',
                    source_assets_external=True)
    (BASE/'manifest.json').write_text(json.dumps(manifest, sort_keys=True, indent=2)+'\n')
    verify(artifacts)
    verify(prior)
    print(json.dumps(dict(manifest_sha256=sha(BASE/'manifest.json'), artifacts=len(artifacts),
                         protected_prior=len(prior), owned_inputs=len(inputs), mismatches=0)))


if __name__ == '__main__':
    main()
