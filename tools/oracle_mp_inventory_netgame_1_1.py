#!/usr/bin/env python3
"""Execute original TU1.1 netgame equipment identity/timer instructions.

Actual Lockout metadata is mapped by InventoryTrace. Allocation supplies an
already original-initialized weapon; physical settling and networking remain
explicit boundaries. No original instruction bytes are replaced.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from oracle_mp_inventory_1_1 import (
    InventoryTrace, ROOT, PIN, CACHE_PIN, SCRATCH, SECOND, THIRD, W2, W3, Cache,
    UNIT, UI, POOL, GAME, SEED, TIME, UC_X86_REG_EAX, UC_X86_REG_ESI,
    UC_X86_REG_EDI, _trace,
)

OUT = ROOT / 'analysis/multiplayer/integration/inventory-source/netgame'
SMG = 'objects\\weapons\\rifle\\smg\\smg'
BR = 'objects\\weapons\\rifle\\battle_rifle\\battle_rifle'
ENTRIES = (0x15c110, 0x15c290, 0x15c320, 0x15c370, 0x15c3f0,
           0x15c4b0, 0x15c820, 0x15c860, 0x15d7c0, 0x15bc80,
           0x1601e0, 0x1a2b00, 0x1a2a90, 0xb8820, 0xbaa80,
           0xa8620, 0x11f680, 0x143450, 0x101bb0, 0xce510,
           0xcfed0, 0xfb120, 0xfb3f0, 0x15b6d0, 0x159180,
           0x159bd0, 0x139750, 0x14eae0, 0xb7e00, 0xb7f50, 0xb8040,
           0x239385, 0x23940f)


class NetgameTrace(InventoryTrace):
    def __init__(self, row):
        super().__init__()
        self.inventory_services()
        self.row = row
        self.entries = self.u(0x4f10b8) + 0x7dc
        self.scenario = self.u(0x4e7920)
        self.count_rows = self.u(self.scenario + 0x120)
        self.record = self.u(self.scenario + 0x124) + row*0x90
        self.samples = []
        self.w(POOL + 0x38, 4)
        self.action('equipment_initialize', 0x15c320,
                    registers=((UC_X86_REG_ESI, self.entries),))
        self.action('equipment_round_reset', 0x15c370,
                    registers=((UC_X86_REG_EDI, self.entries),))
        # Isolate one real authored row. Other rows retain the original NONE
        # datum and explicitly supplied never timers so no unrelated allocator
        # result is required during the bounded full-update routine.
        for n in range(self.count_rows):
            if n != row:
                self.sh(self.entries+n*8+4, -1)

    def row_state(self):
        p = self.entries + self.row*8
        return dict(object=f'{self.u(p):08x}',
                    timer=struct.unpack('<h', self.g.mem_read(p+4, 2))[0],
                    empty_counter=struct.unpack('<h', self.g.mem_read(p+6, 2))[0])

    def sample(self, name, tick):
        self.tick = tick
        self.w(TIME + 8, tick)
        before = self.row_state()
        self.action(name, 0x15c4b0, (self.entries,))
        result = dict(name=name, tick=tick, before=before, after=self.row_state())
        self.samples.append(result)
        return result


def listings(traces, output):
    result = []
    output.mkdir(parents=True, exist_ok=True)
    executed = set().union(*(t.executed for t in traces))
    t = traces[0]
    for entry in ENTRIES:
        routine = _trace(t.code, entry, t.starts | set(ENTRIES))
        ins = [t.code.at(a) for a in sorted(routine.code)]
        path = output / f'{entry:08x}.asm'
        path.write_text('\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'
                                  for i in ins) + '\n')
        result.append(dict(entry=f'{entry:08x}', listing=str(path.relative_to(ROOT)),
                           sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                           original_instruction_bytes_sha256=hashlib.sha256(
                               b''.join(i.bytes for i in ins)).hexdigest(),
                           instruction_count=len(ins),
                           executed_instruction_count=sum(i.address in executed for i in ins),
                           direct_calls=[dict(site=f'{s:08x}', target=f'{d:08x}')
                                         for s,d,k in routine.sites if k == 'call']))
    return result


def run_case(row, reason, option=0, fail=False):
    t = NetgameTrace(row)
    name = SMG if row == 12 else BR
    weapon = t.world_weapon(W2, SECOND, name)
    definition = int(weapon['datum'], 16)
    t.allocations = [(W2, definition)]
    initial = t.sample('initial_spawn_on_row_phase', row)
    assert initial['before']['timer'] == 1
    assert initial['after'] == dict(object=f'{W2:08x}', timer=0, empty_counter=0)
    assert t.u(SECOND + 0xaf, 1) == row
    assert t.u(SEED) == 0x12345678
    request = next(e for e in reversed(t.events) if e.get('service') == '000b8a30')
    assert request['request_position']['bits'] == t.vec(t.record + 0x40)['bits']
    # Allocator world-position materialization is outside this instruction
    # slice. Fixture supplies the exact authored position for subsequent timer
    # checks; no settling/collision result is claimed.
    t.g.mem_write(SECOND + 0x30, bytes(t.g.mem_read(t.record + 0x40, 12)))
    for sec in (1, 2):
        assert t.sample(f'unmoved_unowned_second_{sec}', row+sec*30)['after']['timer'] == 0
    if reason == 'carried':
        t.action('attach_real_world_identity', 0xce510, (UI, W2, 1))
        assert t.u(SECOND + 0x154) == UI
        assert t.u(UNIT + 0x218) == W2
    elif reason in ('at_threshold', 'beyond_threshold'):
        t.f(SECOND + 0x30, t.fv(SECOND + 0x30) + (1 if reason == 'at_threshold' else 1.125))
    elif reason == 'item_bit1':
        t.w(SECOND + 0x12c, 2)
    elif reason == 'missing':
        t.w(t.entries + row*8, -1)
    else:
        raise AssertionError(reason)
    t.g.mem_write(GAME + 0x211, bytes([option]))
    before_inventory = t.inventory_state()
    miss = t.sample('non_row_phase', row+3*30+1)
    assert miss['before'] == miss['after']
    first = t.sample('timer_start_on_row_phase', row+3*30)
    delay = 30 if row == 12 else 45
    delay = (-1 if option == 1 else delay >> 1 if option == 2 else delay << 1 if option == 3 else delay)
    if reason == 'at_threshold':
        assert first['after']['timer'] == 0
        expiry = None
    elif delay == -1:
        assert first['after']['timer'] == -1
        assert t.sample('never_timer_next_second', row+4*30)['after']['timer'] == -1
        expiry = None
    else:
        assert first['after']['timer'] == delay-1
        if not fail:
            t.world_weapon(W3, THIRD, name)
            t.allocations = [(W3, definition)]
        else:
            t.services[0xb8a30] = ('supplied allocation failure', 0xffffffff, 4)
        for visit in range(1, delay):
            expiry = t.sample(f'timer_visit_{visit}', row+(3+visit)*30)
            if visit < delay-1:
                assert expiry['after']['timer'] == delay-1-visit
                assert expiry['after']['object'] == (f'{W2:08x}' if reason != 'missing' else 'ffffffff')
        assert expiry['after'] == dict(object='ffffffff' if fail else f'{W3:08x}', timer=0, empty_counter=0)
        if reason != 'missing':
            assert t.u(SECOND + 0xaf, 1) == 255
        if reason == 'carried':
            assert t.u(SECOND + 0x154) == UI
            assert t.u(UNIT + 0x218) == W2
            assert t.inventory_state()['current'] == before_inventory['current']
        if not fail:
            assert t.u(THIRD + 0xaf, 1) == row
            assert t.u(THIRD + 0x154) == 0xffffffff
        if fail:
            retry = t.sample('allocation_failure_next_second', expiry['tick']+30)
            assert retry['after']['timer'] == delay-1
    assert t.u(SEED) == 0x12345678
    return t, dict(row=row, reason=reason, weapon_timer_option=option,
                   supplied_allocation_failure=fail, delay=delay,
                   first_spawn_tick=initial['tick'], first_timer_visit_tick=first['tick'],
                   expiry_tick=expiry['tick'] if expiry else None,
                   initial_request=request, samples=t.samples,
                   final_inventory=t.inventory_state(),
                   old_object_row=t.u(SECOND+0xaf, 1),
                   old_object_levitation_flag=t.u(SECOND+0xc1, 1),
                   rows=t.inventory_rows, events=t.events,
                   invoked_supplied_services=sorted({e['service'] for e in t.events if 'service' in e}))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=OUT/'report.json')
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    traces, cases = [], []
    specs = [(row, 'carried', option, False) for row in (0, 1, 12) for option in (0, 1, 2, 3)]
    specs += [(12, reason, 0, False) for reason in ('at_threshold', 'beyond_threshold', 'item_bit1', 'missing')]
    specs += [(12, 'carried', 0, True)]
    for spec in specs:
        trace, result = run_case(*spec)
        traces.append(trace)
        cases.append(result)
    t = traces[0]
    authored = t.cache.inspect_map()
    variants=[]
    for parameter,offset in ((0x53,0xd4),(0x57,0xd5)):
        entry=t.u(0x477d10+parameter*4)
        t.action(f'original_variant_descriptor_{parameter:02x}',entry,(SCRATCH+0x600,SCRATCH+0x604,SCRATCH+0x608))
        assert t.u(SCRATCH+0x600)==3 and t.u(SCRATCH+0x604)==offset and t.u(SCRATCH+0x608)==0x3f800000
        variants.append(dict(parameter=parameter,original_entry=f'{entry:08x}',descriptor_type=3,
                             offset=offset,options_offset=0x13c+offset,scale_bits='3f800000'))
    menu=Cache(ROOT/'assets/disc/maps/mainmenu.map')
    menu_data=menu.inspect_variants()
    report = dict(xbe_sha256=PIN, cache_sha256=CACHE_PIN,
                  oracle_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  inventory_tool_sha256=hashlib.sha256((ROOT/'tools/oracle_mp_inventory_1_1.py').read_bytes()).hexdigest(),
                  equipment=[dict(r, raw_bytes=bytes(t.cache.data[int(r['file_offset'],16):
                                                int(r['file_offset'],16)+0x90]).hex())
                             for r in authored['equipment'] if r['index'] in (0, 1, 12)],
                  variant_descriptors=variants,
                  variant_metadata=dict(cache_sha256=hashlib.file_digest(menu.stream,'sha256').hexdigest(),
                                        parameters={f'{p:02x}':menu_data['parameters'][p] for p in (0x53,0x57)}),
                  original_constants={f'{a:08x}': dict(bits=f'{t.u(a):08x}', value=t.fv(a))
                                      for a in (0x465030, 0x46516c, 0x44c5f0)},
                  cases=cases, instructions=listings(traces, args.output.parent/'instructions'),
                  fixture='Exact mapped Lockout metadata; synthetic salted object pool; original weapon constructor. '
                          'One real equipment row has its source reset timer; other rows have explicit never timers. '
                          'Allocation returns preconstructed objects and authored position is supplied for timer tests. '
                          'Mode 1 notification follows original local branch. Animation/world services are named in events.',
                  scope='Original complete equipment reset, spawn, delay, per-frame timer and identity-unbind instructions. '
                        'Physical allocation/settling, vehicle branches, nondefault map weapon sets and distributed transport excluded.')
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases), routine_rows=sum(len(t.inventory_rows) for t in traces),
                          samples=sum(len(t.samples) for t in traces))))


if __name__ == '__main__':
    main()
