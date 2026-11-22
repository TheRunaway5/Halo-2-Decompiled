#!/usr/bin/env python3
"""Compare TU1.5 render-light admission and its child-call packets on physical i386."""

import random

from native_light_admission_replacements import BODY_SHA256
import native_shadow_frame_oracle as oracle


REGIONS = tuple((base, 4096) for base in (
    0x491000, 0x4c6000, 0x4ec000, 0x4f0000, 0x61000000,
    0x61001000, 0x61002000, 0x61003000, 0x62000000,
))


def set_enabled(ctx):
    ctx.gp[0] = (ctx.gp[0] & 0xffffff00) | int(bool(ctx.mode & 1))


def set_visibility(ctx):
    ctx.gp[0] = (ctx.gp[0] & 0xffffff00) | int(bool(ctx.mode & 2))


def store_shape(ctx):
    ctx.put(ctx.entry_gp[0], 0x3f800002)
    ctx.gp[0] = (ctx.gp[0] & 0xffffff00) | int(bool(ctx.mode & 4))


def corpus(_image):
    rng = random.Random(0x31570)
    rows = []
    for mode in (0, 1, 3, 7):
        for family in range(8):
            banks = [bytearray(rng.randbytes(size)) for _, size in REGIONS]

            def put(address, value, width=4):
                for (base, size), bank in zip(REGIONS, banks, strict=True):
                    if base <= address <= base + size - width:
                        bank[address - base:address - base + width] = value.to_bytes(width, 'little')
                        return
                raise ValueError(f'Unmapped light field {address:#x}')

            cluster = 7 if family < 2 else 0xffff
            flags = (0, 0x400, 1 << 18, 1 << 5, 0x40020, 0x240420, 0x200000, 0x200400)[family]
            put(0x4ecbdc, 0x61000000)
            put(0x61000044, 0x61001000)
            put(0x61001000 + 5*272 + 4, 0xffd00014)
            put(0x61001000 + 5*272 + 72, cluster, 2)
            put(0x4f0414, 0x61002000)
            put(0x61002000 + 20*16 + 8, 0x61003000)
            put(0x61003000, flags)
            put(0x491e22, 2 if family in (1, 5) else 3, 2)
            put(0x4c6841, 1 if family >= 4 else 0, 1)
            stack = 0x62000800
            put(stack, 0x63000000)
            put(stack + 4, 0x61003100)
            put(stack + 8, 0)
            gp = [rng.getrandbits(32) for _ in range(8)]
            gp[4], gp[7] = stack, 0xffffffff if family == 0 else 0xd45e0005
            xmm = [[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
            rows.append(oracle.row(f'family{family}-{mode}', 0x31570, gp,
                0x202 | (rng.getrandbits(12) & 0x8d5), xmm, [0x27f, 0, 0x1f80],
                list(oracle.LIVE if family & 1 else ()), banks, mode, REGIONS, (0x61003100, 0), max_steps=128))
    return rows


SPEC = oracle.Spec(
    name='light_admission', label='31570 light admission', entry=0x31570,
    binding='native_light_admission_replacements', replace='replace_light_admission', body_pin=BODY_SHA256,
    owned={0x31570: 0x31650}, entries=(0x31570,), regions=REGIONS,
    callees={
        0xc5170: oracle.Callee((0x31589,), 0, set_enabled),
        0x31500: oracle.Callee((0x315c8,), 0, set_visibility),
        0xc3820: oracle.Callee((0x315d9,), 4, store_shape),
    },
    inputs=(
        'src/pstencil/native_tu15_light_admission.hpp', 'src/pstencil/native_tu15_light_admission_systems.hpp',
        'src/pstencil/native_tu15_light_visibility_frame.hpp',
        'tests/native_tu15_light_admission_tests.cpp', 'tools/validate_native_light_admission_tu15.py',
        'tools/native_light_admission_replacements.py',
    ),
    reference_variable='HALO2_REFERENCE_LIGHT_VISIBILITY', reference_test_variable='HALO2_LIGHT_ADMISSION_REFERENCE_TEST',
    census_test_variable='HALO2_LIGHT_ADMISSION_CENSUS_TEST', census_range=(0x31570, 0x3164d),
    undefined_flag_scope=('AF only after SHL ECX,4 at the 315C2 tag load, 315C8 call push and 31500 entry',),
    helpers='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){s.required(site,target,tail);}\n'
            'static void run_c5170(WorldState&s){dispatch(s,0xc5170u,0x31589u,false);}\n'
            'static void run_31500(WorldState&s){dispatch(s,0x31500u,0x315c8u,false);}\n'
            'static void run_c3820(WorldState&s){dispatch(s,0xc3820u,0x315d9u,false);}\n',
)


if __name__ == '__main__':
    oracle.UNIT = 'src/pstencil/native_light_portal_tu15_generated.cpp'
    oracle.COMMON_INPUTS = (oracle.UNIT, *oracle.COMMON_INPUTS[1:])
    raise SystemExit(oracle.main(SPEC, corpus))
