#!/usr/bin/env python3
"""Compare the private TU1.5 107140 query against unchanged original i386."""
from validate_native_flight_property_query_frame_tu15 import FrameProfile, main
from validate_native_projectile_function_query_frame_tu15 import REGIONS, corpus as projectile_inputs

ENTRY = 0x107140
BANKS = (*REGIONS, (0x51d000, 4096))
VALUES = (0x3f800000, 0xbf800000, 0, 0x80000000, 1, 0x807fffff, 0x00800000,
          0x7f7fffff, 0x7f800000, 0xff800000, 0x7fc01234, 0xffc0a55a, 0x7fa05678)


def corpus(image):
    rows = projectile_inputs(image, True)[:192]
    for n, row in enumerate(rows):
        banks = {bank['address']:bytearray.fromhex(bank['before']) for bank in row['banks']}
        banks[0x51d000] = bytearray(4096)

        def locate(address, width):
            for base, data in banks.items():
                if base <= address <= base+len(data)-width:
                    return data, address-base
            raise ValueError('107140 fixture escaped its declared banks')

        def get(address):
            data, offset = locate(address, 4)
            return int.from_bytes(data[offset:offset+4], 'little')

        def put(address, value, width=4):
            data, offset = locate(address, width)
            data[offset:offset+width] = (value&((1<<(width*8))-1)).to_bytes(width, 'little')

        datum = row['matrix']
        slots = get(get(0x4ecbd0)+0x44)
        body = get((slots+(datum&0xffff)*12+8)&0xffffffff)
        definition = get(get(0x4f0414)+(get(body)&0xffff)*16+8)
        records, scale = 0x61003300, 0x61003800
        variant = n % 8
        index, count, kind = ((0,0,1),(2,1,1),(1,2,2),(0,1,0xff),
                              (-1,0,1),(-2,-1,2),(1,2,1),(-1,-1,1))[variant]
        if n >= 64:
            index, count, kind = (-2 if n&1 else 2), 3, 1
        row['gp'][0] = datum
        argument = (row['gp'][7]&0xffff0000)|(index&0xffff)
        row.update(name=f'object-record-ratio-{n}', entry=ENTRY, matrix=argument, max_steps=100)
        put(row['gp'][4]+4, argument)
        put(definition+0x2c8, count)
        put(definition+0x2cc, records)
        put(body+0x20c+index*12, kind, 1)
        put(body+0x20e+index*12, (-32768,-9,0,1,31,32767)[n%6], 2)
        put(0x51df54, scale)
        put(records+index*64+0x1c, VALUES[n%len(VALUES)])
        put(scale+4, VALUES[(n//5+7)%len(VALUES)])
        if n >= 64:
            # Directed two-NaN frontiers separate destination/source priority
            # at both MULSS and SUBSS; their values are supplied to original.
            if n%8 == 0:
                put(records+index*64+0x1c, 0x7fc03333)
                put(scale+4, 0xffc04444)
                put(0x46a0d0, 0x7fc05555)
            elif n%8 == 1:
                put(records+index*64+0x1c, 0x7fa02222)
                put(scale+4, 0x7fc01111)
                put(0x46a0d0, 0xffa06666)
        row['banks'] = [dict(address=base,before=banks[base].hex()) for base,_ in BANKS]
    return rows


PROFILE = FrameProfile(ENTRY,36,'src/gameplay/objects/native_tu15_object_record_ratio_frame.hpp',
    'tests/native_tu15_object_record_ratio_frame_tests.cpp',
    'include/halo2/native_tu15_object_record_ratio_kernel.hpp',BANKS,corpus,(),(
        ('added-lower-bound','return !s_.flags.zf&&s_.flags.sf==s_.flags.of;',
         'return !(g[0]&0x80000000u)&&!s_.flags.zf&&s_.flags.sf==s_.flags.of;'),
        ('wrong-stack-cleanup','pop(ret_pc);s_.gp[4]+=4u;','pop(ret_pc);s_.gp[4]+=8u;'),
        ('software-shift-residue','g[0]=shift_eax<6>(g[0]);','g[0]=s_.flags.shift(g[0],6,4);'),
        ('commuted-nan-multiply','asm volatile("mulss %1,%0":"+x"(s_.xmm[1]):"x"(scale));',
         'auto swapped=scale;asm volatile("mulss %1,%0":"+x"(swapped):"x"(s_.xmm[1]));'
         's_.xmm[1]=_mm_move_ss(s_.xmm[1],swapped);'),
    ),extra_pins=('tools/validate_native_object_record_ratio_frame_tu15.py',))


if __name__ == '__main__':
    raise SystemExit(main(PROFILE))
