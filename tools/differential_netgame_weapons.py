#!/usr/bin/env python3
"""Compare bounded stock weapon-row timers with original instruction traces.

Allocation results and the original oracle's explicit object-position fixture
are inputs. This does not validate physical allocation or live pickup targeting.
"""
import argparse
import ctypes as c
import hashlib
import json
from pathlib import Path
import struct
from differential_weapon_error import Vector

ROOT = Path(__file__).resolve().parents[1]
class State(c.Structure):
    _fields_ = [('object',c.c_uint32),('timer',c.c_int16),('empty_counter',c.c_int16)]
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def f32(x): return struct.unpack('<f',struct.pack('<f',x))[0]
def state(row): return State(int(row['object'],16),row['timer'],row['empty_counter'])

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library',type=Path,default=ROOT/'build-multiplayer/libhalo2_weapon_kernels.so')
    parser.add_argument('--report',type=Path,default=ROOT/'analysis/multiplayer/integration/inventory-source/netgame/report.json')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/netgame-weapons-differential.json')
    parser.add_argument('--export-fixtures',type=Path)
    args=parser.parse_args();report=json.loads(args.report.read_text())
    if digest(ROOT/'tools/oracle_mp_inventory_netgame_1_1.py')!=report['oracle_tool_sha256']:
        raise ValueError('Original oracle differs from report')
    if digest(ROOT/'assets/disc/maps/lockout.map')!=report['cache_sha256']:
        raise ValueError('Original source cache differs')
    if digest(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')!=report['xbe_sha256']:
        raise ValueError('Original source executable differs')
    library=c.CDLL(str(args.library.resolve()))
    tick=library.halo2_netgame_weapon_timer
    tick.argtypes=[c.POINTER(State),c.c_uint,c.c_int32]+[c.c_int]*3+[
        c.POINTER(Vector),c.POINTER(Vector),c.c_uint32,c.c_int,c.c_int]
    tick.restype=c.c_int
    complete=library.halo2_netgame_weapon_spawn_result
    complete.argtypes=[c.POINTER(State),c.c_uint32];complete.restype=None
    records={r['index']:r for r in report['equipment']}
    visits=0;requests=0;fixtures=[]
    bits=lambda x:f'{struct.unpack("<I",struct.pack("<f",x))[0]:08x}'
    for equipment in report['equipment']:
        item=equipment['collection']['items'][0]
        fixtures.append(' '.join(map(str,['equipment',equipment['index'],equipment['flags'],
            *equipment['modes'],equipment['team'],equipment['spawn_seconds'],
            equipment['respawn_on_empty_seconds'],equipment['timer_starts'],equipment['classification'],
            *[bits(x) for x in equipment['position']+equipment['orientation']],
            equipment['collection']['datum'],equipment['collection']['spawn_seconds'],
            bits(item['weight']),item['item']['datum'],item['variant']])))
    for case in report['cases']:
        row=records[case['row']];authored=Vector(*row['position'])
        allocations={(e['tick'],e['phase']):int(e['return_value'],16)
            for e in case['events'] if e.get('service')=='000b8a30'}
        old_flags=next(int(w['flags'],16) for w in case['final_inventory']['weapons']
                       if w['index']=='00010002')
        for sample in case['samples']:
            current=state(sample['before']);position=Vector(*row['position']);flags=0
            # These mutations are the explicit source oracle's fixture inputs,
            # not new gameplay behavior. New allocations return unowned objects.
            if sample['tick']>=case['first_timer_visit_tick'] and current.object==0x10002:
                flags=old_flags
                if case['reason'] in ('at_threshold','beyond_threshold'):
                    position.x=f32(position.x+(1 if case['reason']=='at_threshold' else 1.125))
            requested=bool(tick(c.byref(current),case['row'],sample['tick'],30,1,0,
                c.byref(authored),c.byref(position),flags,row['spawn_seconds'],case['weapon_timer_option']))
            key=(sample['tick'],sample['name'])
            if requested!=(key in allocations):
                raise AssertionError((case['row'],case['reason'],key,'allocation request'))
            if requested:
                complete(c.byref(current),allocations[key]);requests+=1
            if bytes(current)!=bytes(state(sample['after'])):
                raise AssertionError((case['row'],case['reason'],key,bytes(current).hex(),sample['after']))
            before=sample['before'];after=sample['after']
            fixtures.append(' '.join(map(str,['visit',case['row'],sample['tick'],flags,
                *[bits(v) for v in (position.x,position.y,position.z)],case['weapon_timer_option'],
                before['object'],before['timer'],before['empty_counter'],int(requested),
                allocations.get(key,0xffffffff),after['object'],after['timer'],after['empty_counter']])))
            visits+=1
    if args.export_fixtures:
        if ROOT/'analysis' not in args.export_fixtures.resolve().parents:
            raise ValueError('Original authored fixtures must remain in ignored analysis')
        args.export_fixtures.write_text('\n'.join(fixtures)+'\n')
    args.output.write_text(json.dumps(dict(passed=True,cases=len(report['cases']),timer_visits=visits,
        allocation_requests=requests,native_library_sha256=digest(args.library),
        original_report_sha256=digest(args.report),tool_sha256=digest(Path(__file__)),
        scope='Original stock weapon timer-start0/classification0/type2 phase cadence, displacement gate, options, decrement and supplied allocation-result state. Eligibility, world allocation/settling and pickup remain separate.'),indent=2)+'\n')
    print(f'Matched {len(report["cases"])} original weapon timer cases, {visits} visits and {requests} allocation requests')
if __name__=='__main__':main()
