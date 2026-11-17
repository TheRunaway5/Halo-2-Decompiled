#!/usr/bin/env python3
"""Compare the private 165590 query with original TU1.5 i386 packets."""
from validate_native_flight_property_query_frame_tu15 import FrameProfile, main
from validate_native_projectile_function_query_frame_tu15 import REGIONS, corpus as projectile_inputs

ENTRY = 0x165590
BANKS = (*REGIONS, (0x4f3000, 4096))


def corpus(image):
    rows = projectile_inputs(image,True)[:96]
    for index,row in enumerate(rows):
        banks = {bank['address']:bytearray.fromhex(bank['before']) for bank in row['banks']}
        banks[0x4f3000] = bytearray(4096)

        def locate(address,width):
            for base,data in banks.items():
                if base<=address<=base+len(data)-width:
                    return data,address-base
            raise ValueError('165590 fixture escaped its banks')

        def get(address):
            data,offset=locate(address,4)
            return int.from_bytes(data[offset:offset+4],'little')

        def put(address,value,width=4):
            data,offset=locate(address,width)
            data[offset:offset+width]=value.to_bytes(width,'little')

        datum=row['matrix']
        slots=get(get(0x4ecbd0)+0x44)
        body=get((slots+(datum&0xffff)*12+8)&0xffffffff)
        definition=get(get(0x4f0414)+(get(body)&0xffff)*16+8)
        state=0x61000c00
        put(0x4f3218,state)
        put(state+0x180,8 if index%4==0 else 9)
        put(definition+0x290,(0,2,0xffff,1)[index%4],2)
        row['gp'][1]=0xffffffff if index%4==1 else datum
        row.update(name=f'definition-mode-{index}',entry=ENTRY,max_steps=100)
        row['banks']=[dict(address=base,before=banks[base].hex()) for base,_ in BANKS]
    return rows


PROFILE=FrameProfile(ENTRY,23,'src/gameplay/objects/native_tu15_object_definition_mode_query_frame.hpp',
    'tests/native_tu15_object_definition_mode_query_frame_tests.cpp',
    'include/halo2/native_tu15_object_definition_mode_query_kernel.hpp',BANKS,corpus,(),(
        ('wrong-mode','s_.flags.minus(g[6],9u,4);','s_.flags.minus(g[6],8u,4);'),
        ('missing-datum-route','return !s_.flags.zf;}','return false;}'),
        ('wide-return','s_.set(0,1u,1,0);','s_.set(0,1u,4,0);'),
    ),extra_pins=('tools/validate_native_object_definition_mode_query_frame_tu15.py',))

if __name__=='__main__':
    raise SystemExit(main(PROFILE))
