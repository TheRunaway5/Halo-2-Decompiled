#!/usr/bin/env python3
"""Freeze actual original request/lookup comparisons and production artifacts."""
import json
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha,actual,relative,add,verify,copy

BASE=ROOT/'analysis/multiplayer/integration/object-creation-request-native'
FROZEN=BASE/'frozen'
DEPS={
    'analysis/multiplayer/integration/sound-create-cache-production-20261009/frozen/manifest.json':
    'd173b52f6e78c5ef2b7f83ace948f6e8c2134b94305af1373dcae4367a99de31',
    'analysis/multiplayer/integration/sound-driver-update-native/final-manifest.json':
    '04b6dbc10522848f8511c3adf4e2f001b3c4970c771c1c530ec14a7dbb0a750d',
    'analysis/multiplayer/integration/spawn-source/sound-create-cache-gate/manifest.json':
    '01d5a7e63d300ee01cd336bb362d5d2fa22b327cee6ce8e3bfe05a4582872cb0',
}
REPORT='2e454f51fcc53b20af9e34f1bdb0fad9e8570bd9707dd93694f9b3cf1fd993a4'
PAIR='49f0ffd3722252559a15ac9e23b0b5ee143617a70c8e01b2dd26bbf99747673c'
FIXTURE='63a0d97e641d3572c3c07ecf21122db21a55a54971124f901241a255a6dfbfba'


def files(folder):
    return {str(p.relative_to(folder)):sha(p) for p in folder.rglob('*') if p.is_file()}


def main():
    destination=FROZEN/'manifest.json'
    if destination.exists():
        report=json.loads(destination.read_text())
        verify(report['artifact_pins']);verify(report['protected_prior_pins']);verify(report['owned_source_pins'])
        print(json.dumps(dict(manifest_sha256=sha(destination),audit='passed')));return
    prior=dict(DEPS);verify(prior)
    for name in DEPS:
        dependency=json.loads(actual(name).read_text())
        for group in ('artifact_pins','protected_prior_pins','owned_input_pins','owned_current_files',
            'evidence_files','original_inputs','prior_pins','files','dependency_pins','new_source_pins'):
            for path,pin in dependency.get(group,{}).items():add(prior,path,pin)
    # Freeze all original owning trace tools and inputs already authenticated by
    # the reused admitted original preparation, independently of the audio tree.
    for name in ('analysis/multiplayer/integration/grenade-owned-release/manifest.json',
                 'analysis/multiplayer/integration/grenade-owned-release/checkpoint-manifest.json'):
        dependency=json.loads(actual(name).read_text());add(prior,name,sha(actual(name)))
        for group in ('source_files','verified_prior_pins','frozen_prior_inputs'):
            for path,pin in dependency.get(group,{}).items():add(prior,path,pin)
    verify(prior)
    assert sha(BASE/'measured/report.json')==sha(BASE/'repeat/report.json')==REPORT
    assert files(BASE/'measured')==files(BASE/'repeat')
    assert sha(BASE/'paired/report.json')==sha(BASE/'paired-repeat/report.json')==PAIR
    assert files(BASE/'paired')==files(BASE/'paired-repeat')
    assert sha(BASE/'fixtures/fixtures.tsv')==FIXTURE
    original=json.loads((BASE/'measured/report.json').read_text())
    assert len(original['images'])==2 and sum(len(q['cases']) for q in original['images'])==62
    assert sum(len(c['stores']) for q in original['images'] for c in q['cases'])==902
    assert not original['instruction_patches'] and not original['supplied_source_returns']
    validation=json.loads((BASE/'validation/report.json').read_text())
    for name,pin in validation['source_pins'].items():
        assert sha(actual(name))==sha(BASE/'validation/source'/name)==pin
    assert all(not q['exit_code'] for q in json.loads((BASE/'validation/commands.json').read_text()))
    expected='62 original cases, 902 stores, 1975940 full compared bytes'
    for name in ('production-normal','production-sanitizer'):
        record=json.loads((BASE/name/'report.json').read_text())
        assert all(not q['exit_code'] for q in json.loads((BASE/name/'commands.json').read_text()).values())
        assert expected in (BASE/name/'test.log').read_text()
        for source,pin in record['observed_before_source_pins'].items():
            if source!='CMakeLists.txt':assert sha(actual(source))==pin
        if name=='production-sanitizer':assert record['actual_sanitizer_flags_checked']
    for folder in ('measured','repeat','paired','paired-repeat','fixtures','validation','production-normal','production-sanitizer'):
        for path in sorted((BASE/folder).rglob('*')):
            if path.is_file():copy(path,FROZEN/'evidence'/folder/path.relative_to(BASE/folder))
    source_names=['include/halo2/object_creation_request.hpp','src/object_creation_request.cpp',
        'tests/object_creation_request_tests.cpp','tools/research_object_creation_request_u5.py',
        'tools/oracle_object_creation_request_native.py','tools/export_object_creation_request_fixtures.py',
        'tools/validate_object_creation_request_native.py','tools/validate_object_creation_request_production.py',
        'tools/freeze_mp_object_creation_request_production.py','docs/MP_OBJECT_CREATION_REQUEST_NATIVE_RESEARCH.md']
    owned={name:sha(actual(name)) for name in source_names}
    for name in source_names+['CMakeLists.txt']:copy(actual(name),FROZEN/'source'/name)
    artifacts={relative(path):sha(path) for path in sorted(FROZEN.rglob('*')) if path.is_file()}
    report=dict(schema='object-creation-request-production-v1',artifact_pins=artifacts,protected_prior_pins=prior,
        owned_source_pins=owned,dependency_manifests=DEPS,complete_request_instructions=87,
        complete_typed_lookup_instructions=35,original_cases=62,ordered_stores=902,full_output_bytes=1975940,
        independent_original_repeat=True,copied_source_and_production_normal_asan_ubsan_pass=True,
        supplied_projectile_allocation=False,native_unit_or_q_admitted=False,live_gameplay_fixed=False,
        launcher_changed=False,scope='Complete generic request and salted typed body lookup; literal real owning source backing, no completed projectile/native gameplay claim.')
    destination.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    verify(artifacts);verify(prior);verify(owned)
    print(json.dumps(dict(manifest_sha256=sha(destination),artifacts=len(artifacts),protected_prior=len(prior),owned=len(owned),mismatches=0)))


if __name__=='__main__':
    main()
