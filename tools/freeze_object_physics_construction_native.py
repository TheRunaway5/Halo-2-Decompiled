#!/usr/bin/env python3
"""Freeze complete physical-construction caller and labeled boundary tests."""
import json
from freeze_mp_native_sound_storage import ROOT,sha,actual,relative,add,verify,copy

BASE=ROOT/'analysis/multiplayer/integration/object-physics-construction-native'
FROZEN=BASE/'frozen'
DEPS={
 'analysis/multiplayer/integration/looping-construction-operands-20261009/frozen/manifest.json':'bcb5fa772eded922b4398da43cf34e6a19df1d9e63cedc4dc3e17119a26be0e5',
 'analysis/multiplayer/sandbox-source/power-body-slots/final/manifest.json':'3ad33293c3dc234089ca27c399d0f99434b62132bc0eac971601406ad7467d03',
}

def main():
    destination=FROZEN/'manifest.json'
    if destination.exists():
        report=json.loads(destination.read_text())
        for group in('artifact_pins','protected_prior_pins','owned_source_pins'):verify(report[group])
        print(json.dumps(dict(manifest_sha256=sha(destination),audit='passed')));return
    prior=dict(DEPS);verify(prior)
    for name in DEPS:
        dependency=json.loads(actual(name).read_text())
        for group in('artifact_pins','protected_prior_pins','owned_current_files','evidence_files','original_inputs',
            'prior_pins','files','dependency_pins','new_source_pins','owned_source_pins'):
            for path,pin in dependency.get(group,{}).items():add(prior,path,pin)
    verify(prior)
    for left,right in(('measured','repeated'),('paired','paired-repeat'),('controls','controls-repeat'),('fixtures','fixtures-repeat')):
        left_files={p.relative_to(BASE/left):p for p in(BASE/left).rglob('*')if p.is_file()}
        right_files={p.relative_to(BASE/right):p for p in(BASE/right).rglob('*')if p.is_file()}
        assert left_files.keys()==right_files.keys()
        for name,path in left_files.items():assert path.read_bytes()==right_files[name].read_bytes(),name
    original=json.loads((BASE/'measured/report.json').read_text())
    assert original['actual_factory_identical']and len(original['calls'])==5 and len(original['stores'])==3
    assert not original['instruction_patches']and not original['supplied_lower_results']
    fixture=json.loads((BASE/'fixtures/report.json').read_text())
    assert fixture['cases']==133 and fixture['stores']==147 and fixture['full_output_bytes']==4556774
    assert fixture['recorded_lower_boundaries_for_upper_caller_only']==5 and not fixture['expected_from_native']
    native=json.loads((BASE/'native-validation/report.json').read_text());assert native['normal_and_asan_ubsan_pass']
    for folder in('production-normal','production-sanitizer'):
        report=json.loads((BASE/folder/'report.json').read_text())
        assert all(not c['exit_code']for c in json.loads((BASE/folder/'commands.json').read_text()).values())
        assert '100% tests passed out of 1'in(BASE/folder/'test.log').read_text()
        for source,pin in report['observed_before_source_pins'].items():assert sha(BASE/folder/'source-before'/source)==pin
        if folder=='production-sanitizer':assert report['actual_sanitizer_flags_checked']
    folders=('measured','repeated','paired','paired-repeat','controls','controls-repeat','fixtures','fixtures-repeat',
        'native-validation','production-normal','production-sanitizer')
    for folder in folders:
        for path in sorted((BASE/folder).rglob('*')):
            if path.is_file():copy(path,FROZEN/'evidence'/folder/path.relative_to(BASE/folder))
    names=['include/halo2/object_physics_construction.hpp','src/object_physics_construction.cpp',
        'tests/object_physics_construction_tests.cpp','tools/oracle_object_physics_construction_native.py',
        'tools/research_object_physics_construction_u5.py','tools/measure_object_physics_construction_controls.py',
        'tools/export_object_physics_construction_fixtures.py','tools/validate_object_physics_construction_native.py',
        'tools/validate_object_physics_construction_production.py','tools/freeze_object_physics_construction_native.py',
        'docs/MP_OBJECT_PHYSICS_CONSTRUCTION_NATIVE_RESEARCH.md']
    owned={name:sha(actual(name))for name in names}
    for name in names+['CMakeLists.txt']:copy(actual(name),FROZEN/'source'/name)
    artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
    result=dict(schema='object-physics-construction-frozen-v1',artifact_pins=artifacts,protected_prior_pins=prior,
        owned_source_pins=owned,dependency_manifests=DEPS,complete_cpu_owners=5,paired_full_bodies_with_lower_targets=10,
        original_control_cases=132,actual_upper_caller_cases=1,recorded_lower_boundaries_upper_caller_only=5,
        ordered_stores=147,full_output_bytes=4556774,production_normal_and_actual_asan_ubsan_pass=True,
        complete_native_lower_physics_or_live_collision_claimed=False,launcher_changed=False)
    destination.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    verify(artifacts);verify(prior);verify(owned)
    print(json.dumps(dict(manifest_sha256=sha(destination),artifacts=len(artifacts),protected_prior=len(prior),owned=len(owned),mismatches=0)))

if __name__=='__main__':main()
