#!/usr/bin/env python3
"""Freeze immutable owned-image looping/construction adapter evidence."""
import json
from freeze_mp_native_sound_storage import ROOT,sha,actual,relative,add,verify,copy

BASE=ROOT/'analysis/multiplayer/integration/looping-construction-operands-20261009'
FROZEN=BASE/'frozen'
DEPS={
 'analysis/multiplayer/integration/object-model-resources-native/frozen/manifest.json':'9f9f14a739a49a5afa016ef8a8b017831371d5d530fe06eb2604044b47e0805a',
 'analysis/multiplayer/integration/spawn-source/generic-object-constructor/manifest.json':'eca49b6594615fff1fc2a96e206dd5371a548c04fbc0bb100a2e9f95fc16e0aa',
 'analysis/multiplayer/integration/sound-looping-native/final-manifest.json':'e37d9a60054c11067a80c526d6a4ac2b3dd76abb4a00b1b8131898f153e8b1ef',
 'analysis/multiplayer/sandbox-source/power-gjk-owners/final/manifest.json':'243b14c36bd310200849eaf744337307a9ab9c2f469493f00f49b79582dc9024',
}

def main():
    target=FROZEN/'manifest.json'
    if target.exists():
        report=json.loads(target.read_text())
        for group in('artifact_pins','protected_prior_pins','owned_source_pins'):verify(report[group])
        print(json.dumps(dict(manifest_sha256=sha(target),audit='passed')));return
    prior=dict(DEPS);verify(prior)
    for name in DEPS:
        dependency=json.loads(actual(name).read_text())
        for group in('artifact_pins','protected_prior_pins','owned_current_files','evidence_files','original_inputs',
            'prior_pins','files','dependency_pins','new_source_pins','owned_source_pins'):
            for path,pin in dependency.get(group,{}).items():add(prior,path,pin)
    verify(prior)
    for name in('operands.tsv','report.json'):assert (BASE/'measured'/name).read_bytes()==(BASE/'repeated'/name).read_bytes()
    exported=json.loads((BASE/'measured/report.json').read_text());verify(exported['input_pins'])
    assert exported['fixture_sha256']=='739d38f0789190f104bb48d6cf8687f0dba2612802bca72235ce6e5731dcd68a'
    assert not exported['expected_from_native']and not exported['mutable_runtime_values_imported']
    for folder in('production-normal','production-sanitizer'):
        report=json.loads((BASE/folder/'report.json').read_text())
        assert all(not c['exit_code']for c in json.loads((BASE/folder/'commands.json').read_text()).values())
        assert '100% tests passed out of 1'in(BASE/folder/'test.log').read_text()
        for source,pin in report['observed_before_source_pins'].items():assert sha(BASE/folder/'source-before'/source)==pin
        if folder=='production-sanitizer':assert report['actual_sanitizer_flags_checked']
    for folder in('measured','repeated','production-normal','production-sanitizer'):
        for path in sorted((BASE/folder).rglob('*')):
            if path.is_file():copy(path,FROZEN/'evidence'/folder/path.relative_to(BASE/folder))
    names=['tests/looping_construction_operands_tests.cpp','tools/export_looping_construction_operands.py',
        'tools/validate_looping_construction_operands.py','tools/freeze_looping_construction_operands.py',
        'docs/MP_LOOPING_CONSTRUCTION_OPERANDS_RESEARCH.md']
    owned={name:sha(actual(name))for name in names}
    for name in names+['CMakeLists.txt','include/halo2/original_data.hpp','src/original_data.cpp']:copy(actual(name),FROZEN/'source'/name)
    artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
    result=dict(schema='looping-construction-operands-frozen-v1',artifact_pins=artifacts,protected_prior_pins=prior,
        owned_source_pins=owned,dependency_manifests=DEPS,original_string_fields=114,original_string_bytes_including_nul=1244,
        extra_constructor_sentinel_bytes=4,shared_arithmetic_reused=True,base_provider_absent=True,
        production_normal_and_actual_asan_ubsan_pass=True,mutable_source_values_imported=False,
        live_q_or_playback_admitted=False,launcher_changed=False)
    target.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    verify(artifacts);verify(prior);verify(owned)
    print(json.dumps(dict(manifest_sha256=sha(target),artifacts=len(artifacts),protected_prior=len(prior),owned=len(owned),mismatches=0)))

if __name__=='__main__':main()
