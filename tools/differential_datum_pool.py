#!/usr/bin/env python3
"""Compare native datum storage/identity operations with frozen original pools.

Lifecycle replay compares only allocation/salt/bitmap state, not source player
records, binding/death callbacks, original arenas or the complete world order.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/spawn-source'
REPORT=BASE/'datums-report.json'
def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,default=ROOT/'build-multiplayer/datum_pool_tests')
    p.add_argument('--fixtures',type=Path,default=ROOT/'analysis/multiplayer/integration/datum-pool-fixtures.tsv')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/datum-pool-differential.json')
    p.add_argument('--export-only',action='store_true')
    a=p.parse_args()
    if ROOT/'analysis' not in a.fixtures.resolve().parents: raise ValueError('Keep original exports in ignored analysis')
    for path,pin in {
        REPORT:'9d93d609507bd352f13d6bd9c202071f26b05ee2ea4bb942a87ce3fcdf945629',
        ROOT/'tools/oracle_mp_spawn_datums.py':'cc9aa611cd8e482a9e29879b731835c76544e697d08acedf8c44a11a515fea11',
        BASE/'tu11-datums-instructions.txt':'1bcd717925b5b8642370db4ee6c0dab9dd31af87d94ff9039ee8a5a240077d88',
    }.items():
        if digest(path)!=pin: raise ValueError(f'Original datum authority changed: {path}')
    source=json.loads(REPORT.read_text())
    for relative,pin in source['dependencies'].items():
        if digest(ROOT/relative)!=pin: raise ValueError(f'Original datum dependency changed: {relative}')
    for name,pin in source['frozen_report_inputs'].items():
        if digest(BASE/name)!=pin: raise ValueError(f'Prior source report changed: {name}')
    for path,pin in {
        ROOT/source['source_path']:source['source_sha256'],
        ROOT/'assets/disc/maps/lockout.map':source['cache_sha256'],
    }.items():
        if digest(path)!=pin: raise ValueError(f'Original datum input changed: {path}')
    lines=[]
    def new(s,flags=4): lines.append(f"N {s['name']} {s['capacity']} {s['stride']} {flags}")
    def snapshot(s,full=True):
        values=['S',str(s['first_free']),str(s['high_water']),str(s['count']),s['next_salt'],s['bitmap_hex'],str(len(s['occupied'])),str(int(full))]
        for e in s['occupied']:
            values += [str(e['index']),e['salt']]
            if full: values.append(e['record_hex'])
        lines.append(' '.join(values))
    for r in source['allocator_cases']:
        kind=r['case']
        if kind=='actual_initializer_callers':
            for key in ('players','objects'):new(r[key]);snapshot(r[key])
        elif kind=='automatic_hole_and_trailing_release':
            new(r['initial']);snapshot(r['initial'])
            for d in r['allocated']:lines.append('A '+d)
            snapshot(r['after_three']);lines.append('D '+r['allocated'][1]);snapshot(r['hole'])
            lines.append('A '+r['reused']);lines.append('L '+r['allocated'][1]+' 0');lines.append('L '+r['reused']+' 1')
            lines.append('D '+r['allocated'][2]);snapshot(r['after_trailing_release'])
            lines.append('D '+r['reused']);lines.append('D '+r['allocated'][0]);snapshot(r['final'])
        elif kind=='salt_wrap':
            new(r['after']);lines.append('W '+r['supplied_counter'])
            for d in r['allocated']:lines.append('A '+d)
            snapshot(r['after'])
        elif kind=='explicit_index_reject':
            new(r['before']);e=r['before']['occupied'][0]
            lines.append(f"I {e['index']} {e['salt']}{e['index']:04x}");snapshot(r['before'])
            lines.append(f"I {r['index']} {r['result']}");snapshot(r['after'])
        elif kind=='explicit_index_preserves_cursor':
            new(r['after']);lines.append('I 5 '+r['explicit']);snapshot(r['after_explicit'])
            lines.append('A '+r['automatic']);snapshot(r['after'])
        elif kind=='poison_fill_and_map_reset':
            new(r['after'],8);lines.append('A '+r['allocated']);lines.append('D '+r['allocated'])
            lines.append('F 0 '+r['released_hex'])
            # Original free-element poison bytes are retained by the primitive.
            lines.append('Z');lines.append('A '+r['reset_first']);snapshot(r['after'])
        elif kind=='bounded_capacity_exhaustion':
            new(r['before'])
            for d in r['allocated']:lines.append('A '+d)
            snapshot(r['before']);lines.append('A '+r['result']);snapshot(r['after'])
        else:raise ValueError(kind)
    for r in source['storage_cases']:
        kind=r['case']
        if kind=='body_and_header_primitive_release':
            new(r['after']);lines.append('A '+r['released']);lines.append('D '+r['released']);snapshot(r['after'],False)
        else:
            new(r['before']);snapshot(r['before']);allocated=f"{r['before']['next_salt']}0000"
            lines.append('A '+allocated)
            if r['result']=='ffffffff':lines.append('D '+allocated)
            snapshot(r['after'],False)
    for r in source['lifecycle_cases']:
        # One shared object pool. Exact source allocation/release order is
        # replayed; mutated player/object body bytes are intentionally excluded.
        s=r['stages'][0]['state'];new(s['object_pool']);lines.append('A '+r['original_unit']);lines.append('A '+r['projectile']);snapshot(s['object_pool'],False)
        for stage in r['stages'][1:]:
            if stage['stage']=='delete_callback_then_header_release':lines.append('D '+r['original_unit'])
            elif stage['stage']=='same_player_bound_to_respawn_unit':lines.append('A '+r['respawn_unit'])
            elif stage['stage']=='same_slot_rejoined_new_player_and_unit':lines.append('A '+r['new_unit'])
            snapshot(stage['state']['object_pool'],False)
        lines.append('L '+r['original_unit']+' '+str(int(not r['delete_corpse'])))
        new(s['player_pool']);lines.append(f"I {r['slot']} {r['player']}");snapshot(s['player_pool'],False)
        lines.append('D '+r['player']);snapshot(next(x['state']['player_pool']for x in r['stages']if x['stage']=='actual_player_leave_with_object_callbacks'),False)
        lines.append(f"I {r['slot']} {r['rejoined_player']}");snapshot(r['stages'][-1]['state']['player_pool'],False)
        lines.append('L '+r['player']+' 0');lines.append('L '+r['rejoined_player']+' 1');lines.append('L ffffffff 0')
    a.fixtures.write_text('\n'.join(lines)+'\n')
    case=next(r for r in source['lifecycle_cases']if r['slot']==0 and not r['delete_corpse'])
    (a.fixtures.parent/'player-identity-fixture.tsv').write_text(case['player']+' '+case['rejoined_player']+'\n')
    if a.export_only:print('Exported all 25 source cases for bounded pool comparisons');return
    run=subprocess.run([str(a.binary.resolve()),str(a.fixtures.resolve())],text=True,capture_output=True)
    print(run.stdout,end='');print(run.stderr,end='')
    a.output.write_text(json.dumps(dict(passed=run.returncode==0,source_cases=25,scope=__doc__,
        source_report_sha256=digest(REPORT),native_binary_sha256=digest(a.binary),
        fixtures_sha256=digest(a.fixtures),tool_sha256=digest(Path(__file__)),stdout=run.stdout,stderr=run.stderr),indent=2)+'\n')
    raise SystemExit(run.returncode)
if __name__=='__main__':main()
