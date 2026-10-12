#!/usr/bin/env python3
"""Bounded inventory research against untouched original TU1.1 instructions.

Engine services supplied at the fixture boundary are named in the output.
Neither this fixture nor its reports are an integrated original game runtime.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from oracle_smg_firing_1_1 import (
    Trace, ROOT, PIN, CACHE_PIN, Cache, UNIT, WEAPON, WI, UI, TAGS,
    GAME, SEED, TIME, HEADERS, BITMAP, STOP, STACK, POOL, PLAYERS,
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EBP,
)
from functions import _trace

OUT = ROOT / 'analysis/multiplayer/integration/inventory-source'
SCRATCH = 0x10e0000
SECOND = 0x10e2000
THIRD = 0x10e4000
W2, W3 = 0x10002, 0x10003


class InventoryTrace(Trace):
    def __init__(self):
        super().__init__()
        self.cache = Cache(ROOT / 'assets/disc/maps/lockout.map')
        self.inventory_rows = []
        self.executed = set()
        self.allocations = []
        self.w(0x4e7920, self.cache.scenario + self.cache.magic)
        matg = next(t for t in self.cache.tags.values() if t['group'] == 'matg')
        self.w(0x4e791c, matg['offset'] + self.cache.magic)
        # Original table stores engine objects at runtime, rather than in XBE data.
        # This explicit fixture provides only the Slayer mode-query leaf.
        self.w(0x566084, SCRATCH)
        self.w(SCRATCH, SCRATCH + 0x100)
        self.w(SCRATCH + 0x100, STOP + 0x100)
        self.services[STOP + 0x100] = ('supplied game engine mode: Slayer (2)', 2, 0)
        self.services[0xb8a30] = ('object allocator: supplied unavailable boundary', 0xffffffff, 4)
        self.w(UNIT + 0xd4, -1)
        self.w(UNIT + 0x13c, -1)
        self.w(WEAPON, int(self.authored['tag']['datum'], 16))
        self.w(UNIT + 0x14, -1)
        self.sh(UNIT + 0x1fc, -1)
        self.g.mem_write(UNIT + 0x212, b'\xff\xff')
        self.g.mem_write(UNIT + 0x216, b'\xff\xff')
        for off in range(0x218, 0x228, 4):
            self.w(UNIT + off, -1)
        self.w(UNIT + 0x710, -1)
        self.sh(UNIT + 0x714, -1)

    def inventory_services(self):
        self.services.update({
            0x111490: ('supplied animation state capability available', 1, 12),
            0xbdf10: ('world transformed bounds boundary', 0, 4),
            0xb9a60: ('world activation notification boundary', 0, 0),
            0xb81f0: ('world activation notification boundary', 0, 0),
            0xb8180: ('world activation notification boundary', 0, 0),
            0xfb560: ('world marker attachment boundary', 0, 8),
            0x170060: ('audio playback boundary', 0, 4),
            0x1d0a60: ('supplied runtime animation node unavailable', 0, 20),
            0x111500: ('unit animation transition boundary', 0, 8),
            0xefe30: ('unit animation action event boundary', 0, 0),
            0xefee0: ('supplied dual animation condition unavailable', 0, 4),
        })

    def world_weapon(self, index, address, name):
        definition = next(t for t in self.cache.tags.values()
                          if t['group'] == 'weap' and t['name'] == name)
        self.g.mem_write(address, bytes(0x800))
        self.w(address, int(definition['datum'], 16))
        for off in (0xc, 0x10, 0x14, 0xb4, 0x13c, 0x194, 0x24c, 0x154):
            self.w(address + off, -1)
        self.sh(address + 0x12a, -1)
        self.sh(address + 0x2c, -1)
        self.sh(address + 0x112, -1)
        self.w(address + 0xd4, -1)
        self.g.mem_write(address + 0xaa, b'\x02')
        self.f(address + 0xa0, 1)
        slot = index & 0xffff
        self.g.mem_write(HEADERS + slot*12, struct.pack('<HBBII', 1, 3, 2, 0, address))
        self.w(BITMAP, self.u(BITMAP) | 1 << slot)
        self.action(f'construct_{name}', 0x101bb0, (index,))
        return definition

    def hook(self, g, a, size, user):
        self.executed.add(a)
        if a == 0xb8a30 and self.allocations:
            index, definition = self.allocations.pop(0)
            sp = g.reg_read(UC_X86_REG_ESP)
            descriptor = self.u(sp+4)
            assert self.u(descriptor) == definition
            self.services[a] = ('supplied allocator returns preconstructed object; allocation internals excluded', index, 4)
        super().hook(g, a, size, user)

    def inventory_state(self):
        signed_byte = lambda a: struct.unpack('<b', self.g.mem_read(a, 1))[0]
        return dict(current=[signed_byte(UNIT + 0x212 + i) for i in range(2)],
                    requested=[signed_byte(UNIT + 0x216 + i) for i in range(2)],
                    slots=[f'{self.u(UNIT + 0x218 + i*4):08x}' for i in range(4)],
                    ready_ticks=[self.u(UNIT + 0x228 + i*4) for i in range(4)],
                    inventory_sequence=self.u(UNIT + 0x210, 2),
                    replicated_sequence=self.u(UNIT + 0x214, 2),
                    grenade_current=signed_byte(UNIT + 0x23c),
                    grenade_requested=signed_byte(UNIT + 0x23d),
                    grenade_counts=[self.u(UNIT + 0x23e + i, 1) for i in range(2)],
                    action_bits=[f'{self.u(UNIT + 0x704 + i*4):08x}' for i in range(2)],
                    seed=f'{self.u(SEED):08x}',
                    weapons=[dict(index=f'{idx:08x}', definition=f'{self.u(addr):08x}',
                                  owner=f'{self.u(addr+0x154):08x}', flags=f'{self.u(addr+0x12c):08x}',
                                  reserve=self.u(addr+0x22a,2), loaded=self.u(addr+0x22c,2),
                                  linear_velocity=self.vec(addr+0x88),
                                  angular_velocity=self.vec(addr+0x94))
                             for idx,addr in ((WI,WEAPON),(W2,SECOND),(W3,THIRD))
                             if self.u(HEADERS+(idx&0xffff)*12+8) == addr])

    def action(self, name, address, args=(), registers=()):
        self.active = name
        before = self.inventory_state()
        event_start = len(self.events)
        self.run(address, args, registers)
        row = dict(name=name, original_entry=f'{address:08x}', before=before,
                   after=self.inventory_state(), eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',
                   event_range=[event_start, len(self.events)])
        self.inventory_rows.append(row)
        return row

    def fragment(self, name, start, stop, registers=()):
        self.active = name
        before = self.inventory_state()
        event_start = len(self.events)
        self.count = 0
        self.g.reg_write(UC_X86_REG_ESP, STACK)
        for reg, value in registers:
            self.g.reg_write(reg, value)
        self.g.emu_start(start, stop, count=100000)
        assert self.g.reg_read(UC_X86_REG_EIP) == stop
        row = dict(name=name, original_fragment=[f'{start:08x}', f'{stop:08x}'],
                   stop_is_exclusive=True, before=before, after=self.inventory_state(),
                   event_range=[event_start, len(self.events)])
        self.inventory_rows.append(row)
        return row


def placements(cache):
    result = cache.inspect_map()
    return {k: result[k] for k in ('map', 'sha256', 'scenario_file_offset',
                                  'equipment', 'starting_equipment')}


def dump_instructions(trace, directory):
    entries = (0x1a28a0, 0x1a2700, 0x1a2a90, 0x1a2b00, 0x15b6d0,
               0x101bb0, 0xcea10, 0xcea70, 0xcd1a0, 0xcd2b0, 0xcf300,
               0xceaf0, 0xcec00, 0xce510, 0xcfed0, 0xcfe60, 0xcfd80,
               0xcf8c0, 0xcfb00, 0xce440, 0xce370, 0xe7f30, 0xe7ef0,
               0xe9fe0, 0xea180, 0xea270, 0xea370, 0xea440, 0xea660,
               0xe9a90, 0xe9b40, 0xe9c50, 0xe9d50, 0xd1d20,
               0xfb120, 0xfb3f0, 0xfb360, 0x1058f0, 0x105ad0,
               0x10a1d0, 0x10a240, 0xf99a0, 0xfb620, 0xbb0c0, 0x147a90, 0x15ffd0,
               0xc52c0, 0x1884e0, 0xc8140, 0xc6c50, 0xcd310, 0xcf440,
               0x112270, 0xc8410, 0xc83d0, 0x15a460, 0x1a2ba0, 0x16d680,
               0x159bd0, 0x190a30, 0x190130, 0x1902e0, 0x158ba0, 0x158c10,
               0x151c90, 0x151aa0, 0xc8250, 0xccc60, 0x110800,
               0x110a20, 0x109a60, 0xe8a70, 0xe8740, 0xe85c0,
               0x239436, 0x239457, 0x23947e)
    directory.mkdir(parents=True, exist_ok=True)
    rows = []
    for entry in entries:
        routine = _trace(trace.code, entry, trace.starts | set(entries))
        path = directory / f'{entry:08x}.asm'
        code = [trace.code.at(a) for a in sorted(routine.code)]
        path.write_text('\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'
                                  for i in code) + '\n')
        rows.append(dict(entry=f'{entry:08x}', listing=str(path.relative_to(ROOT)),
                         sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                         original_instruction_bytes_sha256=hashlib.sha256(
                             b''.join(i.bytes for i in code)).hexdigest(),
                         instruction_count=len(code),
                         executed_instruction_count=sum(i.address in trace.executed for i in code),
                         direct_calls=[dict(site=f'{s:08x}', target=f'{t:08x}')
                                       for s,t,k in routine.sites if k == 'call']))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT / 'report.json')
    parser.add_argument('--experiment', choices=('loadout', 'loadout-created', 'slots', 'pickup', 'actions', 'ammo', 'maps', 'grenades', 'choices', 'grenade-selection', 'player-cap', 'grenade-gate', 'variant-fields'), default='loadout')
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    t = InventoryTrace()
    authored = placements(t.cache)
    if args.experiment in ('loadout', 'loadout-created'):
        if args.experiment == 'loadout-created':
            t.inventory_services()
            definition = t.world_weapon(WI, WEAPON, 'objects\\weapons\\rifle\\smg\\smg')
            t.allocations = [(WI,int(definition['datum'],16))]
        t.w(SCRATCH + 0x200, 2)
        t.w(SCRATCH + 0x204, 0)
        t.action('default_loadout_with_supplied_created_object' if args.experiment == 'loadout-created'
                 else 'default_loadout_with_allocator_failure', 0x1a28a0,
                 (UI, SCRATCH + 0x200, SCRATCH + 0x204))
        result = dict(grenades=[t.u(SCRATCH + 0x200), t.u(SCRATCH + 0x204)])
        if args.experiment == 'loadout-created':
            assert t.inventory_state()['current'] == [0,-1]
            assert t.u(WEAPON+0x22c,2) == 60 and t.u(WEAPON+0x22a,2) == 120
    elif args.experiment == 'slots':
        result = {}
        for occupied in range(5):
            for i in range(4):
                t.w(UNIT + 0x218 + i*4, WI if i < occupied else -1)
            row = t.action(f'first_empty_slot_{occupied}_occupied', 0xcea70,
                           registers=((UC_X86_REG_EAX, UI),))
            assert t.g.reg_read(UC_X86_REG_EAX) & 0xffff == (occupied if occupied < 4 else 0xffff)
    elif args.experiment in ('pickup', 'actions'):
        t.inventory_services()
        t.world_weapon(WI, WEAPON, 'objects\\weapons\\rifle\\smg\\smg')
        t.world_weapon(W2, SECOND, 'objects\\weapons\\rifle\\battle_rifle\\battle_rifle')
        t.action('pickup_smg_into_empty_mode1', 0xce510, (UI, WI, 1))
        t.action('pickup_battle_rifle_carried_mode0', 0xce510, (UI, W2, 0))
        if args.experiment == 'actions':
            t.g.mem_write(UNIT+0x216, b'\x01')
            t.w(SCRATCH+0x300,8)
            t.w(SCRATCH+0x304,0)
            t.services[0x115650] = ('supplied put-away animation accepted; caller must wait', 1, 16)
            row = t.action('switch_begin_with_put_away_animation', 0xe9fe0, (UI, SCRATCH+0x300))
            assert row['after']['current'] == [0,-1]
            assert t.u(UNIT+0x704) & 0x100
            t.services[0x115650] = ('supplied put-away animation completed/unavailable; original fallback', 0, 16)
            t.w(TIME+8,47)
            row = t.action('switch_update_after_animation_gate_clears', 0xea180, (UI,8))
            assert row['after']['current'] == [1,-1]
            assert row['after']['ready_ticks'][1] == 47
            # Direct update callback returns finished=0; the enclosing action updater
            # owns clearing its active bit. Do not invent that outer callback here.
            t.w(SCRATCH+0x300,9)
            row = t.action('drop_primary_with_return_to_smg', 0xea440, (UI,SCRATCH+0x300))
            assert row['after']['slots'] == [f'{WI:08x}','ffffffff','ffffffff','ffffffff']
            assert row['after']['current'] == [0,-1]
            assert t.u(SECOND+0x154) == 0xffffffff
            assert row['after']['seed'] == '4b5c952c'
            t.w(SCRATCH+0x300,20)
            t.w(SCRATCH+0x304,W2)
            t.sh(SCRATCH+0x308,3)
            row = t.action('request_type20_pickup_dropped_battle_rifle_mode3', 0xea660,
                           (UI,SCRATCH+0x300))
            assert t.g.reg_read(UC_X86_REG_EAX) & 0xff == 1
        result = {}
    elif args.experiment == 'ammo':
        t.inventory_services()
        t.world_weapon(WI, WEAPON, 'objects\\weapons\\rifle\\smg\\smg')
        t.world_weapon(W2, SECOND, 'objects\\weapons\\rifle\\smg\\smg')
        result = []
        for target_reserve, source_reserve, source_loaded in ((120,120,60), (170,20,50), (180,20,60), (0,0,60)):
            t.sh(WEAPON+0x22a,target_reserve)
            t.sh(WEAPON+0x22c,17)
            t.sh(SECOND+0x22a,source_reserve)
            t.sh(SECOND+0x22c,source_loaded)
            row = t.action(f'merge_ammo_{target_reserve}_{source_reserve}_{source_loaded}',
                           0x1058f0, registers=((UC_X86_REG_EAX,WI),(UC_X86_REG_ECX,W2)))
            weapons=row['after']['weapons']
            assert weapons[0]['loaded']==17
            assert weapons[0]['reserve']==min(180,target_reserve+source_reserve+source_loaded)
            assert weapons[0]['reserve']+weapons[1]['reserve']+weapons[1]['loaded']==target_reserve+source_reserve+source_loaded
            assert row['before']['seed']==row['after']['seed']
            result.append(dict(target_reserve=target_reserve, source_reserve=source_reserve,
                               source_loaded=source_loaded, after=weapons))
    elif args.experiment == 'grenades':
        result=[]
        start=t.cache.block(t.cache.scenario+0x128,0x9c)[0]+t.cache.magic
        for scenario_flags in (0,1,2):
            for enabled in (False,True):
                for maximum_one in (False,True):
                    for weapon_set in (0,15,16,17):
                        t.w(start,scenario_flags)
                        t.w(GAME+0x184,0x800 if enabled else 0)
                        t.g.mem_write(GAME+0x210,bytes([weapon_set]))
                        t.g.mem_write(t.u(0x4f10b8), bytes([8 if maximum_one else 0]))
                        row=t.action(f'grenades_flags{scenario_flags}_enabled{int(enabled)}_cap{1 if maximum_one else 2}_set{weapon_set}',
                                     0x15ffd0,registers=((UC_X86_REG_ECX,UI),))
                        counts=[t.u(UNIT+0x23e,1),t.u(UNIT+0x23f,1)]
                        result.append(dict(supplied_scenario_flags=scenario_flags, starting_grenades=enabled,
                                           supplied_maximum_one=maximum_one, supplied_weapon_set=weapon_set,counts=counts))
                        cap=1 if maximum_one else 2
                        expected=[0,0] if not enabled or scenario_flags&1 else ([0,cap] if scenario_flags&2 else [cap,0])
                        if weapon_set in (15,17):expected=[0,min(cap,sum(expected))]
                        if weapon_set==16:expected=[min(cap,sum(expected)),0]
                        assert counts==expected
    elif args.experiment == 'grenade-selection':
        result=[]
        paths=[ROOT/'assets/disc/maps/lockout.map']
        for name in ('gemini', 'backwash'):
            paths += list((ROOT/'assets/multiplayer-map-pack/demos/dlc').glob(f'*/{name}.map'))
        assert len(paths)==3
        start=t.cache.block(t.cache.scenario+0x128,0x9c)[0]+t.cache.magic
        t.w(GAME+0x184,0x800)
        t.g.mem_write(GAME+0x210,b'\0')
        t.g.mem_write(t.u(0x4f10b8),b'\0')
        for path in paths:
            cache=Cache(path)
            source=placements(cache)
            flags=source['starting_equipment'][0]['flags']
            t.w(start,flags)
            t.w(STACK,STOP);t.w(STACK+4,UI)
            row=t.fragment(f'{path.stem}_unit_ctor_inventory_prefix',0xc52c0,0xc5359)
            assert row['after']['grenade_current']==-1 and row['after']['grenade_requested']==-1
            row=t.action(f'{path.stem}_starting_grenade_counts',0x15ffd0,registers=((UC_X86_REG_ECX,UI),))
            assert row['after']['grenade_current']==-1 and row['after']['grenade_requested']==-1
            initial=row['after']
            t.sh(SCRATCH+0x42c,-1)
            t.w(STACK+0x70,UNIT);t.w(STACK+0x34,0)
            row=t.fragment(f'{path.stem}_player_control_grenade_selection',0x188676,0x18871d,
                           ((UC_X86_REG_EBP,SCRATCH+0x400),(UC_X86_REG_EDI,UI),(UC_X86_REG_EBX,POOL)))
            selected=struct.unpack('<h',t.g.mem_read(SCRATCH+0x42c,2))[0]
            expected=0 if flags==0 else 1 if flags==2 else -1
            assert selected==expected
            assert row['after']['grenade_current']==-1 and row['after']['grenade_requested']==-1
            t.g.mem_write(SCRATCH+0x500,bytes(0x80))
            t.w(SCRATCH+0x428,t.u(UNIT+0x214))
            row=t.fragment(f'{path.stem}_marshal_player_control',0x188a4d,0x188a94,
                           ((UC_X86_REG_EBP,SCRATCH+0x400),(UC_X86_REG_ESI,SCRATCH+0x500)))
            assert struct.unpack('<h',t.g.mem_read(SCRATCH+0x522,2))[0]==selected
            row=t.action(f'{path.stem}_unit_apply_selected_control',0x151c90,(UI,SCRATCH+0x500),
                         registers=((UC_X86_REG_EAX,0x10000),))
            assert row['after']['grenade_requested']==selected and row['after']['grenade_current']==-1
            t.w(STACK+0x3c,UI)
            row=t.fragment(f'{path.stem}_unit_update_grenade_selection',0xc6e19,0xc6e54,
                           ((UC_X86_REG_EBP,UNIT),))
            assert row['after']['grenade_current']==selected
            result.append(dict(map=source['map'],sha256=source['sha256'],authored_starting_flags=flags,
                               initial=initial,control_grenade_index=selected,after=row['after'],
                               fixture='Lockout metadata with source-read starting-equipment flags from named cache; '
                                       'allocator failure. Control selection, control marshalling and unit-update '
                                       'fragments are bounded. Full151C90 executes conversion and C8140 controlapply; '
                                       'no network/control-delivery scheduling is asserted.'))
            cache.data.close();cache.stream.close()
    elif args.experiment == 'variant-fields':
        result=[]
        for parameter,expected_type,expected_offset,expected_bits in ((0x58,1,0x48,11),(0x59,3,0xd6,0x3f800000),(0x5e,3,0xd7,0x3f800000)):
            descriptor=t.u(0x477d10+parameter*4)
            t.action(f'variant_descriptor_{parameter:02x}',descriptor,(SCRATCH+0x700,SCRATCH+0x704,SCRATCH+0x708))
            type_id,offset,bits=[t.u(SCRATCH+0x700+i*4) for i in range(3)]
            assert (type_id,offset,bits)==(expected_type,expected_offset,expected_bits)
            result.append(dict(parameter=f'{parameter:02x}',descriptor=f'{descriptor:08x}',type=type_id,
                               variant_offset=f'{offset:02x}',field_param_bits=f'{bits:08x}'))
        authored['variant_descriptor_table']='00477d10'
    elif args.experiment == 'grenade-gate':
        t.inventory_services()
        authored['grenade_gate_forced_names']=dict(original_table='0046e8d0',
            values=[f'{v:08x}' for v in struct.unpack('<5I',t.g.mem_read(0x46e8d0,20))])
        t.world_weapon(WI,WEAPON,'objects\\weapons\\rifle\\smg\\smg')
        t.sh(UNIT+0x12a,0x800);t.sh(UNIT+0x33e,0x900)
        t.w(UNIT+0x218,WI);t.g.mem_write(UNIT+0x212,b'\x00\xff')
        t.action('unit_control_reset_original',0x110800,registers=((UC_X86_REG_EAX,UI),))
        assert t.u(UNIT+0x9a4)==0xffffffff
        phases=(('idle',0,0x400000c,60),('held_fire',1,0x6000006,59),
                ('full_reload',5,0x8000002,17),('empty_reload',5,0x8000002,0))
        result=[]
        for name,weapon_selector,animation_name,loaded in phases:
            t.sh(WEAPON+0x12a,0x600)
            t.w(STACK+0x1c,weapon_selector)
            t.fragment(f'{name}_original_weapon_action_name',0x109ad5,0x109b2a,
                       ((UC_X86_REG_ESI,WEAPON),))
            assert t.g.reg_read(UC_X86_REG_ESI)==animation_name
            for controller_present in (False,True):
                for weapon_status in (-1,0,1):
                    for interruptible in (False,True):
                        for overlay in (0xffffffff,animation_name):
                            t.w(UNIT+0x13c,0x10000 if controller_present else -1)
                            t.w(UNIT+0x87c,animation_name);t.w(UNIT+0x9a4,overlay)
                            t.g.mem_write(UNIT+0x814,bytes([0x10 if interruptible else 0]))
                            t.w(WEAPON+0x178,weapon_selector)
                            t.sh(WEAPON+0x17c,weapon_status);t.sh(WEAPON+0x22c,loaded)
                            t.active=f'gate_{name}'
                            t.run(0x112270,(UI,))
                            blocked=t.g.reg_read(UC_X86_REG_EAX)&0xff
                            assert blocked==0
                            t.g.mem_write(UNIT+0x23c,b'\xff\x01\x00\x02')
                            t.w(STACK+0x3c,UI)
                            row=t.fragment(f'{name}_selection_c{int(controller_present)}_status{weapon_status}_interrupt{int(interruptible)}_overlay{overlay:08x}',
                                           0xc6e19,0xc6e54,((UC_X86_REG_EBP,UNIT),))
                            assert row['after']['grenade_current']==1
                            result.append(dict(phase=name,weapon_selector=weapon_selector,loaded=loaded,
                                               animation_name=f'{animation_name:08x}',overlay=f'{overlay:08x}',
                                               controller_present=controller_present,weapon_status_17c=weapon_status,
                                               interruptible=interruptible,blocked=bool(blocked),current=1))
        for name,animation_name,overlay in (('enter',0x5000049,0xffffffff),
                                           ('melee',0x500000a,0xffffffff),
                                           ('melee_overlay_over_idle',0x400000c,0x500000a)):
            t.w(UNIT+0x13c,0x10000);t.sh(WEAPON+0x17c,1)
            t.w(UNIT+0x87c,animation_name);t.w(UNIT+0x9a4,overlay)
            t.g.mem_write(UNIT+0x814,b'\0');t.active=f'gate_positive_{name}'
            t.run(0x112270,(UI,))
            assert t.g.reg_read(UC_X86_REG_EAX)&0xff==1
            t.g.mem_write(UNIT+0x23c,b'\xff\x01\x00\x02');t.w(STACK+0x3c,UI)
            row=t.fragment(f'{name}_selection_deferred',0xc6e19,0xc6e54,((UC_X86_REG_EBP,UNIT),))
            assert row['after']['grenade_current']==-1
            result.append(dict(phase=name,animation_name=f'{animation_name:08x}',overlay=f'{overlay:08x}',blocked=True,current=-1))
    elif args.experiment == 'player-cap':
        result=[]
        player_data=SCRATCH+0x6000
        player_bitmap=SCRATCH+0x600
        t.w(PLAYERS+0x20,16);t.w(PLAYERS+0x24,0x21c)
        t.w(PLAYERS+0x44,player_data);t.w(PLAYERS+0x48,player_bitmap)
        for count,excluded,preexisting in [(n,0,0) for n in range(17)]+[(9,1,0),(16,8,0),(2,0,8)]:
            t.w(PLAYERS+0x38,count);t.w(player_bitmap,(1<<count)-1)
            t.g.mem_write(player_data,bytes(16*0x21c))
            for i in range(count):
                t.sh(player_data+i*0x21c,1)
                t.g.mem_write(player_data+i*0x21c+2,bytes([2 if i<excluded else 0]))
            t.w(t.u(0x4f10b8),preexisting)
            row=t.action(f'roster_flags_{count}_excluded_{excluded}_existing_{preexisting}',0x15a460)
            eligible=count-excluded
            flags=t.u(t.u(0x4f10b8))
            assert flags==preexisting | (2 if eligible>6 else 0) | (4 if eligible>=5 else 0) | (8 if eligible>=9 else 0)
            result.append(dict(allocated_count=count,excluded_flag_bit1_count=excluded,
                               eligible_count=eligible,flags_before=preexisting,flags_after=flags))
        runtime_globals=t.u(0x4f10b8)
        t.fragment('runtime_globals_lifecycle_reset_prefix',0x158c10,0x158c25)
        assert t.u(runtime_globals)==0
        authored['engine_globals_lifecycle_entry22']=[f'{v:08x}' for v in struct.unpack('<9I',t.g.mem_read(0x447430+22*0x24,0x24))]
        authored['cap_flag_callers']=[dict(site='00159de3',operation='tail jump',target='0015a460'),
                                     dict(site='00190a8e',operation='call',target='00159bd0'),
                                     dict(site='0019019a',operation='call',target='00190a30'),
                                     dict(site='00190345',operation='call',target='00190a30')]
        for caller in authored['cap_flag_callers']:
            ins=t.code.at(int(caller['site'],16))
            assert ins.mnemonic==('jmp' if caller['operation']=='tail jump' else 'call')
            assert ins.operands[0].imm==int(caller['target'],16)
        assert authored['engine_globals_lifecycle_entry22'][0]=='00158c10'
    elif args.experiment == 'choices':
        matg=next(tag for tag in t.cache.tags.values() if tag['group']=='matg')
        mulg=t.cache.tags[t.cache.u(matg['offset']+0x16c)]
        runtime=t.cache.block(mulg['offset']+8,0x568)[0]
        weapons=t.cache.block(runtime+0x68,8)
        defaults=int(t.authored['tag']['datum'],16)
        permutation=(3,0,1,6,7,5,2,4,12,9,10,13,11,15,14,8,16,17)
        result=[]
        for choice in range(21):
            row=t.action(f'weapon_choice_{choice}',0x1a2700,(defaults,choice,0xffffffff))
            selected=t.g.reg_read(UC_X86_REG_EAX)
            if choice==0:assert selected==defaults
            if choice==1:assert selected==0xffffffff
            if choice>=3:assert selected==t.cache.u(weapons[permutation[choice-3]]+4)
            result.append(dict(choice=choice, runtime_index=permutation[choice-3] if choice>=3 else None,
                               selected=t.cache.reference(selected)))
        random_allowed={t.cache.u(weapons[index]+4) for index in permutation[:16]}-{defaults,0xffffffff}
        for case in range(128):
            row=t.action(f'random_weapon_excluding_smg_{case}',0x1a2700,(defaults,2,defaults))
            selected=t.g.reg_read(UC_X86_REG_EAX)
            assert selected in random_allowed
        authored['multiplayer_weapon_references']=[dict(index=i,reference=t.cache.reference(t.cache.u(p+4)))
                                                  for i,p in enumerate(weapons)]
    else:
        paths = [ROOT/'assets/disc/maps'/f'{name}.map' for name in
                 ('ascension','beavercreek','burial_mounds','coagulation','colossus','cyclotron',
                  'foundation','headlong','lockout','midship','waterworks','zanzibar')]
        paths += sorted((ROOT/'assets/multiplayer-map-pack/demos/dlc').glob('*/*.map'))
        result = []
        for path in paths:
            cache = Cache(path)
            rows = placements(cache)
            rows['path'] = str(path.relative_to(ROOT))
            result.append(rows)
            cache.data.close()
            cache.stream.close()
        assert len(result)==21
    instructions = dump_instructions(t, args.output.parent / 'instructions')
    report = dict(xbe_sha256=PIN, cache_sha256=CACHE_PIN,
                  oracle_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  authored=authored, experiment=args.experiment, result=result,
                  rows=t.inventory_rows, events=t.events, instructions=instructions,
                  invoked_supplied_services=sorted({e['service'] for e in t.events if 'service' in e}),
                  fixture='Synthetic object/header buffers and unit action storage, original Lockout tag bytes. '
                          'Unowned object links are NONE; raw object fields are zero unless explicitly initialized. '
                          'Weapon initialization is original 00101BB0. Unit aim is (1,0,0), unit linear velocity zero. '
                          'Starting grenade experiment mutates mapped scenario flags and option bytes as labeled fixture inputs.',
                  supplied_services={f'{a:08x}':dict(description=n, returned_value=f'{r:08x}',
                                                        stack_cleanup=p)
                                     for a,(n,r,p) in t.services.items()},
                  scope='Untouched original instruction slice; explicit fixture services. '
                        'Successful loadout allocation is supplied as an already original-initialized fixture object. '
                        'Actual allocator, animation timeline, model marker attachment and world services are excluded.')
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(experiment=args.experiment, rows=len(t.inventory_rows), events=len(t.events),
                          result_count=len(result))))


if __name__ == '__main__':
    main()
