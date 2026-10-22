#!/usr/bin/env python3
"""Freeze imported construction metadata and the admission arithmetic word."""
import json
from freeze_mp_native_sound_storage import ROOT,sha,actual,relative,add,verify,copy

BASE=ROOT/'analysis/multiplayer/integration/object-construction-provider-20261009'
FROZEN=BASE/'frozen'
DEPS={
 'analysis/multiplayer/integration/sound-driver-master-gain-native/frozen/manifest.json':
 '7c7f24932e168fe043ef4600dfb214f9b4671ffd3074d0ce22ee3f6d12d93c2b',
 'analysis/multiplayer/sandbox-source/power-narrowphase-owners/final/manifest.json':
 '17d06f534bd5ed83c4f97b4bae3e3f6c3d3511eb784d407605a8939c0398d9c8',
}

def main():
    destination=FROZEN/'manifest.json'
    if destination.exists():
        report=json.loads(destination.read_text())
        for group in ('artifact_pins','protected_prior_pins','owned_source_pins'):verify(report[group])
        print(json.dumps(dict(manifest_sha256=sha(destination),audit='passed')));return
    prior=dict(DEPS);verify(prior)
    for name in DEPS:
        dependency=json.loads(actual(name).read_text())
        for group in ('artifact_pins','protected_prior_pins','owned_current_files','evidence_files',
                      'original_inputs','prior_pins','files','dependency_pins','new_source_pins','owned_source_pins'):
            for path,pin in dependency.get(group,{}).items():add(prior,path,pin)
    verify(prior)
    for name in ('tables.tsv','report.json'):
        assert (BASE/'measured'/name).read_bytes()==(BASE/'repeated'/name).read_bytes()
    assert sha(BASE/'measured/tables.tsv')=='9525c834e30cdeeb517e1b0727d8384122ffc36ad3dc8e697d1e9ab6089efc9c'
    exported=json.loads((BASE/'measured/report.json').read_text());verify(exported['input_pins'])
    assert not exported['expected_from_native'] and not exported['mutable_source_state_imported']
    for name in ('production-normal','production-sanitizer'):
        report=json.loads((BASE/name/'report.json').read_text())
        assert all(not q['exit_code'] for q in json.loads((BASE/name/'commands.json').read_text()).values())
        assert '100% tests passed out of 3' in (BASE/name/'test.log').read_text()
        for source,pin in report['observed_before_source_pins'].items():
            assert sha(BASE/name/'source-before'/source)==pin
        if name=='production-sanitizer':assert report['actual_sanitizer_flags_checked']
    for folder in ('measured','repeated','production-normal','production-sanitizer'):
        for path in sorted((BASE/folder).rglob('*')):
            if path.is_file():copy(path,FROZEN/'evidence'/folder/path.relative_to(BASE/folder))
    names=['include/halo2/object_construction_tables.hpp','src/gameplay/objects/object_construction_tables.cpp',
        'tests/object_construction_tables_tests.cpp','tools/export_object_construction_tables.py',
        'tools/validate_object_construction_tables_production.py','tools/freeze_mp_object_construction_tables.py',
        'docs/MP_OBJECT_CONSTRUCTION_TABLES_RESEARCH.md']
    owned={name:sha(actual(name)) for name in names}
    for name in names+['CMakeLists.txt','include/halo2/original_data.hpp','src/assets/original_data.cpp']:
        copy(actual(name),FROZEN/'source'/name)
    artifacts={relative(path):sha(path) for path in sorted(FROZEN.rglob('*')) if path.is_file()}
    result=dict(schema='object-construction-provider-frozen-v1',artifact_pins=artifacts,
        protected_prior_pins=prior,owned_source_pins=owned,dependency_manifests=DEPS,
        original_images=2,immutable_construction_bytes=960,extra_admission_bytes=8,
        original_group_query_controls=24,u5_group_queries_use_declared_old_authored_inputs=True,
        base_provider_absent=True,production_normal_and_actual_asan_ubsan_pass=True,
        mutable_object_or_sound_state_imported=False,live_q_or_playback_admitted=False,launcher_changed=False)
    destination.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    verify(artifacts);verify(prior);verify(owned)
    print(json.dumps(dict(manifest_sha256=sha(destination),artifacts=len(artifacts),
                         protected_prior=len(prior),owned=len(owned),mismatches=0)))

if __name__=='__main__':main()
