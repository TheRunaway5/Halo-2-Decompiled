#!/usr/bin/env python3
"""Freeze complete raw component owners and no-replay required world stop."""
import json
from freeze_mp_native_sound_storage import ROOT,sha,actual,relative,add,verify,copy

BASE=ROOT/'analysis/multiplayer/integration/physics-component-construction-native'
FROZEN=BASE/'frozen'
DEPS={
 'analysis/multiplayer/integration/object-physics-construction-native/frozen/manifest.json':'a7e347505a9d753648dcb70b6b02a1d7937fb8a5f1731a6cdc59609407c0bb35',
 'analysis/multiplayer/integration/fp-sound-submit-composition/final-manifest.json':'3101ff47d8e17449acc4e035004220bd6d243bf9925fcd858a5a30e1c403273b',
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
    for left,right in(('measured','repeated'),('paired','paired-repeat'),('controls','controls-repeat'),('fixtures-v2','fixtures-v2-repeat')):
        left_files={p.relative_to(BASE/left):p for p in(BASE/left).rglob('*')if p.is_file()}
        right_files={p.relative_to(BASE/right):p for p in(BASE/right).rglob('*')if p.is_file()}
        assert left_files.keys()==right_files.keys()
        for name,path in left_files.items():assert path.read_bytes()==right_files[name].read_bytes(),name
    original=json.loads((BASE/'measured/report.json').read_text())
    assert original['entire_factory_and_retained_slot_sequence_identical']and len(original['stores'])==71
    assert not original['instruction_patches']and not original['supplied_callee_results']
    fixture=json.loads((BASE/'fixtures-v2/report.json').read_text());verify(fixture['additional_input_pins'])
    assert fixture['cases']==511 and fixture['stores']==59789 and fixture['full_output_bytes']==3775976
    assert fixture['recorded_lower_boundaries_replayed']==0 and not fixture['expected_from_native']
    assert fixture['complete_original_control_cases']==510 and fixture['additional_native_composed_required_rebuild_stops']==1
    native=json.loads((BASE/'native-validation/report.json').read_text());assert native['normal_and_asan_ubsan_pass']and not native['recorded_lower_replay']
    verify(native['source_pins'])
    for folder in('production-normal','production-sanitizer'):
        report=json.loads((BASE/folder/'report.json').read_text())
        assert all(not c['exit_code']for c in json.loads((BASE/folder/'commands.json').read_text()).values())
        assert '100% tests passed out of 2'in(BASE/folder/'test.log').read_text()
        assert 'native SSE status retained (emulator status excluded)'in(BASE/folder/'test.log').read_text()
        for source,pin in report['observed_before_source_pins'].items():assert sha(BASE/folder/'source-before'/source)==pin
        if folder=='production-sanitizer':assert report['actual_sanitizer_flags_checked']
    for folder in('measured','repeated','paired','paired-repeat','controls','controls-repeat','fixtures-v2','fixtures-v2-repeat',
        'native-validation','production-normal','production-sanitizer'):
        for path in sorted((BASE/folder).rglob('*')):
            if path.is_file():copy(path,FROZEN/'evidence'/folder/path.relative_to(BASE/folder))
    names=['include/halo2/raw_datum_pool.hpp','src/gameplay/objects/raw_datum_pool.cpp','include/halo2/physics_component_construction.hpp',
        'src/physics/physics_component_construction.cpp','tests/physics_component_construction_tests.cpp',
        'tools/oracle_physics_component_construction_native.py','tools/research_physics_component_construction_u5.py',
        'tools/measure_physics_component_construction_controls.py','tools/export_physics_component_construction_fixtures.py',
        'tools/validate_physics_component_construction_native.py','tools/validate_physics_component_construction_production.py',
        'tools/freeze_physics_component_construction_native.py','docs/MP_PHYSICS_COMPONENT_CONSTRUCTION_NATIVE_RESEARCH.md']
    owned={name:sha(actual(name))for name in names}
    for name in names+['CMakeLists.txt']:copy(actual(name),FROZEN/'source'/name)
    artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
    result=dict(schema='physics-component-construction-frozen-v1',artifact_pins=artifacts,protected_prior_pins=prior,
        owned_source_pins=owned,dependency_manifests=DEPS,complete_cpu_owners=3,complete_paired_instructions=149,
        original_complete_cases=510,native_composed_required_rebuild_stops=1,calls=1561,ordered_stores=59789,
        full_output_bytes=3775976,recorded_lower_replay=False,actual_native_sse_status_retained=True,
        full_source_x87_stack_or_sticky_state_represented=False,registered_fp_world_raw_submit_cold_events=3,
        production_normal_and_actual_asan_ubsan_pass=True,complete_native_world_or_live_collision_claimed=False,launcher_changed=False)
    destination.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    verify(artifacts);verify(prior);verify(owned)
    print(json.dumps(dict(manifest_sha256=sha(destination),artifacts=len(artifacts),protected_prior=len(prior),owned=len(owned),mismatches=0)))

if __name__=='__main__':main()
