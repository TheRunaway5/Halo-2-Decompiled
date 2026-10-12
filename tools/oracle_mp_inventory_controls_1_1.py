#!/usr/bin/env python3
"""Bounded original TU1.1 pickup controls and player/unit phase research.

XInput samples, device/profile selection, spatial enumeration, visibility and
lower animation/physics services are explicit fixture inputs. Original input
polling, profile mapping, flag generation, player-phase admission, action
dispatch, salted allocation and inventory updates execute unchanged.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from oracle_mp_inventory_1_1 import (
    ROOT, PIN, CACHE_PIN, SCRATCH, SECOND, THIRD, W2, W3, UNIT, UI,
    WEAPON, WI, POOL, HEADERS, SEED, STACK, STOP, GAME, _trace,
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP,
)
from oracle_smg_firing_1_1 import PLAYER_DATA, PLAYERS, USERS, TIME, UC_X86_REG_EBP
from oracle_mp_inventory_target_1_1 import TargetTrace, SMG, BR
from oracle_mp_inventory_world_1_1 import WorldTrace
from inspect_multiplayer import Cache, MAPS

OUT = ROOT / 'analysis/multiplayer/integration/inventory-source/controls'
CONTROL = SCRATCH + 0x1000
RAW = 0x4ed7ac
LOGICAL = 0x526318
PROFILE = 0x5262a8
ACTION = SCRATCH + 0x900
FRAME = SCRATCH + 0xb00
PLAYER_BITMAP = SCRATCH + 0x1400
VIEW = SCRATCH + 0x1800
ENTRIES = (0x126350, 0x1260a0, 0x12cbc0, 0x21a530, 0x192666,
           0x219d50, 0x187610, 0x189560, 0x1884e0, 0x188ad0,
           0x14a759, 0x12d600, 0x14eae0, 0x14e780, 0x14e7e0,
           0x150dd0, 0x1ea490, 0x1ec670, 0x150f00, 0x151c90,
           0xc8140, 0xc6c50, 0xcf300, 0xcec00, 0x153360, 0x1533e0,
           0x153090, 0xe7f30, 0xe7ef0, 0xea660, 0xce510,
           0xe9fe0, 0xea180, 0xe9b40, 0x112270, 0xcfed0,
           0xcfd80, 0x15c110, 0x15c4b0, 0xb8a30, 0x16d2c0, 0x101bb0,
           0x2529b7, 0x19316e, 0x252f4d, 0x19399c, 0x252cf2,
           0x1045e0, 0x1021d0, 0x105640, 0x1019f0, 0x106c30)


class ControlsMixin:
    def finish_returns(self, address):
        for ret, record in self.control_returns[:]:
            if address == ret:
                record['return_eax'] = f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
                if '_out' in record:
                    out, length = record.pop('_out')
                    record['output_hex'] = bytes(self.g.mem_read(out, length)).hex()
                self.control_returns.remove((ret, record))

    def run(self, address, argv=(), regs=()):
        super().run(address, argv, regs)
        if getattr(self, 'controls_ready', False):
            self.finish_returns(STOP)

    def setup_controls(self, layout=0):
        self.control_calls = []
        self.control_returns = []
        self.samples = []
        self.xinput_sample = bytes(22)
        self.w(0x4f4854, CONTROL)
        self.g.mem_write(CONTROL, bytes(0x300))
        self.w(CONTROL+0x14, UI)
        for off in (0x2a, 0x2c, 0x2e):
            self.sh(CONTROL+0x14+off, -1)
        self.w(PLAYER_DATA+0x24, 0)
        self.sh(PLAYER_DATA+0x28, 0)
        for n in range(4):
            self.w(0x4ed79c+n*4, int(n == 0))
        self.g.mem_write(RAW, bytes(0x48))
        self.w(0x526250, VIEW)
        # Empty HUD backing storage is an explicit lifecycle fixture. Original
        # 252F4D marks each message slot unused; drop's original 252CF2 still
        # executes and sees those empty slots. No HUD text/display parity claim.
        self.w(0x509bd8, SCRATCH+0x8000)
        self.fragment('original_zero_hud_backing_storage', 0x1931b6, 0x1931bf,
                      ((UC_X86_REG_EDI, SCRATCH+0x8000),))
        self.action('original_clear_hud_message_slots_in_supplied_storage', 0x252f4d)
        self.g.mem_write(USERS+0xc, struct.pack('<4I', 0, 0xffffffff, 0xffffffff, 0xffffffff))
        self.g.mem_write(0x556548, bytes([layout]))
        self.action('original_profile_button_mapping', 0x192666,
                    registers=((UC_X86_REG_EAX, 0),))
        self.profile_hex = bytes(self.g.mem_read(PROFILE, 28)).hex()
        self.w(PLAYERS+0x24, 0x21c)
        self.w(PLAYERS+0x38, 1)
        self.w(PLAYERS+0x48, PLAYER_BITMAP)
        self.w(PLAYER_BITMAP, 1)
        self.sh(PLAYER_DATA, 1)
        self.sh(PLAYER_DATA+2, 1)
        # Runtime controlled-unit admission input to original player phase.
        # The full biped constructor/lifecycle remains outside this experiment.
        self.w(UNIT+0x134, self.u(UNIT+0x134) | 1)
        self.services[0x4101ac] = ('supplied Xbox XInputGetState sample', 0, 8)
        self.controls_ready = True

    def inventory_state(self):
        result = super().inventory_state()
        if not hasattr(self, 'g'):
            return result
        # Report the source allocator's salt, rather than fixture constants.
        result['weapons'] = []
        for n in range(self.u(POOL+0x38)):
            h = HEADERS+n*12
            if self.u(h+3, 1) != 2 or not self.u(h+2, 1):
                continue
            p = self.u(h+8)
            if not p:
                continue
            datum = self.u(h, 2) << 16 | n
            result['weapons'].append(dict(
                index=f'{datum:08x}', definition=f'{self.u(p):08x}',
                owner=f'{self.u(p+0x154):08x}', flags=f'{self.u(p+0x12c):08x}',
                reserve=self.u(p+0x22a, 2), loaded=self.u(p+0x22c, 2),
                heat_bits=f'{self.u(p+0x184):08x}',
                row_index=self.u(p+0xaf, 1), linear_velocity=self.vec(p+0x88),
                angular_velocity=self.vec(p+0x94)))
        return result

    def hook(self, g, a, size, user):
        if getattr(self, 'controls_ready', False):
            self.finish_returns(a)
            sp = g.reg_read(UC_X86_REG_ESP)
            if a == 0x4101ac:
                assert self.u(sp+4) == 1
                output = self.u(sp+8)
                g.mem_write(output, self.xinput_sample)
                self.control_calls.append(dict(phase=self.active, supplied=True,
                    entry='004101ac', sample_hex=self.xinput_sample.hex(),
                    description='supplied original Xbox hardware packet, not native binding'))
            if a in (0x187610, 0x1884e0, 0x14e7e0, 0x150dd0,
                     0x1ea490, 0x150f00, 0x151c90, 0xc8140,
                     0xcec00, 0xce510, 0xcf300, 0xe7f30, 0xe7ef0):
                record = dict(phase=self.active, entry=f'{a:08x}',
                              registers=self.registers())
                if a == 0x187610:
                    record['_out'] = (self.u(sp+20), 0x4c)
                elif a == 0x1884e0:
                    record['_out'] = (self.u(sp+16), 0x5c)
                elif a in (0x14e7e0, 0x150dd0):
                    record['_out'] = (g.reg_read(UC_X86_REG_ECX), 0x5c)
                elif a in (0x1ea490, 0x151c90):
                    record['_out'] = (self.u(sp+4 if a == 0x1ea490 else sp+8), 0x5c)
                elif a == 0x150f00:
                    record['_out'] = (self.u(sp+12), 0x5c)
                    record['player'] = f'{self.u(sp+4):08x}'
                    record['unit'] = f'{self.u(sp+8):08x}'
                elif a == 0xc8140:
                    p = g.reg_read(UC_X86_REG_ECX)
                    record['incoming_sequence'] = self.u(p+6, 2)
                    record['unit_sequence'] = self.u(UNIT+0x214, 2)
                    record['incoming_slots'] = list(g.mem_read(p+8, 2))
                elif a == 0xcec00:
                    record['unit'] = f'{self.u(sp+4):08x}'
                    record['weapon'] = f'{self.u(sp+8):08x}'
                    record['_out'] = (self.u(sp+12), 4)
                elif a == 0xce510:
                    record['unit'] = f'{self.u(sp+4):08x}'
                    record['weapon'] = f'{self.u(sp+8):08x}'
                    record['mode'] = self.u(sp+12, 2)
                elif a == 0xe7f30:
                    p = g.reg_read(UC_X86_REG_EDI)
                    record['request_hex'] = bytes(g.mem_read(p, 32)).hex()
                    record['type'] = self.u(p)
                    record['weapon_or_payload'] = f'{self.u(p+4):08x}'
                    record['mode'] = self.u(p+8, 2)
                elif a == 0xe7ef0:
                    record['type'] = g.reg_read(UC_X86_REG_ECX)
                self.control_calls.append(record)
                self.control_returns.append((self.u(sp), record))
        super().hook(g, a, size, user)

    def generated_action(self, name):
        self.action(name+'_profile_map', 0x219d50)
        self.action(name+'_full_control_generation', 0x1884e0,
                    (0, 0x3d088889, 0x3d088889, ACTION))
        return dict(flags=self.u(ACTION+0x1c, 2),
                    sequence=self.u(ACTION+0x1e, 2),
                    slots=list(self.g.mem_read(ACTION+0x20, 2)),
                    candidate_hex=bytes(self.g.mem_read(ACTION+0x28, 12)).hex(),
                    logical_hex=bytes(self.g.mem_read(LOGICAL, 0x50)).hex(),
                    action_hex=bytes(self.g.mem_read(ACTION, 0x5c)).hex())

    def player_phase(self, name, frame_mask=1):
        self.w(FRAME+8, frame_mask)
        self.g.mem_write(FRAME+12, bytes(self.g.mem_read(ACTION, 0x5c)))
        self.w(STACK+0x90, FRAME)
        self.fragment(name, 0x14ed0c, 0x14ed4f,
                      ((UC_X86_REG_EBX, PLAYERS), (UC_X86_REG_EBP, POOL)))
        return dict(flags=self.u(STACK+0x30+0x1c, 2),
                    consumed_player_flags=self.u(PLAYER_DATA+2, 2),
                    previous_buttons=self.u(PLAYER_DATA+0x3c, 2),
                    view_cache_hex=bytes(self.g.mem_read(VIEW+0x88, 0x5c)).hex(),
                    processed_copy_hex=bytes(self.g.mem_read(STACK+0x30, 0x5c)).hex(),
                    frame_action_hex=bytes(self.g.mem_read(FRAME+12, 0x5c)).hex())

    def poll(self, name, analog=None, digital=0, milliseconds=33, process=True):
        sample = bytearray(22)
        struct.pack_into('<H', sample, 4, digital)
        for button, value in (analog or {}).items():
            assert 0 <= button < 8 and 0 <= value <= 255
            sample[6+button] = value
        self.xinput_sample = bytes(sample)
        self.action(name+'_hardware_poll', 0x126350, (milliseconds,))
        generated = self.generated_action(name)
        processed = self.player_phase(name+'_bounded_original_player_phase') if process else None
        record = dict(name=name, supplied_poll_milliseconds=milliseconds,
                      supplied_packet_hex=self.xinput_sample.hex(),
                      raw_hex=bytes(self.g.mem_read(RAW, 0x48)).hex(),
                      raw_frames=list(self.g.mem_read(RAW+0x10, 16)),
                      raw_milliseconds=list(struct.unpack('<16H', self.g.mem_read(RAW+0x20, 32))),
                      generated=generated, processed=processed,
                      inventory=self.inventory_state())
        self.samples.append(record)
        return record

    def seeded_duration(self, button, milliseconds, frames=7):
        # Explicit processed-device initial state; generated action flags are
        # never supplied. The subsequent original poll increments these values.
        self.g.mem_write(RAW+button, b'\xff')
        self.g.mem_write(RAW+8+button, b'\xdf')
        self.g.mem_write(RAW+0x10+button, bytes([frames]))
        self.sh(RAW+0x20+button*2, milliseconds)

    def unit_switch_phase(self, name):
        definition = self.u(self.u(0x4eb114)+(self.u(UNIT)&0xffff)*16+8)
        self.fragment(name, 0xc6d6b, 0xc6e19,
                      ((UC_X86_REG_ESI, definition), (UC_X86_REG_EBP, UNIT),
                       (UC_X86_REG_EBX, UI)))


class ControlsTrace(ControlsMixin, TargetTrace):
    def __init__(self, target=BR, trigger=0, layout=0):
        self.controls_ready = False
        super().__init__(target, trigger)
        self.setup_controls(layout)


class AllocatedControlsTrace(ControlsMixin, WorldTrace):
    def __init__(self, row=1, trigger=0):
        self.controls_ready = False
        super().__init__(12, True)
        self.w(SCRATCH, 0x463b48)
        self.w(UNIT+0x13c, 0)
        self.supplied_unit_pose()
        self.sample('original_initial_smg_world_allocation', 12)
        self.initial_datum = self.u(self.entries+12*8)
        self.initial_pointer = self.u(HEADERS+(self.initial_datum&0xffff)*12+8)
        self.action('attach_original_allocated_initial_smg', 0xce510, (UI, self.initial_datum, 1))
        self.g.mem_write(self.initial_pointer+0x20c, bytes([trigger]))
        self.row = row
        self.record = self.u(self.scenario+0x124)+row*0x90
        self.sh(self.entries+row*8+4, 1)
        self.sample('original_target_br_world_allocation', row+30)
        self.target_datum = self.u(self.entries+row*8)
        self.target_pointer = self.u(HEADERS+(self.target_datum&0xffff)*12+8)
        self.allocation_before = self.object_state(self.target_datum)
        self.g.mem_write(self.target_pointer+0x20c, b'\x00')
        # Unit root pose/radius is a declared geometric fixture near the actual
        # allocated row weapon. Original matrix/bounds math reads real metadata.
        for n in range(3):
            self.f(UNIT+0x64+n*4, self.fv(self.target_pointer+0x30+n*4)+(0.05 if n == 0 else 0))
        self.unit_bounds = TargetTrace.bounds(self, UI, UNIT)
        self.g.mem_write(UNIT+0x40, bytes(self.g.mem_read(UNIT+0x30, 12)))
        self.services[0xc57a0] = ('supplied clear pickup visibility; full world ray admission excluded', 1, 0)
        self.setup_controls()

    def hook(self, g, a, size, user):
        if getattr(self, 'controls_ready', False) and a == 0xbbf40:
            sp = g.reg_read(UC_X86_REG_ESP)
            mask, out, maximum = self.u(sp+8), self.u(sp+24), self.u(sp+28)
            values = [self.target_datum] if mask & 4 and maximum else []
            for n, datum in enumerate(values):
                self.w(out+n*4, datum)
            self.control_calls.append(dict(phase=self.active, entry='000bbf40', supplied=True,
                type_mask=f'{mask:08x}', returned=[f'{d:08x}' for d in values],
                description='supplied spatial enumeration of original allocated target datum'))
            self.services[a] = ('supplied spatial enumeration of allocated row object', len(values), 28)
        super().hook(g, a, size, user)


def run_cases():
    traces, cases = [], []
    def save(t, name, **fields):
        assert not t.control_returns
        traces.append(t)
        cases.append(dict(name=name, **fields, profile_hex=t.profile_hex,
                          samples=t.samples, final_inventory=t.inventory_state(),
                          calls=t.control_calls, rows=t.inventory_rows, events=t.events,
                          supplied_services=sorted({e['service'] for e in t.events if 'service' in e})))
    for layout in range(4):
        t = ControlsTrace(layout=layout)
        t.seeded_duration(2, 232)
        r = t.poll('exact_primary_threshold', {2:255}, milliseconds=1)
        assert r['generated']['flags'] == 3
        assert t.u(UNIT+0x21c) == W2
        assert t.u(UNIT+0x212, 1) == 1 and t.u(UNIT+0x216, 1) == 1
        assert t.u(SEED) == 0x12345678
        save(t, f'primary_br_profile_{layout}_233ms', layout=layout)
    for target in (BR, SMG):
        for button in (2, 3):
            for ms in (232, 233):
                t = ControlsTrace(target)
                t.seeded_duration(button, ms-1)
                r = t.poll('duration_boundary', {button:255}, milliseconds=1)
                emitted = [c for c in t.control_calls if c.get('type') == 20]
                accepted = ms == 233 and ((target == BR and button == 2) or (target == SMG and button == 3))
                assert bool(emitted) == accepted
                assert r['generated']['flags'] == ((3 if ms == 233 else 1) << (0 if button == 2 else 4))
                save(t, f'{target.rsplit(chr(92),1)[-1]}_button_{button}_{ms}ms',
                     target_definition=target, supplied_initial_device_duration_ms=ms-1,
                     button=button, duration_ms=ms, emitted=emitted)
    for trigger in (0, 1, 2):
        t = ControlsTrace(trigger=trigger)
        t.seeded_duration(2, 232)
        t.poll('primary_pickup', {2:255}, milliseconds=1)
        emitted = [c for c in t.control_calls if c.get('type') == 20]
        assert len(emitted) == 1 and emitted[0]['mode'] == (3 if trigger == 0 else 4)
        assert t.u(UNIT+0x218+(1 if trigger == 0 else 0)*4) == W2
        assert t.u(SEED) == (0x12345678 if trigger == 0 else 0x4b5c952c)
        save(t, f'primary_br_trigger_selector_{trigger}', numeric_trigger_selector=trigger)
    for kind, arguments in (
            ('idle', dict(hold_ticks=0, release_ticks=1)),
            ('held_firing', dict(hold_ticks=45, release_ticks=0)),
            ('full_reload', dict(hold_ticks=45, release_ticks=1, reload_tick=45)),
            ('empty_reload', dict(hold_ticks=135, release_ticks=1, reload_tick=135))):
        t = ControlsTrace()
        # This prelude uses the existing original SMG input/tick/update oracle.
        # It does not seed a numeric selector from a native animation label.
        prelude = t.execute(**arguments)
        source_state = t.state()
        seed = t.u(SEED)
        assert source_state['trigger_state'] == 0
        if kind.endswith('reload'):
            assert source_state['magazine_mode'] == 1
        t.seeded_duration(2, 232)
        t.poll('pickup_after_actual_smg_'+kind, {2:255}, milliseconds=1)
        emitted = [c for c in t.control_calls if c.get('type') == 20]
        assert len(emitted) == 1 and emitted[0]['mode'] == 3
        assert t.u(UNIT+0x21c) == W2 and t.u(UNIT+0x212, 1) == 1
        assert t.u(WEAPON+0x22c, 2) == source_state['loaded']
        assert t.u(WEAPON+0x22a, 2) == source_state['reserve']
        assert t.u(SEED) == seed
        save(t, 'actual_smg_'+kind+'_primary_br_pickup',
             source_weapon_state_before_pickup=source_state,
             original_weapon_prelude=prelude,
             source_weapon_tick_arguments=arguments)
    t = ControlsTrace()
    for n in range(11):
        t.poll(f'held_poll_{n}', {2:255}, milliseconds=33)
    t.poll('first_release_poll', {}, milliseconds=33)
    t.poll('second_release_poll', {}, milliseconds=33)
    t.poll('neutral_after_release', {}, milliseconds=33)
    emitted = [c for c in t.control_calls if c.get('type') == 20]
    assert len(emitted) == 1
    assert t.samples[7]['generated']['flags'] == 1
    assert t.samples[8]['generated']['flags'] == 3
    assert t.samples[8]['raw_milliseconds'][2] == 264
    assert t.samples[9]['processed']['flags'] == 0
    assert t.samples[11]['raw_frames'][2] == 11
    assert t.samples[12]['raw_frames'][2] == 0
    assert t.samples[12]['processed']['consumed_player_flags'] & 0x10 == 0
    save(t, 'hardware_hold_consumption_and_release', supplied_poll_interval_ms=33)
    for kind in ('no_frame_bit', 'disabled_unit', 'invalid_candidate', 'input_cleared'):
        t = ControlsTrace()
        t.seeded_duration(2, 232)
        if kind == 'input_cleared':
            t.g.mem_write(0x4f026a, b'\x01')
        t.poll('generated_threshold', {2:255}, milliseconds=1, process=False)
        if kind == 'disabled_unit':
            t.w(UNIT+0x134, t.u(UNIT+0x134) & ~1)
            # No remote/local auxiliary processing for this gate case.
            t.sh(PLAYER_DATA+0x28, -1)
        elif kind == 'invalid_candidate':
            t.w(ACTION+0x2c, -1)
        t.player_phase('bounded_player_admission_gate', 0 if kind == 'no_frame_bit' else 1)
        assert not any(c.get('type') == 20 for c in t.control_calls)
        save(t, 'admission_'+kind)
    t = ControlsTrace()
    t.seeded_duration(2, 232)
    t.poll('pick_up_br', {2:255}, milliseconds=1)
    t.poll('release_primary_first', {}, milliseconds=33)
    t.poll('release_primary_second', {}, milliseconds=33)
    # A short secondary press followed by release requests the other carried
    # slot. This is a source selector/phase trace, not animation-time proof.
    t.poll('secondary_press_first', {3:255}, milliseconds=33)
    t.poll('secondary_press_second', {3:255}, milliseconds=33)
    t.poll('secondary_release_first', {}, milliseconds=33)
    r = t.poll('secondary_release_second', {}, milliseconds=33)
    assert r['generated']['slots'][0] == 0
    assert t.u(UNIT+0x216, 1) == 0
    t.unit_switch_phase('original_unit_switch_request_phase')
    assert any(c['entry'] == '000e7ef0' and c.get('type') == 8 for c in t.control_calls)
    save(t, 'pickup_then_short_secondary_release_switch',
         animation_limit='original animation availability fixture; duration/visual parity excluded')
    for row, trigger in ((0,0), (1,0), (1,1)):
        t = AllocatedControlsTrace(row, trigger)
        t.seeded_duration(2, 232)
        r = t.poll('allocated_row_primary_pickup', {2:255}, milliseconds=1)
        emitted = [c for c in t.control_calls if c.get('type') == 20]
        assert len(emitted) == 1 and emitted[0]['weapon_or_payload'] == f'{t.target_datum:08x}'
        assert t.u(t.target_pointer+0x154) == UI
        assert t.u(UNIT+0x218+(0 if trigger else 1)*4) == t.target_datum
        assert t.u(t.target_pointer+0xaf, 1) == row
        assert t.u(t.target_pointer+0x22c, 2) == 36
        assert t.u(t.target_pointer+0x22a, 2) == 72
        assert t.initial_datum == 0x10002 and t.target_datum == 0x20003
        row_before_timer = t.row_state()
        t.action('original_netgame_timer_stage_after_same_tick_pickup', 0x15c4b0, (t.entries,))
        assert t.row_state()['timer'] == 44
        assert t.row_state()['object'] == f'{t.target_datum:08x}'
        save(t, f'original_allocated_br_row_{row}_trigger_{trigger}',
             initial_smg_datum=f'{t.initial_datum:08x}', target_datum=f'{t.target_datum:08x}',
             allocation_before=t.allocation_before, allocation_after=t.object_state(t.target_datum),
             row_before_timer_stage=row_before_timer, row_after=t.row_state(), unit_bounds=t.unit_bounds,
             lower_world_limit='Havok shape unavailable, supplied unit root pose/registry center and pickup visibility')
    return traces, cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT/'report.json')
    args = parser.parse_args()
    traces, cases = run_cases()
    t = traces[0]
    directory = args.output.parent/'instructions'
    directory.mkdir(parents=True, exist_ok=True)
    executed = set().union(*(t.executed for t in traces))
    instructions = []
    for entry in ENTRIES:
        routine = _trace(t.code, entry, t.starts | set(ENTRIES))
        ins = [t.code.at(a) for a in sorted(routine.code)]
        path = directory/f'{entry:08x}.asm'
        path.write_text('\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}' for i in ins)+'\n')
        instructions.append(dict(entry=f'{entry:08x}', listing=str(path.relative_to(ROOT)),
            sha256=hashlib.sha256(path.read_bytes()).hexdigest(), instruction_count=len(ins),
            executed_instruction_count=sum(i.address in executed for i in ins),
            original_instruction_bytes_sha256=hashlib.sha256(b''.join(i.bytes for i in ins)).hexdigest(),
            direct_calls=[dict(site=f'{s:08x}', target=f'{d:08x}') for s,d,k in routine.sites if k == 'call']))
    p = t.u(t.u(0x4e791c)+0xf4)
    authored_maps = []
    paths = [ROOT/'assets/disc/maps'/f'{name}.map' for name in MAPS]
    paths += sorted((ROOT/'assets/multiplayer-map-pack/demos/dlc').glob('*/*.map'))
    for path in paths:
        cache = Cache(path)
        matg = next(tag for tag in cache.tags.values() if tag['group'] == 'matg')
        root = matg['offset']
        count = cache.u(root+0xf0)
        record = cache.u(root+0xf4)-cache.magic
        seconds = cache.f(record+0x7c)
        single_ms = struct.unpack('<f', struct.pack('<f', seconds*1000.0))[0]
        authored_maps.append(dict(map=path.stem, path=str(path.relative_to(ROOT)),
            sha256=hashlib.sha256(cache.data).hexdigest(), matg=matg,
            player_control_count=count, first_record_file_offset=f'{record:08x}',
            hold_seconds_bits=f'{cache.u(record+0x7c):08x}', hold_seconds=seconds,
            original_single_product_ms=single_ms, truncation_threshold_ms=int(single_ms),
            scope='authored metadata read; control execution uses Lockout'))
        cache.data.close()
        cache.stream.close()
    assert len(authored_maps) == 21
    report = dict(xbe_sha256=PIN, cache_sha256=CACHE_PIN,
        oracle_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        dependency_tool_sha256={name:hashlib.sha256((ROOT/'tools'/name).read_bytes()).hexdigest()
            for name in ('oracle_smg_firing_1_1.py', 'oracle_mp_inventory_1_1.py', 'oracle_mp_inventory_target_1_1.py', 'oracle_mp_inventory_world_1_1.py')},
        authored_hold_threshold=dict(original_pointer=f'{p:08x}', file_offset=f'{p-t.cache.magic+0x7c:08x}',
            relative_offset='7c', seconds_bits=f'{t.u(p+0x7c):08x}', seconds=t.fv(p+0x7c),
            milliseconds_scale_bits=f'{t.u(0x46513c):08x}', milliseconds_scale=t.fv(0x46513c),
            conversion='original MULSS followed by CVTTSS2SI at00188433', threshold_ms=233),
        original_button_tables=dict(analog=bytes(t.g.mem_read(0x447288,8)).hex(),
                                    digital=bytes(t.g.mem_read(0x447290,8)).hex()),
        authored_player_control_maps=authored_maps, cases=cases, instructions=instructions,
        fixture='One present Xbox gamepad/controller/local player, original profile mapping, declared near-target unit geometry, '
                'supplied spatial enumeration/visibility and existing labeled animation services. '
                'Original allocated cases read actual netgame row objects and original salt/index assignment.',
        scope='Complete original hardware packet processing, profile mapping, full control generation, '
              'bounded final player phase14ED0C..14ED4F, pickup request/admission/ownership and unit switch-request fragment. '
              'No native binding, complete input frame scheduler, full biped lifecycle, animation timing, Havok or live networking proof.')
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases), routine_rows=sum(len(t.inventory_rows) for t in traces),
                          input_samples=sum(len(t.samples) for t in traces))))


if __name__ == '__main__':
    main()
