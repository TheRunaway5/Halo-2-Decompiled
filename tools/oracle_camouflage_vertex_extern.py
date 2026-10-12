#!/usr/bin/env python3
"""Execute unchanged whole retail19BE0/1B2C0 for scalar/RGB vertex externs.

Declared finite scalar mode0 and component4 RGB inputs exercise both banks,
all16 registers, all scalar axes, ignored row byte2 and destination bits10..12,
destination texture selectors0/1, nonzero untouched axes and dirty masks.
19BE0 passes scalar mode0 regardless of destination highbits. Compiled camouflage
mappings reuse and independently reproduce all420 immutable source-bank cases.
No periodic/dimension mode, native implementation, cache success or GPU service.
"""
import argparse
import itertools
import json
import struct
from pathlib import Path

from oracle_camouflage_overlay import CamouflageOverlay
from oracle_camouflage_render_context import ROOT, sha
from oracle_damage_feedback import bits
from oracle_mp_spawn import Cache, CACHE_PIN
from oracle_multiplayer_hud_positions import PIN
from disasm import listing
from inventory import read_rows
from xbe import FUNCTIONS_CSV
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_ECX, UC_X86_REG_EDI

OUT = ROOT / 'analysis/multiplayer/game-variants/camouflage-binder-20261011'
FIXTURE = ROOT / 'tests/fixtures/camouflage_vertex_extern_original.json'
PRIOR = ROOT / 'analysis/multiplayer/game-variants/camouflage-pass-20261011/report.json'
PRIOR_PIN = '152fca115d6c5e0f5a528873b2f95938446aa1b29b6607fc61ec07ff5d0887c2'


class VertexTrace(CamouflageOverlay):
    def hook(self, uc, address, size, data):
        super().hook(uc, address, size, data)
        if getattr(self, 'recording', False) and address == 0x1b2c0:
            sp = uc.reg_read(UC_X86_REG_ESP)
            self.callee_inputs.append(dict(texture_selector=self.r(sp + 4), scalar_mode=self.r(sp + 8),
                                           source=uc.reg_read(UC_X86_REG_ECX), component=uc.reg_read(UC_X86_REG_EDI)))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=OUT)
    ap.add_argument('--fixture', type=Path, default=FIXTURE)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    assert sha(PRIOR) == PRIOR_PIN
    assert sha(ROOT / 'assets/disc/maps/lockout.map') == CACHE_PIN
    cache = Cache(ROOT / 'assets/disc/maps/lockout.map')
    o = VertexTrace(cache)
    o.watched = (0x19be0, 0x1b2c0)
    o.recording = True
    bank = o.alloc(bytes(0x1800))
    post = o.alloc(bytes(0x40))
    table = o.alloc(bytes(0x100))
    packed_range = o.alloc(bytes(4))
    rows = read_rows(FUNCTIONS_CSV)
    owners = []
    for name, address in (('vertex_extern', 0x19be0), ('extern_value', 0x1b2c0)):
        data = o.image.read(address, int(rows[address]['size']))
        path = args.output / f'{name}-{address:08x}.asm'
        path.write_text(f'{address:08x} indexed_bytes={len(data)} sha256={__import__("hashlib").sha256(data).hexdigest()}\n'
                        + '\n'.join(listing(data, address, rows)) + '\n')
        owners.append(dict(address=address, indexed_bytes=len(data), listing=path.name, listing_sha256=sha(path)))

    def run_case(row):
        o.g.mem_write(bank, bytes(0x1800))
        o.w(bank + 0x20, post)
        o.w(post + 0x3c, table)
        o.g.mem_write(table, b'\xa5' * 0x100)
        first = row['range_first']
        mapping = b''.join(bytes.fromhex(v) for v in row['mappings'])
        o.g.mem_write(table + first * 4, mapping or b'')
        o.g.mem_write(packed_range, struct.pack('<H', len(row['mappings']) << 9 | first))
        for index, value in row['scalars_bits'].items():
            o.w(bank + 0x5c + int(index) * 4, value)
        for index, color in row['colors_bits'].items():
            o.g.mem_write(bank + 0x108 + int(index) * 12, struct.pack('<3I', *color))
        initial = bytes.fromhex(row['initial_vertex_hex'])
        o.g.mem_write(bank + 0x1530, initial)
        o.g.mem_write(bank + 0x1730, struct.pack('<2I', *row['initial_vertex_dirty_masks']))
        before = dict(o.entries)
        o.callee_inputs = []
        o.run(0x19be0, argv=[bank, packed_range])
        row['callee_inputs'] = o.callee_inputs.copy()
        assert all(arg['scalar_mode'] == 0 for arg in row['callee_inputs'])
        row['vertex_hex'] = bytes(o.g.mem_read(bank + 0x1530, 512)).hex()
        row['vertex_dirty_masks'] = list(struct.unpack('<2I', o.g.mem_read(bank + 0x1730, 8)))
        row['calls'] = {f'{address:08x}': o.entries[address] - before.get(address, 0) for address in o.watched}
        assert row['calls']['00019be0'] == 1
        assert row['calls']['0001b2c0'] == len(row['mappings'])
        return row

    # Source inputs are declared, not expected outputs. All output bits below
    # come from the unchanged executable. Distinct baseline axes expose clobbers.
    baseline = struct.pack('<128f', *(0.125 + n * .03125 for n in range(128))).hex()
    values = [bits(v) for v in (-2., -0., 0., .25, .5, .75, 1., 2.)] + [0x31c00000]
    cases = []
    for texture_selector, component, dst_bank, register, byte2, value in itertools.product(
            (0, 1), range(5), range(2), range(16), (0, 0xa5, 0xff), values):
        index = len(cases)
        source = (0, 37, 39, 41)[index % 4]
        ignored = index % 8
        dest = (texture_selector << 13) | (ignored << 10) | (component << 5) | (dst_bank << 4) | register
        row = dict(kind='declared_finite', texture_selector=texture_selector, scalar_mode=0,
                   component=component, bank=dst_bank, register=register,
                   byte2=byte2, ignored_destination_bits10_12=ignored, source=source,
                   range_first=index % 8, mappings=[struct.pack('<HBB', dest, byte2, source).hex()],
                   scalars_bits={str(source): value}, colors_bits={str(source): [value, bits(.375), bits(-.625)]},
                   initial_vertex_hex=baseline, initial_vertex_dirty_masks=[0xa0050005, 0x40014001])
        cases.append(run_case(row))

    # Multiple mappings to one register expose source iteration order and mask
    # accumulation, including scalar writes after/before the RGB copy.
    for dst_bank, register, reverse in itertools.product(range(2), range(16), (False, True)):
        maps = [struct.pack('<HBB', (axis << 5) | (dst_bank << 4) | register | ((axis % 2) << 13),
                            0xa5, 37).hex() for axis in range(5)]
        if reverse:
            maps.reverse()
        cases.append(run_case(dict(kind='declared_order', bank=dst_bank, register=register, reverse=reverse,
                                   range_first=7, mappings=maps, scalars_bits={'37': bits(.25)},
                                   colors_bits={'37': [bits(.75), bits(-.5), bits(1.25)]},
                                   initial_vertex_hex=baseline, initial_vertex_dirty_masks=[0xa0050005, 0x40014001])))

    generic_count = len(cases)
    prior = json.loads(PRIOR.read_text())
    for index, previous in enumerate(prior['cases']):
        row = dict(kind='compiled_camouflage', prior_case=index, layer=previous['layer'],
                   implementation=previous['implementation'], primary_bits=previous['primary_bits'],
                   fog=previous['fog'], local_user=previous['local_user'], range_first=0,
                   mappings=previous['mappings']['vertex'], scalars_bits=previous['scalar_bits'],
                   colors_bits={'40': previous['color40_bits'], '41': previous['color41_bits']},
                   initial_vertex_hex=previous['initial_vertex_hex'], initial_vertex_dirty_masks=[0, 0])
        result = run_case(row)
        assert result['vertex_hex'] == previous['vertex_hex'], index
        assert result['vertex_dirty_masks'] == previous['vertex_dirty_masks'], index
        cases.append(result)

    dependencies = [Path(__file__).resolve(), ROOT / 'tools/oracle_camouflage_overlay.py',
                    ROOT / 'tools/oracle_shield_overlay.py', ROOT / 'tools/oracle_damage_feedback.py',
                    ROOT / 'tools/oracle_multiplayer_hud_positions.py', ROOT / 'tools/oracle_camouflage_render_context.py',
                    ROOT / 'tools/oracle_mp_spawn.py', ROOT / 'assets/disc/maps/lockout.map', PRIOR]
    provenance = dict(xbe_sha256=PIN, cache_sha256=CACHE_PIN, prior_report_sha256=PRIOR_PIN,
                      dependencies={str(path.relative_to(ROOT)): sha(path) for path in dependencies},
                      instruction_patches=[], supplied_returns=[], floating_point=dict(fpcw=0x027f, mxcsr=0x1f80),
                      scalar_modes=[0], destination_texture_selectors=[0, 1],
                      components=[0, 1, 2, 3, 4], banks=[0, 1], registers=list(range(16)),
                      correction='Earlier metadata called destination bits13..15 mode. They select texture;19BE0 passes scalar mode0 as the second callee argument. Oracle output bits are unchanged.')
    fixture = dict(schema='original-camouflage-vertex-extern-v1', boundary=__doc__, provenance=provenance, cases=cases)
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text(json.dumps(fixture, sort_keys=True, indent=2) + '\n')
    report = dict(schema='original-camouflage-vertex-extern-report-v1', boundary=__doc__, provenance=provenance,
                  fixture_sha256=sha(args.fixture), cases=len(cases), declared_cases=generic_count,
                  compiled_camouflage_cases=len(prior['cases']), owners=owners,
                  executed_owner_counts={f'{address:08x}': count for address, count in sorted(o.entries.items())},
                  executed_original=[dict(address=address, bytes_hex=o.image.read(address, size).hex())
                                     for address, size in sorted(o.executed.items())],
                  limits=['Complete19BE0 and reached1B2C0 execute to return; inputs are declared banks and row ranges.',
                          'Scalar mode0 and component4 RGB only; destination texture selectors0/1 are ignored by these components. Periodic/dimension/nonfinite behavior is not tested.',
                          'The420 compiled cases import immutable original1AE70-produced source banks and reproduce previous vertex outputs exactly.',
                          'No whole16B90, resource/cache readiness, animation, graphics submission or framebuffer parity is claimed.'])
    path = args.output / 'vertex-report.json'
    path.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(cases=len(cases), declared_cases=generic_count, compiled_cases=420,
                          fixture_sha256=sha(args.fixture), report_sha256=sha(path)), indent=2))


if __name__ == '__main__':
    main()
