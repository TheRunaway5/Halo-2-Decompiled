#!/usr/bin/env python3
"""Audit complete, fresh per-case evidence from the owned native vehicle cohort."""
import argparse
import json
from pathlib import Path

from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]
PRIMARY=('ghost','banshee','scorpion','wraith','warthog-mg','warthog-gauss',
         'spectre','h_turret_ap','c_turret_ap')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('cohort','refresh','validation','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();cohort=json.loads(args.cohort.read_text())
    refresh=json.loads(args.refresh.read_text());validation=json.loads(args.validation.read_text())
    build=Path(validation['build']).resolve();owner=Path(cohort['owner']).resolve()
    if build!=owner/'build' or refresh['owner']!=str(owner):raise ValueError('Foreign validation owner')
    journal=json.loads((ROOT/'analysis/vehicles-tu15-goal-20261023/session-v1/session.json').read_text())
    commands=[row for row in journal['commands'] if
        (any(args.validation.parent.name in arg and 'tools/run_tests.py' in arg for arg in row['argv']) or
         (any(Path(arg).name=='run_vehicle_goal_cohort_tu15.py' for arg in row['argv'] if '\n' not in arg) and
          any(arg.rstrip('/').endswith('/'+args.validation.parent.name) for arg in row['argv'])))]
    if len(commands)!=1 or 'finished' not in commands[0]:raise ValueError('One finished cohort command is required')
    started=commands[0]['started'];retained={row['path']:row for row in validation['retained_small_reports']}
    wanted={**{'vehicle-variant-'+case+'.json':'variant_constructor' for case in refresh['registered_cases']},
        **{'vehicle-destruction-'+case+'.json':'destruction' for case in cohort['destruction_cases']},
        **{'vehicle-weapon-'+case+'.json':'primary_target_damage' for case in PRIMARY},
        'vehicle-secondary-banshee.json':'secondary_input',
        'vehicle-secondary-scorpion.json':'secondary_input',
        'vehicle-boost-ghost.json':'native_boost',
        'vehicle-bv-agent-original.json':'original_bv_component'}
    results=[];failed=[];counts={}
    for name,group in wanted.items():
        row=retained.get(name);record=dict(report=name,group=group,passed=False)
        try:
            if not row or row['status']!='retained':raise ValueError('Required bounded report was not retained')
            path=Path(row['retained_path']);live=build/name
            if sha(path)!=row['sha256'] or sha(live)!=row['sha256']:raise ValueError('Retained/live report hash differs')
            if live.stat().st_mtime<started:raise ValueError('Stale report from an earlier command')
            data=json.loads(path.read_text());record.update(sha256=row['sha256'],source_mtime=live.stat().st_mtime)
            if data.get('passed') is not True:raise ValueError(data.get('runtime_error',data.get('error','Case did not pass')))
            if group=='primary_target_damage':
                target=data['cases'][0]['primary_firing']['target_damage']
                if target['passed'] is not True:raise ValueError('Primary firing lacks its target-damage gate')
                record['target_damage']=target
            elif group=='native_boost':
                activity=data['native_vehicle_rate_activity'];boost=data['cases'][0]['ghost_boost']
                if not (boost['passed'] and activity['acceleration_calls']>0 and activity['live_cursor_calls']>0):
                    raise ValueError('Actual boost did not reach an authenticated native rate kernel')
                record.update(native_rate_activity=activity,original_boost=boost)
            elif group=='secondary_input':
                firing=data['cases'][0]['secondary_firing']
                if firing['passed'] is not True:raise ValueError('Secondary input gate did not pass')
                if 'banshee' in name and not (firing['authored_trigger_input']==2 and
                        not firing['original_mode_expects_projectile'] and
                        firing['multiplayer_third_input_suppression_checks']==180 and
                        firing['constructed_projectile_generations']==0):
                    raise ValueError('Original Banshee multiplayer suppression was not preserved')
                if 'scorpion' in name and not (firing['authored_trigger_input']==1 and
                        firing['constructed_projectile_generations']>0):
                    raise ValueError('Actual Scorpion secondary input published no projectile')
            elif group=='original_bv_component':
                if not (data['full_x87_status_compared'] and data['full_mxcsr_compared'] and
                        not data['production_opcode_execution'] and not data['subcall_results_supplied']):
                    raise ValueError('Strict original native BV comparison is incomplete')
            record['passed']=True;counts[group]=counts.get(group,0)+1
        except (OSError,ValueError,KeyError) as error:
            record['error']=str(error);failed.append(record)
        results.append(record)
    passed=validation['passed'] and not failed and len(results)==len(wanted)
    report=dict(passed=passed,validation=str(args.validation),validation_sha256=sha(args.validation),
        command_started=started,command_finished=commands[0]['finished'],
        expected_cases=len(wanted),counts=counts,failed=failed,results=results,
        every_retained_report_hash_and_freshness_verified=not failed,
        full_vehicle_parity=False,launcher_promoted=False,tool_sha256=sha(Path(__file__)))
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=passed,counts=counts,failures=len(failed),report=str(args.output))))
    return 0 if passed else 1


if __name__=='__main__':raise SystemExit(main())
