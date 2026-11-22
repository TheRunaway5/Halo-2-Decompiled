#!/usr/bin/env python3
"""Compare the TU1.5 visibility controller and each child call against physical i386."""

import random

from native_light_view_replacements import BODY_SHA256
from native_shadow_frame_oracle import Callee, LIVE, Spec, main, row


REGIONS = tuple((base, 4096) for base in (
    0x4ea000, 0x4ec000, 0x4cd000, 0x4f4000, 0x556000, 0x559000, 0x55c000, 0x62000000,
))


def set_view_result(ctx):
    ctx.gp[0] = ctx.mode & 3


def corpus(_image):
    rng = random.Random(0x134c00)
    rows = []
    for mode in range(4):
        for secondary in (False, True):
            for has_auxiliary in (False, True):
                banks = [bytearray(rng.randbytes(size)) for _, size in REGIONS]

                def put(address, value, width=4):
                    for (base, size), bank in zip(REGIONS, banks, strict=True):
                        if base <= address <= base + size - width:
                            bank[address - base:address - base + width] = value.to_bytes(width, 'little')
                            return
                    raise ValueError(f'Unmapped view field {address:#x}')

                view = 0x5598f8 if secondary else 0x556e18
                put(view, 0x61004000)
                put(view + 8, 0xffff if has_auxiliary else 12, 2)
                put(view + 0x2ad0, 0x556e18 if has_auxiliary else 0)
                put(0x556e18, 0x61005000)
                for epoch in (0x4eabcc, 0x4ecbd8, 0x4f44ec):
                    put(epoch, (0x7fffffff if has_auxiliary else 0xffffffff) if secondary else rng.getrandbits(32))
                stack = 0x62000800
                put(stack, 0x63000000)
                args = [0x400, 0x61000000, 0x61000400, 0x3f800000, 0x556e18, 0, 0xffff]
                for index, arg in enumerate(args, 1):
                    put(stack + index * 4, arg)
                gp = [rng.getrandbits(32) for _ in range(8)]
                gp[0], gp[4] = int(secondary), stack
                xmm = [[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
                rows.append(row(f'family{mode}-{int(secondary)}-{int(has_auxiliary)}', 0x134c00, gp,
                    0x202 | (rng.getrandbits(12) & 0x8d5), xmm, [0x27f, 0, 0x1f80],
                    list(LIVE if has_auxiliary else ()), banks, mode, REGIONS, args[:2], max_steps=128))
    return rows


SPEC = Spec(
    name='light_view', label='134C00 view collection', entry=0x134c00,
    binding='native_light_view_replacements', replace='replace_light_view', body_pin=BODY_SHA256,
    owned={0x134c00: 0x134cf3}, entries=(0x134c00,), regions=REGIONS,
    callees={
        0x135690: Callee((0x134c39,), 36, set_view_result),
        0x1373f0: Callee((0x134c48,), 0),
        0x176270: Callee((0x134c62,), 4),
        0x135870: Callee((0x134c6b,), 0),
        0x175f90: Callee((0x134c7c,), 16),
        0x135410: Callee((0x134c91,), 8),
        0x136ea0: Callee((0x134ca5,), 4),
        0x134d30: Callee((0x134cd8,), 4),
    },
    inputs=(
        'src/pstencil/native_tu15_light_view.hpp', 'src/pstencil/native_tu15_light_view_systems.hpp',
        'src/pstencil/native_tu15_light_visibility_frame.hpp',
        'tests/native_tu15_light_view_tests.cpp', 'tools/validate_native_light_view_tu15.py',
        'tools/native_light_view_replacements.py',
    ),
    reference_variable='HALO2_REFERENCE_LIGHT_VISIBILITY',
    reference_test_variable='HALO2_LIGHT_VIEW_REFERENCE_TEST',
    census_test_variable='HALO2_LIGHT_VIEW_CENSUS_TEST', census_range=(0x134c00, 0x134cf0),
    helpers='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){s.required(site,target,tail);}\n'
            'static void run_135690(WorldState&s){dispatch(s,0x135690u,0x134c39u,false);}\n'
            'static void run_1373f0(WorldState&s){dispatch(s,0x1373f0u,0x134c48u,false);}\n'
            'static void run_135870(WorldState&s){dispatch(s,0x135870u,0x134c6bu,false);}\n'
            'static void run_136ea0(WorldState&s){dispatch(s,0x136ea0u,0x134ca5u,false);}\n'
            'static void run_134d30(WorldState&s){dispatch(s,0x134d30u,0x134cd8u,false);}\n',
)


if __name__ == '__main__':
    raise SystemExit(main(SPEC, corpus))
