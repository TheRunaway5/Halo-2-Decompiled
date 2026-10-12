#!/usr/bin/env python3
"""Compare native signed countdown kernels with unchanged original execution.

Original frame gathering, ownership changes, world phases and notification
delivery are separate inputs. The native match uses these kernels in its early
player phase; this report compares the bounded timer arithmetic and eligibility.
"""
import argparse
import ctypes as c
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

class Timer(c.Structure):
    _fields_ = [(k,c.c_int32) for k in ('countdown','penalty','displayed_seconds','death_elapsed')]

def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def state(row): return Timer(*(row[k] for k in ('countdown','penalty','displayed_seconds','death_elapsed')))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library',type=Path,default=ROOT/'build-multiplayer/libhalo2_weapon_kernels.so')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/respawn-timing-differential.json')
    parser.add_argument('--export-fixtures',type=Path)
    args=parser.parse_args()
    report_path=ROOT/'analysis/multiplayer/integration/spawn-source/timing-report.json'
    report=json.loads(report_path.read_text())
    if not report['passed']: raise ValueError('Original timing oracle failed')
    if digest(ROOT/'tools/oracle_mp_spawn_timing.py')!=report['oracle_tool_sha256']:
        raise ValueError('Original timing tool differs')
    for name,h in report['imported_tool_sha256'].items():
        if digest(ROOT/'tools'/f'{name}.py')!=h: raise ValueError('Imported original oracle differs')
    for source in [*report['authorities'].values(),report['cache']]:
        if digest(ROOT/source['path'])!=source['sha256']: raise ValueError('Original source differs')
    lib=c.CDLL(str(args.library.resolve()))
    init=lib.halo2_initialize_respawn_timer
    init.argtypes=[c.POINTER(Timer),c.c_uint32,c.c_uint32]+[c.c_int32]*3
    init.restype=None
    tick=lib.halo2_tick_respawn_timer
    tick.argtypes=[c.POINTER(Timer)]+[c.c_uint32]*3+[c.c_int]*4+[c.POINTER(c.c_uint)]
    tick.restype=c.c_int
    visits=deaths=0;fixtures=[]
    for image,rows in report['rows'].items():
        for kind in ('timelines','boundaries'):
            for case in rows[kind]:
                for index,sample in enumerate(case['ticks']):
                    before=sample['before']
                    current=state(before)
                    if sample['after_world_event'] and (kind=='timelines' or
                            case['case']=='release_delayed_two_ticks' and index==0):
                        seconds=case.get('interval_seconds',1)
                        suicide=case.get('suicide_penalty_seconds') or 0
                        current.penalty=case.get('input_stored_penalty_ticks',0)
                        init(c.byref(current),before['game_tick'],report['engine_start_tick'],
                             seconds*report['tick_rate'],report['tick_rate'],suicide*report['tick_rate'])
                        expected=state(sample['after_world_event'])
                        if bytes(current)!=bytes(expected):
                            raise AssertionError((image,case['case'],'death',tuple(current)))
                        fixtures.append(' '.join(map(str,['death',before['game_tick'],0,
                            seconds*30,30,suicide*30,case.get('input_stored_penalty_ticks',0),
                            before['displayed_seconds'],before['death_elapsed'],
                            *[getattr(expected,k) for k,_ in Timer._fields_]])))
                        deaths+=1
                    current=state(before)
                    notice_rows=[]
                    for call in sample['eligibility']:
                        phase=not(case['case']=='engine_phase_gate' and index<2)
                        notice=c.c_uint()
                        eligible=tick(c.byref(current),int(call['player'],16),call['game_tick'],
                            report['engine_start_tick'],report['tick_rate'],1,1,phase,c.byref(notice))
                        if eligible!=call['result'] or current.countdown!=call['after'] or \
                                current.displayed_seconds!=call['displayed_seconds']:
                            raise AssertionError((image,case['case'],call,'eligibility'))
                        if notice.value: notice_rows.append(notice.value)
                        fixtures.append(' '.join(map(str,['tick',int(call['player'],16),
                            call['game_tick'],0,30,int(phase),call['before'],before['penalty'],
                            before['displayed_seconds'],before['death_elapsed'],
                            current.countdown,current.penalty,current.displayed_seconds,
                            current.death_elapsed,int(eligible),notice.value])))
                        visits+=1
                    if notice_rows!=[n['subtype'] for n in sample['notices']]:
                        raise AssertionError((image,case['case'],'notice ordering'))
    if args.export_fixtures:
        if ROOT/'analysis' not in args.export_fixtures.resolve().parents:
            raise ValueError('Original fixtures must remain under ignored analysis')
        args.export_fixtures.write_text('\n'.join(fixtures)+'\n')
    args.output.write_text(json.dumps(dict(passed=True,death_writes=deaths,
        eligibility_calls=visits,original_caller_ticks=report['tick_count'],
        native_library_sha256=digest(args.library),original_report_sha256=digest(report_path),
        tool_sha256=digest(Path(__file__)),scope=__doc__),indent=2)+'\n')
    print(f'Matched {deaths} original death timers and {visits} original eligibility calls')

if __name__=='__main__': main()
