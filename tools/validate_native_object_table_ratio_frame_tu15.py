#!/usr/bin/env python3
"""Compare the private TU1.5 24B430 leaf against unchanged original i386."""
from validate_native_flight_property_query_frame_tu15 import FrameProfile, main
from validate_native_object_record_ratio_frame_tu15 import VALUES
from validate_native_projectile_function_query_frame_tu15 import REGIONS, corpus as projectile_inputs

ENTRY = 0x24b430
BANKS = (*REGIONS,(0x4f3000,4096),(0x51d000,4096),(0x52d000,4096))


def corpus(image):
    rows=projectile_inputs(image,True)[:192]
    for n,row in enumerate(rows):
        banks={bank['address']:bytearray.fromhex(bank['before']) for bank in row['banks']}
        banks.update({base:bytearray(size) for base,size in BANKS if base not in banks})

        def locate(address,width):
            for base,data in banks.items():
                offset=address-base
                if 0<=offset<=len(data)-width:
                    return data,offset
            raise ValueError('24B430 fixture escaped its declared banks')

        def get(address):
            data,offset=locate(address,4)
            return int.from_bytes(data[offset:offset+4],'little')

        def put(address,value,width=4):
            data,offset=locate(address,width)
            data[offset:offset+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')

        datum=row['matrix']
        slots=get(get(0x4ecbd0)+0x44)
        body=get((slots+(datum&0xffff)*12+8)&0xffffffff)
        state,table,scale=0x61000c00,0x61003200,0x61003800
        route=n%8
        index=(-1,1,-2,0,13,-3,1,17)[route]
        count=(-1,0,-32768,32767,1,2,-2,0)[route]
        denominator=(0,3,-3,0,-32768,32767,-1,2)[(n//8+route)%8]
        row['gp'][0]=datum
        put(0x4f3218,state)
        put(state+0x244,denominator,2)
        put(body+0x17e,index,2)
        put(0x52d710,table)
        put(table+0x170+index,8 if route in (1,2) else 4,1)
        put(table+0x1a0+index*2,count,2)
        put(0x51df54,scale)
        put(scale+4,VALUES[(n//3)%len(VALUES)])
        row.update(entry=ENTRY,name=f'object-table-ratio-{n}',max_steps=100)
        row['banks']=[dict(address=base,before=banks[base].hex()) for base,_ in BANKS]
    return rows


PROFILE=FrameProfile(ENTRY,34,'src/gameplay/objects/native_tu15_object_table_ratio_frame.hpp',
    'tests/native_tu15_object_table_ratio_frame_tests.cpp',
    'include/halo2/native_tu15_object_table_ratio_kernel.hpp',BANKS,corpus,(),(
        ('wrong-row-flag','g[0]+g[1]+0x170u,1)&8u','g[0]+g[1]+0x170u,1)&4u'),
        ('missing-zero-substitution','s_.flags.zf?1u:signed_word(ax)','signed_word(ax)'),
        ('unsigned-state','s_.flags.zf?1u:signed_word(ax)','s_.flags.zf?1u:ax'),
        ('dropped-negative-clamp','else g[0]=s_.flags.logic(0u,4);','else g[0]=signed_word(cx);'),
    ),extra_pins=('tools/validate_native_object_table_ratio_frame_tu15.py',
                  'tools/validate_native_object_record_ratio_frame_tu15.py'))


if __name__=='__main__':
    raise SystemExit(main(PROFILE))
