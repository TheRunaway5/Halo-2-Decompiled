#!/usr/bin/env python3
"""Verify frozen constrained-motion build and source-comparison provenance."""
import hashlib, json, re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
EVIDENCE=ROOT/'analysis/multiplayer/sandbox-source/physics-solver-motion-20261011'
BUILD=ROOT/'analysis/multiplayer/integration/physics-engine-20261010'
OLD=ROOT/'analysis/multiplayer/sandbox-source/physics-world-step-20261010'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    manifest=BUILD/'source-manifest-v3.json'
    assert sha(manifest)=='8f3dcd04c9357573a32d79d3fab0df3e148a5baf6c625b5c8780aa89af65743a'
    source=json.loads(manifest.read_text())
    for rel,pin in source['files'].items():assert sha(BUILD/'source-v3'/rel)==pin,rel
    for rel in source['owned_overlays']:
        if rel!='CMakeLists.txt':assert sha(ROOT/rel)==source['files'][rel],rel
    pins=[
        (OLD/'exported-verified-v2/fixture.json','650bed6d5f99d6ba521cd3a36fc9fb2f7ccacdad05153eccf9de186359317a27',117,7831,5884621,12),
        (OLD/'u5-verified/fixture.json','6cea7bf3b3c308d7d62876d3cb6f506265c438d0405009efd668c1d9c08262e3',48,2112,983040,0),
        (OLD/'connected-verified-v3/fixture.json','f14c2d1667515afa0e7c27f637568784d649ce0def764a28420b48c736e0a154',2,245,2367661,2),
        (EVIDENCE/'disc-verified/fixture.json','f3df4c4f8eb61666c843ff412b83576df3f28e5306108fb7e8c7d5b1561ee9ee',80,7776,1638400,0),
        (EVIDENCE/'u5-verified/fixture.json','a889302d575d6300f95f7f2bb9df90dfd107bbcc67d1ef16ecf89aa7e4e56245',80,7776,1638400,0),
        (EVIDENCE/'constraints-verified/fixture.json','ab4b5aae6097644883bba8c4759a0b0abf223f01157693ca33b7774affb96c5d',60,2772,3937200,60)]
    fixtures={};totals=[0,0,0,0]
    for path,pin,cases,writes,state,stops in pins:
        assert sha(path)==pin,str(path)
        f=json.loads(path.read_text());actual=[len(f['cases']),sum(len(c['writes']) for c in f['cases']),
            sum(sum(len(data)//2 for _,data in c['after_banks']) if 'after_banks' in c else len(c['after'])//2 for c in f['cases']),
            sum(c['boundary'] is not None for c in f['cases'])]
        assert actual==[cases,writes,state,stops],(str(path),actual)
        fixtures[str(path.relative_to(ROOT))]=dict(sha256=pin,cases=cases,ordered_persistent_writes=writes,full_state_bytes=state,required_stops=stops)
        totals=[a+b for a,b in zip(totals,actual)]
    repeats={}
    for name in ('disc','u5','constraints'):
        first=EVIDENCE/f'{name}-verified/fixture.json';repeat=EVIDENCE/f'{name}-repeat/fixture.json'
        assert sha(first)==sha(repeat)
        repeats[name]=sha(first)
    correspondence=EVIDENCE/'correspondence-verified/complete-report.json'
    assert sha(correspondence)=='aa3b9dad059a64a9ba52eac5e8807572da1d1c8db7b0f255c17a18e7a59f1c0f'
    paired=json.loads(correspondence.read_text());assert paired['complete_bodies']==22
    assert paired['extension_tool_sha256']==sha(ROOT/'tools/research_physics_solver_motion_20261011.py')
    assert paired['tool_sha256']==sha(ROOT/'tools/research_physics_world_step_20261010.py')
    lifecycle=EVIDENCE/'solver-lifecycle-verified-v3/report.json'
    assert sha(lifecycle)=='6071f480dbeeaabbe1b5cf81cb7978d93d6ae20993333e4ac5afbe5483415c72'
    assert sha(lifecycle)==sha(EVIDENCE/'solver-lifecycle-repeat-v3/report.json')
    research=json.loads(lifecycle.read_text());assert research['tool_sha256']==sha(ROOT/'tools/research_physics_solver_dispatch_20261011.py')
    assert not research['forced_initialization_flags'] and not research['source_instruction_patches']
    for image in research['images']:
        assert len(image['schema_dispatch'])==17 and len(image['cases'])==4
        for case in image['cases']:
            assert case['before_flag']=='00' and case['after_flag']=='01' and case['return_value']==1
            assert case['initialization_executed'] and case['initialization_checksum_code_executed']
    logs={}
    tests=['original_physics_world_step','original_physics_motion_tu15','original_physics_connected_world',
        'original_physics_solver_motion','original_physics_solver_motion_tu15','original_physics_constraint_step']
    for kind in ('production','asan'):
        for stage in ('configure','build','test'):
            path=EVIDENCE/f'{kind}-{stage}.log';logs[path.name]=sha(path)
        result=(EVIDENCE/f'{kind}-test.log').read_text()
        assert '100% tests passed out of 6' in result and 'Skipped' not in result
        for name in tests:assert re.search(r'\b'+name+r'\s+\.+\s+Passed',result),name
    modules=['physics_world_step','physics_motion_step','physics_constraint_step','physics_current_broadphase',
        'physics_pair_manager','physics_agent_scheduler','physics_bsp_children','physics_child_agents',
        'physics_narrowphase_owners','physics_gjk','physics_contact_dispatch','physics_contact_batches']
    ninja=(BUILD/'asan-v3/build.ninja').read_text()
    for module in modules:
        block=ninja.split(f'CMakeFiles/halo2_systems.dir/src/{module}.cpp.o:')[1].split('\n\n')[0]
        assert '-fsanitize=address,undefined' in block,module
    result=dict(schema=1,validator_sha256=sha(Path(__file__)),frozen_source_manifest_sha256=sha(manifest),
        frozen_source_files_verified=len(source['files']),correspondence_sha256=sha(correspondence),complete_paired_bodies=22,
        fixtures=fixtures,repeat_fixtures=repeats,total_cases=totals[0],total_ordered_persistent_writes=totals[1],
        total_full_state_bytes=totals[2],required_stops=totals[3],logs=logs,release_tests_passed=6,sanitizer_tests_passed=6,
        fully_instrumented_modules=modules,production_binary_sha256=sha(BUILD/'build-v3/physics_world_step_tests'),
        sanitizer_binary_sha256=sha(BUILD/'asan-v3/physics_world_step_tests'),
        original_only_solver_lifecycle=dict(report_sha256=sha(lifecycle),cases=8,native_solver_validation=False),
        all_constraint_types_ported=False,whole_physics_engine_complete=False,live_simulation_mover_replaced=False,
        published_launcher_replaced=False,live_lockout_lift_fixed=False,
        remaining=['Retained contact packet production','Real constraint/Jacobian producers and full322ED0 solver',
            'Completed-solve validation of322690 readback/release tails','Live allocator/world/controller and TU1.5 mutable bindings',
            'Deferred mutation, contact delivery, vehicle/ragdoll classes and all-map multiplayer validation'],
        limits=['Unicorn CPU comparisons do not prove Xbox hardware rounding','Readback velocity fields are declared arithmetic controls',
            'Constraint fixtures validate exact required-owner prefixes, not completed solves','Existing live host CollisionWorld mover remains'])
    path=EVIDENCE/'validation.json';path.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases=totals[0],writes=totals[1],bytes=totals[2],required_stops=totals[3],validation_sha256=sha(path))))
if __name__=='__main__':main()
