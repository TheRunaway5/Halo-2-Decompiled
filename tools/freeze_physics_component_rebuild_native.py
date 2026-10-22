#!/usr/bin/env python3
"""Freeze verified component callers and immutable selected-image inputs."""
import json
from freeze_mp_native_sound_storage import ROOT,sha,actual,relative,add,verify,copy

BASE=ROOT/'analysis/multiplayer/integration/physics-component-rebuild-native'
FROZEN=BASE/'frozen'
DEPENDENCIES={
 'analysis/multiplayer/integration/physics-component-construction-native/frozen/manifest.json':'5a1afec9ffe6d2d070332627f135c467fdac7a3ceccb5ae9e20df8d8df8c2b0d',
 'analysis/multiplayer/integration/spawn-source/object-preparation-pressure/manifest.json':'8d698c2db4965068d087357ab4165b1016b9252416b6d9e1521ff73dce0a27f0',
}

def main():
    destination=FROZEN/'manifest.json'
    if destination.exists():
        result=json.loads(destination.read_text())
        for group in('artifact_pins','protected_prior_pins','owned_source_pins'):verify(result[group])
        print(json.dumps(dict(manifest_sha256=sha(destination),audit='passed')));return
    prior=dict(DEPENDENCIES);verify(prior)
    for name in DEPENDENCIES:
        dependency=json.loads(actual(name).read_text())
        for group in('artifact_pins','protected_prior_pins','owned_current_files','evidence_files','original_inputs',
            'prior_pins','files','dependency_pins','new_source_pins','owned_source_pins'):
            for path,pin in dependency.get(group,{}).items():add(prior,path,pin)
    verify(prior)
    for left,right in(('measured','repeated'),('paired','paired-repeat'),('controls','controls-repeat'),
        ('fixtures','fixtures-repeat'),('immutable-operands','immutable-operands-repeat')):
        files={p.relative_to(BASE/left):p for p in(BASE/left).rglob('*')if p.is_file()}
        repeated={p.relative_to(BASE/right):p for p in(BASE/right).rglob('*')if p.is_file()}
        assert files.keys()==repeated.keys()
        for name,path in files.items():assert path.read_bytes()==repeated[name].read_bytes(),name
    source=json.loads((BASE/'measured/report.json').read_text())
    assert source['entire_factory_and_retained_slot_sequence_identical']and len(source['calls'])==2 and len(source['stores'])==4
    assert not source['instruction_patches']and not source['supplied_callee_results']
    fixtures=json.loads((BASE/'fixtures/report.json').read_text());verify(fixtures['input_pins'])
    assert fixtures['cases']==110 and fixtures['stores']==95 and fixtures['full_output_bytes']==2524868
    assert fixtures['recorded_lower_boundaries_for_upper_caller_only']==2 and fixtures['no_replay_required_stops']==109
    assert not fixtures['expected_from_native']
    operands=json.loads((BASE/'immutable-operands/report.json').read_text());verify(operands['input_pins'])
    assert operands['raw_scalar_bytes']==56 and operands['raw_dispatch_bytes']==74
    assert not operands['mutable_heap_pool_clock_world_readiness_imported']
    native=json.loads((BASE/'native-validation/report.json').read_text());verify(native['source_pins'])
    assert native['normal_and_actual_asan_ubsan_pass']and not native['native_composed_prefix_replay']
    for folder in('production-normal','production-sanitizer'):
        report=json.loads((BASE/folder/'report.json').read_text())
        assert all(not c['exit_code']for c in json.loads((BASE/folder/'commands.json').read_text()).values())
        assert '100% tests passed out of 7'in(BASE/folder/'test.log').read_text()
        for path,pin in report['observed_before_source_pins'].items():assert sha(BASE/folder/'source-before'/path)==pin
        if folder=='production-sanitizer':assert report['actual_sanitizer_flags_checked']
    for folder in('measured','repeated','paired','paired-repeat','controls','controls-repeat','fixtures','fixtures-repeat',
        'immutable-operands','immutable-operands-repeat','native-validation','production-normal','production-sanitizer'):
        for path in sorted((BASE/folder).rglob('*')):
            if path.is_file():copy(path,FROZEN/'evidence'/folder/path.relative_to(BASE/folder))
    names=['include/halo2/physics_component_rebuild.hpp','src/physics/physics_component_rebuild.cpp','tests/physics_component_rebuild_tests.cpp',
        'tests/physics_construction_operands_tests.cpp','tools/oracle_physics_component_rebuild_native.py',
        'tools/research_physics_component_rebuild_u5.py','tools/measure_physics_component_rebuild_controls.py',
        'tools/export_physics_component_rebuild_fixtures.py','tools/export_physics_construction_operands.py',
        'tools/validate_physics_component_rebuild_native.py','tools/validate_physics_component_rebuild_production.py',
        'tools/freeze_physics_component_rebuild_native.py','docs/MP_PHYSICS_COMPONENT_REBUILD_NATIVE_RESEARCH.md']
    owned={name:sha(actual(name))for name in names}
    for name in names+['CMakeLists.txt','include/halo2/original_data.hpp','src/assets/original_data.cpp']:copy(actual(name),FROZEN/'source'/name)
    artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
    result=dict(schema='physics-component-rebuild-frozen-v1',artifact_pins=artifacts,protected_prior_pins=prior,owned_source_pins=owned,
        dependency_manifests=DEPENDENCIES,complete_cpu_callers=2,complete_paired_caller_instructions=93,
        cases_and_required_stops=110,ordered_stores=95,full_output_bytes=2524868,upper_caller_only_recorded_replays=2,
        no_replay_required_stops=109,native_outer_allocator_init_rebuild_prefixes=1,immutable_scalar_bytes=56,immutable_indexed_table_bytes=74,
        production_normal_and_actual_asan_ubsan_seven_tests_pass=True,live_world_collision_or_audio_completion_claimed=False,launcher_changed=False)
    destination.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');verify(artifacts);verify(prior);verify(owned)
    print(json.dumps(dict(manifest_sha256=sha(destination),artifacts=len(artifacts),protected_prior=len(prior),owned=len(owned),mismatches=0)))

if __name__=='__main__':main()
