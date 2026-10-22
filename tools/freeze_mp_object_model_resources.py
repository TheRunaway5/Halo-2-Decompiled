#!/usr/bin/env python3
"""Freeze original model-resource query controls and registered native owner."""
import json
from freeze_mp_native_sound_storage import ROOT,sha,actual,relative,add,verify,copy

BASE=ROOT/'analysis/multiplayer/integration/object-model-resources-native'
FROZEN=BASE/'frozen'
DEPS={
 'analysis/multiplayer/integration/object-construction-provider-20261009/frozen/manifest.json':
 '5209c868282f97bf3d39caf6e262f62c88f2ed59e802f8c5adbbcbddea1c48f9',
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
    assert sha(BASE/'owning/capture.json')==sha(BASE/'owning-repeat/capture.json')=='d3ba9d84b9db0ffc188664f39911b49bed70b7c6b18b512a446054a04ab227bf'
    assert sha(BASE/'measured/report.json')==sha(BASE/'repeated/report.json')=='bf1f4bd253cf5075f1cec9007da0725863db8ee94956f76f661f06341bc66330'
    original=json.loads((BASE/'measured/report.json').read_text())
    assert len(original['cases'])==60 and sum(len(q['stores'])for q in original['cases'])==96
    assert not original['instruction_patches']and not original['supplied_callee_results']
    for name in ('fixtures.tsv','report.json'):
        assert (BASE/'fixtures'/name).read_bytes()==(BASE/'fixtures-repeat'/name).read_bytes()
    assert sha(BASE/'fixtures/fixtures.tsv')=='ddd113f08aca3b4889582d9381a581606061a6d40cdee39e8aa0ac57575c035a'
    validation=json.loads((BASE/'validation/report.json').read_text())
    assert all(not q['exit_code']for q in json.loads((BASE/'validation/commands.json').read_text()))
    for name,pin in validation['source_pins'].items():assert sha(actual(name))==sha(BASE/'validation/source'/name)==pin
    for name in ('production-normal','production-sanitizer'):
        report=json.loads((BASE/name/'report.json').read_text())
        assert all(not q['exit_code']for q in json.loads((BASE/name/'commands.json').read_text()).values())
        assert '60 original cases, 96 stores, 21120 full compared bytes'in(BASE/name/'test.log').read_text()
        for source,pin in report['observed_before_source_pins'].items():assert sha(BASE/name/'source-before'/source)==pin
        if name=='production-sanitizer':assert report['actual_sanitizer_flags_checked']
    for folder in ('owning','owning-repeat','measured','repeated','fixtures','fixtures-repeat','validation',
                   'production-normal','production-sanitizer'):
        for path in sorted((BASE/folder).rglob('*')):
            if path.is_file():copy(path,FROZEN/'evidence'/folder/path.relative_to(BASE/folder))
    names=['include/halo2/object_model_resources.hpp','src/gameplay/objects/object_model_resources.cpp',
        'tests/object_model_resources_tests.cpp','tools/oracle_object_model_resources_native.py',
        'tools/measure_object_model_resources_native.py','tools/export_object_model_resource_fixtures.py',
        'tools/validate_object_model_resources_native.py','tools/validate_object_model_resources_production.py',
        'tools/freeze_mp_object_model_resources.py','docs/MP_OBJECT_MODEL_RESOURCES_RESEARCH.md']
    owned={name:sha(actual(name))for name in names}
    for name in names+['CMakeLists.txt']:copy(actual(name),FROZEN/'source'/name)
    artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
    result=dict(schema='object-model-resource-frozen-v1',artifact_pins=artifacts,protected_prior_pins=prior,
        owned_source_pins=owned,dependency_manifests=DEPS,complete_instructions=49,
        actual_owning_calls=2,independent_owning_and_direct_repeats=True,
        original_query_controls=60,ordered_output_stores=96,full_retained_output_bytes=21120,
        source_private_abi_stack_reproduced=False,copied_and_production_normal_asan_ubsan_pass=True,
        positive_live_resource_readiness=False,final_native_unit_injection=False,live_gameplay_fixed=False,
        runtime_or_launcher_changed=False)
    destination.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    verify(artifacts);verify(prior);verify(owned)
    print(json.dumps(dict(manifest_sha256=sha(destination),artifacts=len(artifacts),
                         protected_prior=len(prior),owned=len(owned),mismatches=0)))

if __name__=='__main__':main()
