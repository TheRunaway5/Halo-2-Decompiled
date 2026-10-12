#!/usr/bin/env python3
"""Freeze the exact source master-gain owner and immutable import comparison."""
import json
from freeze_mp_native_sound_storage import ROOT,sha,actual,relative,add,verify,copy

BASE=ROOT/'analysis/multiplayer/integration/sound-driver-master-gain-native'
FROZEN=BASE/'frozen'
DEPS={
    'analysis/multiplayer/integration/object-creation-request-native/frozen/manifest.json':
        '8342b62be50e7c0fa32b6cbac283383fadda92cebb150645c9a899bd0b61e1cf',
    'analysis/multiplayer/integration/sound-admission-native/final-manifest.json':
        '5eb1580c42994c40c19d1744484d7d27c0d8fe57d98374165c316a15edac2deb',
    'analysis/multiplayer/integration/spawn-source/object-type-construction/manifest.json':
        '674c8c57e1d94b495ec3d8538730630bc73da6baf5d585cec21528739d27dd9e',
    'analysis/multiplayer/sandbox-source/power-child-agents/final/manifest.json':
        '9440be04c95ba7f35c3f1a0421ca3146209c7425d9379765683d6c880ea7dc01',
}
PAIR='dec64b3042c91f34913b2622f8aa37e4ffad93549cb58affb3f3e31a11433a13'
REPORT='838fb4a200823975dc932338434a061238bbd7095d8b351177e2eea84f2227b3'
FIXTURE='ff4c6b9d6eed212764971ad62da7a13c31221e7676e5eac520c1cf9a40d9184a'

def files(folder):
    return {str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file()}

def main():
    destination=FROZEN/'manifest.json'
    if destination.exists():
        report=json.loads(destination.read_text())
        for group in ('artifact_pins','protected_prior_pins','owned_source_pins'):verify(report[group])
        print(json.dumps(dict(manifest_sha256=sha(destination),audit='passed')));return
    prior=dict(DEPS);verify(prior)
    for name in DEPS:
        dependency=json.loads(actual(name).read_text())
        for group in ('artifact_pins','protected_prior_pins','owned_input_pins','owned_current_files',
            'evidence_files','original_inputs','prior_pins','files','dependency_pins',
            'new_source_pins','owned_source_pins'):
            for path,pin in dependency.get(group,{}).items():add(prior,path,pin)
    verify(prior)
    assert sha(BASE/'measured/report.json')==sha(BASE/'repeated/report.json')==REPORT
    assert files(BASE/'measured')==files(BASE/'repeated')
    assert sha(BASE/'paired/report.json')==sha(BASE/'paired-repeat/report.json')==PAIR
    assert files(BASE/'paired')==files(BASE/'paired-repeat')
    assert sha(BASE/'fixtures/fixtures.tsv')==FIXTURE
    original=json.loads((BASE/'measured/report.json').read_text())
    assert len(original['images'])==2 and sum(len(q['cases']) for q in original['images'])==144
    assert sum(len(c['stores']) for q in original['images'] for c in q['cases'])==2304
    assert not original['instruction_patches'] and not original['supplied_source_returns']
    fixture=json.loads((BASE/'fixtures/report.json').read_text())
    assert not fixture['expected_from_native'] and fixture['full_before_after_bytes']==41468544
    validation=json.loads((BASE/'validation/report.json').read_text())
    for name,pin in validation['source_pins'].items():
        assert sha(actual(name))==sha(BASE/'validation/source'/name)==pin
    assert all(not q['exit_code'] for q in json.loads((BASE/'validation/commands.json').read_text()))
    expected='144 original cases, 2304 stores, 20734272 full compared bytes'
    for name in ('production-normal','production-sanitizer'):
        record=json.loads((BASE/name/'report.json').read_text())
        assert all(not q['exit_code'] for q in json.loads((BASE/name/'commands.json').read_text()).values())
        text=(BASE/name/'test.log').read_text()
        assert expected in text and 'immutableMasterGainBytes=24 baseUnknown=absent' in text
        for source,pin in record['observed_before_source_pins'].items():
            assert sha(BASE/name/'source-before'/source)==pin
        if name=='production-sanitizer':assert record['actual_sanitizer_flags_checked']
    for folder in ('measured','repeated','paired','paired-repeat','fixtures','validation',
                   'production-normal','production-sanitizer'):
        for path in sorted((BASE/folder).rglob('*')):
            if path.is_file():copy(path,FROZEN/'evidence'/folder/path.relative_to(BASE/folder))
    names=['include/halo2/sound_driver_master_gain.hpp','src/sound_driver_master_gain.cpp',
        'tests/sound_driver_master_gain_tests.cpp','tests/sound_driver_master_gain_constants_tests.cpp',
        'tools/research_sound_driver_master_gain_u5.py','tools/oracle_sound_driver_master_gain_native.py',
        'tools/export_sound_driver_master_gain_fixtures.py','tools/validate_sound_driver_master_gain_native.py',
        'tools/validate_sound_driver_master_gain_production.py','tools/freeze_mp_sound_driver_master_gain.py',
        'docs/MP_SOUND_DRIVER_MASTER_GAIN_RESEARCH.md']
    owned={name:sha(actual(name)) for name in names}
    for name in names+['CMakeLists.txt','include/halo2/original_data.hpp','src/original_data.cpp']:
        copy(actual(name),FROZEN/'source'/name)
    artifacts={relative(path):sha(path) for path in sorted(FROZEN.rglob('*')) if path.is_file()}
    report=dict(schema='sound-driver-master-gain-frozen-v1',artifact_pins=artifacts,
        protected_prior_pins=prior,owned_source_pins=owned,dependency_manifests=DEPS,
        complete_gain_instructions=146,complete_conversion_instructions=37,original_cases=144,
        ordered_stores=2304,full_output_bytes=20734272,immutable_import_bytes=24,base_import_absent=True,
        independent_original_repeat=True,copied_source_and_production_normal_asan_ubsan_pass=True,
        positive_static_voice_execution=False,natural_driver_or_looping_reset_completed=False,
        live_gameplay_fixed=False,launcher_changed=False,
        scope='Complete source gain/conversion CPU owner on declared retained owning outputs; no startup, admission, playback or native grenade claim.')
    destination.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    verify(artifacts);verify(prior);verify(owned)
    print(json.dumps(dict(manifest_sha256=sha(destination),artifacts=len(artifacts),
                         protected_prior=len(prior),owned=len(owned),mismatches=0)))

if __name__=='__main__':main()
