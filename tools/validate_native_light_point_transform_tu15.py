#!/usr/bin/env python3
"""Compare the complete 1454D0 array transform and its raw writes on physical TU1.5."""

import random
import struct

from native_light_point_transform_replacements import BODY_SHA256
from native_shadow_frame_oracle import LIVE, Spec, main, row


REGIONS = ((0x61000000, 4096), (0x62000000, 4096))


def corpus(_image):
    rng = random.Random(0x1454d0)
    rows = []
    transform = 0x61000000
    input_address = 0x61000100
    destinations = (0x61000200, input_address, input_address + 4, transform)
    stack = 0x62000800
    for family, count in enumerate((-1, 0, 1, 4, 8)):
        for variant, target in enumerate(destinations):
            bank = bytearray(rng.randbytes(REGIONS[0][1]))
            stack_bank = bytearray(rng.randbytes(REGIONS[1][1]))
            struct.pack_into('<13f', bank, 0, *(rng.uniform(-100, 100) for _ in range(13)))
            struct.pack_into('<24f', bank, input_address - transform, *(rng.uniform(-10, 10) for _ in range(24)))
            struct.pack_into('<IIiII', stack_bank, stack - REGIONS[1][0],
                0x63000000, transform, count, input_address, target)
            if family >= 2 and variant == 3:
                struct.pack_into('<II', bank, 0, 0x80000000, 0x00000001)
                struct.pack_into('<I', bank, input_address - transform, 0x7f800000)
            gp = [rng.getrandbits(32) for _ in range(8)]
            gp[4] = stack
            xmm = [[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
            rows.append(row(f'family{family}-{variant}', 0x1454d0, gp, 0x202 | (rng.getrandbits(12) & 0x8d5),
                xmm, [0x27f, 0, 0x1f80 | (variant << 13)], list(LIVE if variant & 1 else ()),
                (bank, stack_bank), 0, REGIONS, (transform, count & 0xffffffff), max_steps=256))
    return rows


SPEC = Spec(
    name='light_point_transform', label='1454D0 light point transform', entry=0x1454d0,
    binding='native_light_point_transform_replacements', replace='replace_light_point_transform', body_pin=BODY_SHA256,
    owned={0x1454d0: 0x14555b}, entries=(0x1454d0,), regions=REGIONS, callees={},
    inputs=(
        'src/pstencil/native_tu15_light_point_transform.hpp', 'src/pstencil/native_tu15_light_point_transform_systems.hpp',
        'src/pstencil/native_tu15_light_visibility_frame.hpp', 'tools/native_light_point_transform_replacements.py',
        'tests/native_tu15_light_point_transform_tests.cpp', 'tools/validate_native_light_point_transform_tu15.py',
    ),
    reference_variable='HALO2_REFERENCE_LIGHT_VISIBILITY', reference_test_variable='HALO2_LIGHT_POINT_TRANSFORM_REFERENCE_TEST',
    census_test_variable='HALO2_LIGHT_POINT_TRANSFORM_CENSUS_TEST', census_range=(0x1454d0, 0x145558),
    helpers='''
__m128 native_load_half(M& memory, std::uint32_t address, __m128 value, unsigned offset) {
    auto bytes=std::bit_cast<std::array<std::byte,16>>(value);
    memory.read(address,std::span(bytes).subspan(offset,8));
    return std::bit_cast<__m128>(bytes);
}
void native_store_half(WorldState& state,std::uint32_t pc,std::uint32_t address,__m128 value,unsigned offset) {
    std::uint64_t bits;
    std::memcpy(&bits,reinterpret_cast<const std::byte*>(&value)+offset,8);
    w(state.memory,state.observe,pc,address,8,bits);
}
''',
)


if __name__ == '__main__':
    raise SystemExit(main(SPEC, corpus))
