#!/usr/bin/env python3
"""Unchanged whole1A170 generic render-state externs and bounded reset owners.

The literal seven-row render table is distinct from adjacent1AB50 texture
state data. Declared finite scalar/RGB banks drive all seven row indices,
all four rounding controls, ignored byte2 and non1 color-provider controls.
Each whole owner is repeated with the same resulting cache value to observe
command emission; multirow cases observe ordered source write words directly.
Whole16B10 and bounded1BBF0 prefixes measure actual
Lockout camouflage reset ranges. No instructions/returns/services are supplied.
"""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path

from oracle_camouflage_overlay import CamouflageOverlay
from oracle_camouflage_render_context import ROOT, sha
from oracle_damage_feedback import bits
from oracle_mp_spawn import Cache, CACHE_PIN
from oracle_multiplayer_hud_positions import PIN, STOP
from unicorn.x86_const import (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EIP,
                               UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW,
                               UC_X86_REG_FPTAG, UC_X86_REG_FP0, UC_X86_REG_ECX, UC_X86_REG_EDX)
from xbe import FUNCTIONS_CSV
from inventory import read_rows
from disasm import listing

OUT = ROOT / 'analysis/multiplayer/game-variants/render-state-extern-20261011'
FIXTURE = ROOT / 'tests/fixtures/render_state_extern_original.json'
FISTPS = {0x1a1f8: 0x10, 0x1a821: 0x14, 0x131efe: 0, 0x131f0b: 0, 0x131f1c: 0}
OWNERS = dict(state_extern=0x1a170, rgb_pack=0x131ed0, immediate_state=0x3f5cf0,
              fog_color=0x3f5370, line_width=0x3f5730, trunc_float=0x3fcf00,
              bank_reset=0x16b10, shader_dispatch=0x1bbf0, ordinary_state=0x18900,
              texture_state_extern=0x1ab50, fog_program_dependency=0x3f6f40)


class StateTrace(CamouflageOverlay):
    def __init__(self, cache):
        super().__init__(cache)
        self.watched = tuple(OWNERS.values())
        self.sequence = []
        self.fp = []
        self.pending = None
        self.stop_pass = False
        self.write_events = []
        self.state_methods = {struct.unpack('<I', self.image.read(0x43e6f0 + state * 4, 4))[0]: state
                              for state in (75, 61, 77, 78)}

    def hook(self, uc, pc, size, data):
        self.last = pc
        if getattr(self, 'recording', False) and self.stop_pass and pc in (0x16b90, 0x3f6f40):
            assert bytes(uc.mem_read(pc, size)) == self.image.read(pc, size)
            uc.emu_stop()
            return
        super().hook(uc, pc, size, data)
        if not getattr(self, 'recording', False):
            return
        if pc in self.watched:
            self.sequence.append(pc)
        if pc == 0x3f5cf0 and uc.reg_read(UC_X86_REG_ECX) in self.state_methods:
            self.write_events.append(dict(state=self.state_methods[uc.reg_read(UC_X86_REG_ECX)],
                                          value=uc.reg_read(UC_X86_REG_EDX), owner=pc))
        elif pc in (0x3f5370, 0x3f5730):
            self.write_events.append(dict(state=138 if pc == 0x3f5370 else 157,
                                          value=self.r(uc.reg_read(UC_X86_REG_ESP) + 4), owner=pc))
        if self.pending:
            row, address, next_pc = self.pending
            assert pc == next_pc
            row['produced_integer'] = struct.unpack('<i', uc.mem_read(address, 4))[0]
            row['fpsw_after'] = uc.reg_read(UC_X86_REG_FPSW)
            self.pending = None
        if pc in FISTPS:
            sw = uc.reg_read(UC_X86_REG_FPSW)
            mantissa, exponent = uc.reg_read(UC_X86_REG_FP0 + ((sw >> 11) & 7))
            row = dict(address=pc, fpcw=uc.reg_read(UC_X86_REG_FPCW), fpsw_before=sw,
                       fp80_top_mantissa=mantissa, fp80_top_exponent=exponent)
            self.fp.append(row)
            self.pending = row, uc.reg_read(UC_X86_REG_ESP) + FISTPS[pc], pc + size


def census(cache):
    cases, resets = [], []
    pass_count = impl_count = 0
    for datum, tag in cache.tags.items():
        if tag['group'] != 'spas':
            continue
        pass_count += 1
        count = cache.u(tag['offset'] + 28)
        start = cache.u(tag['offset'] + 32) - cache.magic
        for post_index in range(count):
            post = start + post_index * 88
            count = cache.u(post)
            base = cache.u(post + 4) - cache.magic
            states = cache.u(post + 20) - cache.magic
            externs = cache.u(post + 60) - cache.magic
            for index in range(count):
                impl_count += 1
                impl = base + index * 306
                packed = struct.unpack_from('<H', cache.data, impl + 0x11e)[0]
                for n in range(packed >> 9):
                    raw = cache.data[externs + ((packed & 511) + n) * 4:externs + ((packed & 511) + n) * 4 + 4]
                    cases.append(dict(datum=datum, name=tag['name'], post=post_index, implementation=index, mapping_hex=raw.hex()))
                packed = struct.unpack_from('<H', cache.data, impl + 0x11c)[0]
                raw_states = [cache.data[states + ((packed & 511) + n) * 5:states + ((packed & 511) + n) * 5 + 5]
                              for n in range(packed >> 9)]
                resets.extend(dict(datum=datum, name=tag['name'], post=post_index, implementation=index, state_hex=raw.hex())
                              for raw in raw_states if raw[0] == 75)
    return dict(compiled_spas=pass_count, compiled_implementations=impl_count, render_state_externs=cases,
                state75_reset_records=resets)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=OUT)
    ap.add_argument('--fixture', type=Path, default=FIXTURE)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    assert sha(ROOT / 'assets/disc/maps/lockout.map') == CACHE_PIN
    cache = Cache(ROOT / 'assets/disc/maps/lockout.map')
    o = StateTrace(cache)
    bank = o.alloc(bytes(0x1800))
    post = o.alloc(bytes(0x40))
    externs = o.alloc(bytes(0x80))
    packed_range = o.alloc(bytes(4))
    command = o.alloc(bytes(0x10000))
    o.w(post + 0x3c, externs)
    o.w(bank + 0x20, post)
    o.w(0x407488, 0x404fe0)
    o.w(0x404fe4, command + 0xff00)
    o.w(0x404fe0 + 0x964, bits(1.))
    table = [dict(index=index, raw_hex=o.image.read(0x467020 + index * 4, 4).hex(),
                  state=struct.unpack('<H', o.image.read(0x467020 + index * 4, 2))[0],
                  semantic_byte=o.image.read(0x467020 + index * 4 + 2, 1)[0],
                  scalar_conversion=o.image.read(0x467020 + index * 4 + 3, 1)[0]) for index in range(7)]
    o.recording = True

    def whole_state(cw, state, expected_writes=1):
        o.w(0x404fe0, command)
        o.g.mem_write(command, b'\xa5' * 64)
        o.sequence, o.fp, o.pending = [], [], None
        o.write_events = []
        before = o.r(0x407498 + state * 4)
        o.prepare([packed_range], [(UC_X86_REG_EBX, bank)])
        o.g.reg_write(UC_X86_REG_FPCW, cw)
        o.g.reg_write(UC_X86_REG_FPSW, 0)
        o.g.reg_write(UC_X86_REG_FPTAG, 0xffff)
        o.g.emu_start(0x1a170, STOP, count=10000)
        assert o.g.reg_read(UC_X86_REG_EIP) == STOP and o.pending is None
        assert all(row['fpcw'] == cw for row in o.fp)
        size = o.r(0x404fe0) - command
        assert size == expected_writes * 8 and len(o.write_events) == expected_writes
        return dict(before_state_value=before, after_state_value=o.r(0x407498 + state * 4),
                    command_hex=bytes(o.g.mem_read(command, size)).hex(), fistp_observations=o.fp.copy(),
                    ordered_writes=o.write_events.copy(),
                    owner_sequence=[f'{pc:08x}' for pc in o.sequence])

    cases = []
    values = [bits(v) for v in (-2., -0., 0., .25, .5, .75, 1., 1.5, 2., 1e10)] + [0x31c00000]
    for index, provider, cw, byte2, source, value in itertools.product(range(7), (1, 0, 2, 255),
            (0x027f, 0x067f, 0x0a7f, 0x0e7f), (0, 0xa5, 0xff), (0, 40), values):
        first = len(cases) % 8
        raw = bytes((index, provider, byte2, source))
        o.g.mem_write(externs + first * 4, raw)
        o.g.mem_write(packed_range, struct.pack('<H', 1 << 9 | first))
        color = [value, bits(.5), bits(.75)]
        o.w(bank + 0x5c + source * 4, value)
        o.g.mem_write(bank + 0x108 + source * 12, struct.pack('<3I', *color))
        state = table[index]['state']
        initial = (0x00123456, 0x7f123456, 0xff123456)[len(cases) % 3]
        o.w(0x407498 + state * 4, initial)
        first_result = whole_state(cw, state)
        repeated = whole_state(cw, state)
        assert repeated['before_state_value'] == repeated['after_state_value'] == first_result['after_state_value']
        assert repeated['command_hex'] == first_result['command_hex']
        cases.append(dict(table_index=index, state=state, provider_byte=provider, ignored_byte2=byte2, source=source,
                          mapping_hex=raw.hex(), range_first=first, scalar_bits=value, color_bits=color, fpcw=cw,
                          first=first_result, repeated_same_value=repeated))

    ordered = []
    for cw, reverse in itertools.product((0x027f, 0x067f, 0x0a7f, 0x0e7f), (False, True)):
        mappings = [bytes((0, 2, 0xa5, 40)), bytes((1, 1, 0xff, 37)),
                    bytes((0, 2, 0, 41)), bytes((1, 1, 0, 37))]
        if reverse:
            mappings.reverse()
        scalar = {'37': bits(.5)}
        colors = {'40': [bits(.25), bits(.5), bits(.75)], '41': [bits(.75), bits(.25), bits(.5)]}
        o.w(bank + 0x5c + 37 * 4, scalar['37'])
        for source, color in colors.items():
            o.g.mem_write(bank + 0x108 + int(source) * 12, struct.pack('<3I', *color))
        o.g.mem_write(externs + 7 * 4, b''.join(mappings))
        o.g.mem_write(packed_range, struct.pack('<H', len(mappings) << 9 | 7))
        o.w(0x407498 + 75 * 4, 0x7f123456)
        result = whole_state(cw, 75, expected_writes=len(mappings))
        ordered.append(dict(fpcw=cw, reverse=reverse, mappings=[raw.hex() for raw in mappings], range_first=7,
                            scalars_bits=scalar, colors_bits=colors, result=result))

    # Whole reset owners consume unchanged actual camouflage reset-state ranges.
    resets = []
    for layer, datum, index in [(18, 0xe7ec0667, n) for n in range(4)] + [(19, 0xe7ef066a, 0)]:
        root = o.r(o.table + (datum & 65535) * 16 + 8)
        actual_post = o.r(root + 32)
        impl = o.r(actual_post + 4) + index * 306
        packed = struct.unpack('<H', o.g.mem_read(impl + 0x11c, 2))[0]
        reset_states = [bytes(o.g.mem_read(o.r(actual_post + 0x14) + ((packed & 511) + n) * 5, 5)).hex()
                        for n in range(packed >> 9)]
        for operation in ('whole16B10', 'dispatch_reset0_prefix', 'dispatch_reset1_prefix'):
            current = bank if operation == 'whole16B10' else 0x485b48
            o.g.mem_write(current, bytes(0x1800))
            o.w(current + 0x20, actual_post)
            o.w(current + 0x24, impl)
            initial = 0x7f123456
            o.w(0x407498 + 75 * 4, initial)
            o.w(0x404fe0, command)
            o.sequence = []
            o.stop_pass = operation != 'whole16B10'
            if operation == 'whole16B10':
                o.run(0x16b10, registers=[(UC_X86_REG_ESI, current)])
                end = STOP
            else:
                o.g.mem_write(0x4670bc, bytes([int(operation == 'dispatch_reset1_prefix')]))
                o.prepare([0, layer, 0, index, bits(1.)], [(UC_X86_REG_EDI, 0xffffffff)])
                o.g.emu_start(0x1bbf0, STOP, count=10000)
                end = 0x3f6f40 if operation == 'dispatch_reset1_prefix' else 0x16b90
            assert o.g.reg_read(UC_X86_REG_EIP) == end
            assert o.r(0x407498 + 75 * 4) == initial
            size = o.r(0x404fe0) - command
            resets.append(dict(layer=layer, implementation=index, operation=operation, reset_state_rows=reset_states,
                               state75_before=initial, state75_after=o.r(0x407498 + 75 * 4),
                               owner_sequence=[f'{pc:08x}' for pc in o.sequence],
                               command_hex=bytes(o.g.mem_read(command, size)).hex(), stop_before=None if end == STOP else end))
    o.stop_pass = False

    provenance = dict(xbe_sha256=PIN, cache_sha256=CACHE_PIN, oracle_sha256=sha(__file__), instruction_patches=[], supplied_returns=[],
                      line_width_device_scale_bits=bits(1.), render_state_table=table,
                      provider_indexing=dict(scalar_offset=0x5c, scalar_stride=4, color_offset=0x108, color_stride=12),
                      dependencies={str(path.relative_to(ROOT)): sha(path) for path in (ROOT / 'tools/oracle_camouflage_overlay.py',
                          ROOT / 'tools/oracle_shield_overlay.py', ROOT / 'tools/oracle_damage_feedback.py',
                          ROOT / 'tools/oracle_multiplayer_hud_positions.py')})
    fixture = dict(schema='original-render-state-extern-v1', boundary=__doc__, provenance=provenance,
                   cases=cases, ordered_cases=ordered, reset_cases=resets)
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text(json.dumps(fixture, sort_keys=True, separators=(',', ':')) + '\n')
    rows = read_rows(FUNCTIONS_CSV)
    owners = []
    for name, address in OWNERS.items():
        data = o.image.read(address, int(rows[address]['size']))
        path = args.output / f'{name}-{address:08x}.asm'
        path.write_text(f'{address:08x} indexed_bytes={len(data)} sha256={hashlib.sha256(data).hexdigest()}\n'
                        + '\n'.join(listing(data, address, rows)) + '\n')
        owners.append(dict(owner=name, address=address, listing=path.name, listing_sha256=sha(path)))
    report = dict(schema='original-render-state-extern-report-v1', boundary=__doc__, provenance=provenance,
                  fixture_sha256=sha(args.fixture), cases=len(cases), complete_state_calls=len(cases) * 2,
                  ordered_cases=len(ordered), total_complete_state_calls=len(cases) * 2 + len(ordered),
                  reset_cases=len(resets), census=census(cache), owners=owners,
                  executed_original=[dict(address=pc, bytes_hex=o.image.read(pc, size).hex()) for pc, size in sorted(o.executed.items())],
                  limits=['Declared generic rows/banks; Lockout authored state externs are reported separately by complete compiled-spas census.',
                          'Render table has seven supported rows.1A170 itself does not bounds-check row/provider indices; invalid adjacent-data reads are not admitted.',
                          'Scalar provider is byte1==1; all tested non1 bytes take RGB branch. Byte2 is ignored.',
                          'Whole1A170 and reached CPU/device command owners return unchanged with backed command storage and explicit line-width scale1.',
                          'Same-value repeated writes emit commands; no cache/device ready return is supplied or graphics acceptance claimed.',
                          'Whole16B10 returns.1BBF0 reset0 prefixes stop before16B90; reset1 prefixes stop before3F6F40, whose live vertex-program dependency is not supplied. No whole1BBF0 completion is claimed.',
                          'Observed FP controls and integer/mask outputs do not establish architectural exception-status delivery; known Unicorn FPSW limitation remains.',
                          'No native planner/importer/publication, framebuffer parity, title-update equivalence or gamemode admission is changed.'])
    path = args.output / 'report.json'
    path.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(cases=len(cases), complete_state_calls=len(cases) * 2, reset_cases=len(resets),
                          compiled_spas=report['census']['compiled_spas'], authored_externs=len(report['census']['render_state_externs']),
                          state75_reset_records=len(report['census']['state75_reset_records']),
                          fixture_sha256=sha(args.fixture), report_sha256=sha(path)), indent=2))


if __name__ == '__main__':
    main()
