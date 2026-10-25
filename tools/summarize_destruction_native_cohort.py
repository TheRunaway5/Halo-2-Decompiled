#!/usr/bin/env python3
"""Summarize bounded native-cohort evidence without inferring full acceptance."""
import argparse
import json
from pathlib import Path

from refresh_destruction_current_cohort import sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--admission',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args = parser.parse_args()
    admitted = json.loads(args.admission.read_text())
    validation = args.admission.parent/'validation'
    worker = json.loads((validation/'report.json').read_text())
    cases = {}
    for case in admitted['cases']:
        path = validation/'evidence'/f'destruction-{case}.json'
        if not path.is_file():
            cases[case] = dict(passed=False,error='No retained native case report')
            continue
        raw = json.loads(path.read_text())
        post = raw.get('post_destruction_continuation',{})
        pending = raw.get('pending_response_continuation',{})
        fresh = isinstance(raw.get('native_vehicle_rate_activity'),dict)
        cases[case] = dict(passed=raw.get('passed',False) and fresh,report=str(path),sha256=sha(path),
                          error=raw.get('error') if fresh else 'Report predates the admitted native diagnostics fixture',
                          post_comparisons=post.get('complete_encoded_world_sha256_comparisons',0),
                          pending_comparisons=pending.get('complete_encoded_world_sha256_comparisons',0),
                          source_dead=raw.get('source_dead'),target_retired=raw.get('target_retired'),
                          shots=raw.get('shots'),minimum_body=raw.get('minimum_body'),
                          declared_fixed_clock=raw.get('declared_external_clock',False),
                          native_rate_activity=raw.get('native_vehicle_rate_activity'),
                          native_record_activity=raw.get('native_damage_record_activity'),
                          native_model_activity=raw.get('native_damage_model_activity'),
                          original_no_damage_information_gate_verified=raw.get('original_no_damage_information_gate_verified'),
                          original_explosion_death_attribution_verified=raw.get('original_explosion_death_attribution_verified'),
                          native_record_full_target_or_weapon_verified=any(
                              event.get('full_generation_live') and (event.get('is_target') or event.get('is_target_selected_weapon'))
                              for event in raw.get('actual_native_record_calls',[])),
                          global_vitality_unchanged_original_death=raw.get('global_vitality_unchanged_original_death'))
    ghost = cases.get('ghost',{}).get('native_rate_activity') or {}
    activity = {key:sum((row.get('native_rate_activity') or {}).get(key,0) for row in cases.values() if row['passed'])
                for key in ('acceleration_calls','approach_calls','live_cursor_calls')}
    command = admitted['command']
    targets = {command[n+1] for n,value in enumerate(command[:-1]) if value=='--target'}
    required_targets = {'halo2_native','physics_multiplayer_acceptance','destruction_runtime_tu15_tests'}
    clump=cases.get('campaign_flood_clump',{})
    minigun=cases.get('wraith_minigun',{})
    no_damage=cases.get('campaign_guntower_no_damage',{})
    native_model_admitted='src/gameplay/objects/source_damage_model_native_tu15_internal.hpp' in admitted.get('patches',{})
    complete = (required_targets<=targets and admitted.get('passed',False) and all(step.get('exit_code')==0 for step in worker['steps']) and
                len(cases)==len(admitted['cases']) and all(row['passed'] for row in cases.values()) and
                all(row.get('declared_fixed_clock') and row.get('post_comparisons')==16 for row in cases.values()) and
                cases.get('power_module',{}).get('pending_comparisons')==16 and
                ghost.get('live_cursor_calls',0)>0 and activity['acceleration_calls']>0 and
                (not clump or clump.get('original_explosion_death_attribution_verified') is True) and
                (not minigun or (minigun.get('native_record_full_target_or_weapon_verified') and
                    (minigun.get('native_record_activity') or {}).get('calls',0)>0)) and
                (not native_model_admitted or (all(isinstance(row.get('native_model_activity'),dict)for row in cases.values()) and
                    (cases.get('ghost',{}).get('native_model_activity') or {}).get('calls',0)>0)) and
                (not no_damage or (no_damage.get('original_no_damage_information_gate_verified') is True and
                    (no_damage.get('native_model_activity') or {}).get('no_damage_information',0)>0)))
    report = dict(passed=complete,scope='Native build and bounded destruction/continuation cases with actual live rate-kernel admission',
                  full_goal_complete=False,admission=str(args.admission),admission_sha256=sha(args.admission),
                  worker_report=str(validation/'report.json'),cases=cases,
                  total_complete_world_comparisons=sum(row.get('post_comparisons',0)+row.get('pending_comparisons',0)
                                                       for row in cases.values() if row['passed']),
                  actual_ghost_live_native_kernel_calls=ghost.get('live_cursor_calls',0),
                  actual_native_calls_by_owner=activity,
                  actual_native_record_calls=sum((row.get('native_record_activity') or {}).get('calls',0)
                      for row in cases.values() if row['passed']),
                  native_model_lookup_admitted=native_model_admitted,
                  actual_native_model_lookup_calls=sum((row.get('native_model_activity') or {}).get('calls',0)
                      for row in cases.values() if row['passed']),
                  both_rate_owners_observed_in_runtime=activity['acceleration_calls']>0 and activity['approach_calls']>0,
                  all_37_full_definition_gates_remain_open=True,retained_large_artifacts=[])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=complete,cases_passed=sum(row['passed'] for row in cases.values()),
                         cases=len(cases),comparisons=report['total_complete_world_comparisons'],
                         ghost_live_kernel_calls=report['actual_ghost_live_native_kernel_calls'],report=str(args.output))))
    return 0 if complete else 1


if __name__=='__main__':
    raise SystemExit(main())
