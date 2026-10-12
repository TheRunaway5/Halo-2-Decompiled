#!/usr/bin/env python3
"""Execute unchanged whole retail131ED0 with declared finite RGB/FP controls.

This generic RGB-pack corpus observes every original FISTP, its actual FPCW,
raw x87 value, produced signed32 before masking and FPSW before/after. Inputs
include asymmetric negative/over1/signedzero/fade colors, conversion ties and
signed32 overflow boundaries. All exception masks remain set. No instructions,
game function returns or successful graphics services are replaced/supplied.
This is a complete CPU owner oracle, not a state-range importer or GPU binder.
"""
import argparse
import hashlib
import itertools
import json
import random
import struct
import sys
from pathlib import Path

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP,
                               UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG,
                               UC_X86_REG_MXCSR, UC_X86_REG_FP0)

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe, FUNCTIONS_CSV
from inventory import read_rows
from disasm import listing

ENTRY, STATE, STACK, STOP = 0x131ed0, 0x2100000, 0x22fef00, 0x3000000
PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
OUT = ROOT / 'analysis/multiplayer/game-variants/state-rgb-pack-20261011'
FIXTURE = ROOT / 'tests/fixtures/state_rgb_pack_original.json'
FISTPS = {0x131efe: ('blue', 2), 0x131f0b: ('green', 1), 0x131f1c: ('red', 0)}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def finite(word):
    return word & 0x7f800000 != 0x7f800000


def declared_inputs():
    base = [bits(v) for v in (-2., -1., -.5, -0., 0., .25, .5, .75, 1., 1.5, 2.)] + [0x31c00000]
    rows = [dict(kind='finite_grid', rgb_bits=list(color)) for color in itertools.product(base, repeat=3)]
    probes = []
    # Adjacent binary32 inputs around exact/rounded byte conversion boundaries.
    for integer in (-256, -128, -1, 0, 1, 63, 127, 254, 255, 256, 510):
        center = bits((integer + .5) / 255.)
        probes.extend((f'byte_tie_{integer}_{step}', center + step) for step in (-1, 0, 1))
    # Float values around both signed32 FISTP range edges; keep actual finite
    # rounded inputs rather than assuming a mathematical boundary is representable.
    for value in ((2**31 - 1) / 255., 2**31 / 255., -(2**31) / 255., -(2**31 + 1) / 255.):
        center = bits(value)
        probes.extend((f'signed32_boundary_{center:08x}_{step}', center + step) for step in (-2, -1, 0, 1, 2))
    probes.extend((label, word) for label, word in (
        ('positive_max_finite', 0x7f7fffff), ('negative_max_finite', 0xff7fffff),
        ('positive_min_subnormal', 1), ('negative_min_subnormal', 0x80000001),
        ('positive_max_subnormal', 0x007fffff), ('negative_max_subnormal', 0x807fffff),
        ('positive_min_normal', 0x00800000), ('negative_min_normal', 0x80800000)))
    for (label, word), channel in itertools.product(probes, range(3)):
        color = [bits(.25), bits(.5), bits(.75)]
        color[channel] = word
        rows.append(dict(kind='conversion_boundary', label=label, channel=channel, rgb_bits=color))
    rng = random.Random(ENTRY)
    for _ in range(128):
        color = []
        while len(color) < 3:
            value = rng.getrandbits(32)
            if finite(value):
                color.append(value)
        rows.append(dict(kind='declared_finite_bit_patterns', rgb_bits=color))
    assert all(finite(word) for row in rows for word in row['rgb_bits'])
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=OUT)
    ap.add_argument('--fixture', type=Path, default=FIXTURE)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    x = Xbe(ROOT / 'assets/disc/default.xbe')
    assert hashlib.sha256(x.data).hexdigest() == PIN
    assert struct.unpack('<I', x.read(0x45dc38, 4))[0] == bits(255.)
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    g.mem_map(0x10000, 0x600000)
    for section in x.sections:
        g.mem_write(section.va, x.section_bytes(section))
    g.mem_map(STATE, 0x200000)
    g.mem_map(STOP, 0x1000)
    executed, observations = {}, []
    pending = None

    def hook(uc, pc, size, unused):
        nonlocal pending
        assert bytes(uc.mem_read(pc, size)) == x.read(pc, size)
        executed[pc] = size
        if pending is not None:
            row, address, next_pc = pending
            assert pc == next_pc
            row['produced_integer'] = struct.unpack('<i', uc.mem_read(address, 4))[0]
            row['produced_bits'] = struct.unpack('<I', uc.mem_read(address, 4))[0]
            row['fpsw_after'] = uc.reg_read(UC_X86_REG_FPSW)
            pending = None
        if pc in FISTPS:
            channel, index = FISTPS[pc]
            status = uc.reg_read(UC_X86_REG_FPSW)
            physical_top = (status >> 11) & 7
            mantissa, exponent = uc.reg_read(UC_X86_REG_FP0 + physical_top)
            row = dict(address=pc, channel=channel, channel_index=index,
                       fpcw=uc.reg_read(UC_X86_REG_FPCW), fpsw_before=status,
                       fp80_top_mantissa=mantissa, fp80_top_exponent=exponent)
            observations.append(row)
            pending = (row, uc.reg_read(UC_X86_REG_ESP), pc + size)

    g.hook_add(UC_HOOK_CODE, hook)
    rows = declared_inputs()
    cases = []
    for declared, control in itertools.product(rows, (0x027f, 0x067f, 0x0a7f, 0x0e7f)):
        raw = struct.pack('<3I', *declared['rgb_bits'])
        g.mem_write(STATE, raw)
        g.mem_write(STACK - 32, b'\xa5' * 64)
        g.mem_write(STACK, struct.pack('<2I', STOP, STATE))
        g.reg_write(UC_X86_REG_ESP, STACK)
        g.reg_write(UC_X86_REG_EAX, 0xa5a5a5a5)
        g.reg_write(UC_X86_REG_ECX, 0x12345678)
        g.reg_write(UC_X86_REG_FPCW, control)
        g.reg_write(UC_X86_REG_FPSW, 0)
        g.reg_write(UC_X86_REG_FPTAG, 0xffff)
        g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        before = bytes(g.mem_read(STACK - 8, 8)).hex()
        observations = []
        pending = None
        g.emu_start(ENTRY, STOP, count=1000)
        assert g.reg_read(UC_X86_REG_EIP) == STOP
        assert pending is None and len(observations) == 3
        assert all(row['fpcw'] == control for row in observations)
        assert bytes(g.mem_read(STATE, 12)) == raw
        assert g.reg_read(UC_X86_REG_ECX) == 0x12345678
        output = g.reg_read(UC_X86_REG_EAX)
        assert output <= 0xffffff
        cases.append(dict(**declared, fpcw=control, packed_rgb=output,
                          input_before_hex=raw.hex(), input_after_hex=bytes(g.mem_read(STATE, 12)).hex(),
                          scratch_before_hex=before, scratch_after_hex=bytes(g.mem_read(STACK - 8, 8)).hex(),
                          eax_before=0xa5a5a5a5, eax_after=output,
                          ecx_before=0x12345678, ecx_after=g.reg_read(UC_X86_REG_ECX),
                          returned_fpcw=g.reg_read(UC_X86_REG_FPCW), returned_fpsw=g.reg_read(UC_X86_REG_FPSW),
                          returned_fptag=g.reg_read(UC_X86_REG_FPTAG), fistp_observations=observations.copy()))
    provenance = dict(xbe_sha256=PIN, oracle_sha256=sha(__file__),
                      source_constant=dict(address=0x45dc38, bits=bits(255.)),
                      controls=dict(fpcw=[0x027f, 0x067f, 0x0a7f, 0x0e7f], initial_fpsw=0, initial_fptag=0xffff, mxcsr=0x1f80),
                      instruction_patches=[], supplied_returns=[],
                      status_limit='Unicorn returns signed32 indefinite on finite conversion overflow but the observed FPSW invalid flag remains clear. These observations do not establish architectural exception-status behavior.')
    fixture = dict(schema='original-state-rgb-pack-v1', boundary=__doc__, provenance=provenance, cases=cases)
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text(json.dumps(fixture, sort_keys=True, separators=(',', ':')) + '\n')
    indexed = read_rows(FUNCTIONS_CSV)
    data = x.read(ENTRY, int(indexed[ENTRY]['size']))
    path = args.output / 'rgb_pack-00131ed0.asm'
    path.write_text(f'{ENTRY:08x} indexed_bytes={len(data)} sha256={hashlib.sha256(data).hexdigest()}\n'
                    + '\n'.join(listing(data, ENTRY, indexed)) + '\n')
    fistps = [obs for case in cases for obs in case['fistp_observations']]
    report = dict(schema='original-state-rgb-pack-report-v1', boundary=__doc__, provenance=provenance,
                  fixture_sha256=sha(args.fixture), cases=len(cases), declared_rgb_vectors=len(rows),
                  actual_fistps=len(fistps), observed_invalid_fistps=sum(bool(row['fpsw_after'] & 1) for row in fistps),
                  observed_indefinite_integer_fistps=sum(row['produced_bits'] == 0x80000000 for row in fistps),
                  listing=path.name, listing_sha256=sha(path),
                  executed_original=[dict(address=pc, bytes_hex=x.read(pc, size).hex()) for pc, size in sorted(executed.items())],
                  limits=['Whole retail131ED0 executes unchanged to return; finite RGB bit patterns and FP controls are declared.',
                          'FISTP signed32 result is observed before original AND FF. Status/control/raw x87 values are emulator observations.',
                          'All exception masks are set. Unmasked exception delivery, nonfinite inputs and other precision controls are outside this corpus.',
                          'Unicorn FPSW invalid/precision status does not expose the overflow flag in these observations; architectural exception-status behavior is not admitted.',
                          'This owner produces RGB24 only. State alpha preservation is separately proven by whole1A170 binder cases.',
                          'No authored state-range importer, live state publication, GPU execution, framebuffer parity or title-update equivalence is claimed.'])
    path = args.output / 'report.json'
    path.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('cases', 'declared_rgb_vectors', 'actual_fistps', 'observed_invalid_fistps',
                                                  'observed_indefinite_integer_fistps', 'fixture_sha256')}, indent=2))


if __name__ == '__main__':
    main()
