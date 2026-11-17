#!/usr/bin/env python3
"""Compare the private childless 3EB10 gate with unchanged original i386."""
from dataclasses import replace
import sys

from validate_native_flight_property_query_frame_tu15 import FrameProfile, main
from validate_native_projectile_function_query_frame_tu15 import corpus as input_rows

ENTRY=0x3eb10
BANKS=((0x474000,4096),(0x4c6000,4096),(0x4ec000,4096),(0x61000000,16384),(0x62000000,4096))
FRAME='src/gameplay/objects/native_tu15_indexed_datum_gate_frame.hpp'
DRIVER='tests/native_tu15_indexed_datum_gate_frame_tests.cpp'
KERNEL='include/halo2/native_tu15_indexed_datum_gate_kernel.hpp'
MAPPACK_BANKS=((0x46f000,4096),(0x4c1000,4096),(0x4e7000,4096),(0x61000000,16384),(0x62000000,4096))
MAPPACK_SHA='aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'


def corpus(image,mappack=False):
    regions=MAPPACK_BANKS if mappack else BANKS
    enabled,selected,index_cell,header_cell=(0x4c1409,0x46fb64,0x4c140c,0x4e7918) if mappack else (
        0x4c6709,0x474c24,0x4c670c,0x4ecc18)
    rows=input_rows(image,True)[:144]
    for n,row in enumerate(rows):
        scenario=n%9;variant=n//9;sp=row['gp'][4]
        banks={bank['address']:bytearray.fromhex(bank['before']) for bank in row['banks']
               if bank['address'] in {base for base,_ in regions}}
        for base,size in regions:banks.setdefault(base,bytearray(size))

        def put(address,value,width=4):
            for base,data in banks.items():
                if base<=address<=base+len(data)-width:
                    at=address-base
                    data[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
                    return
            raise ValueError(f'3EB10 fixture escaped backed banks at {address:x}')

        index=(0,1,5,-3,-17,0x80000000,0x80000005,32)[variant%8]
        row_word=(0,1,-2,32767,-32768,-127,1234,-256)[variant%8]
        table,header=0x61002600,0x61001400
        count=(4,1,2,0x7fffffff)[variant%4]
        datum=row_word if scenario!=7 else row_word+1
        if scenario==0:datum=0xffffffff
        if scenario==2:index=0xffffffff
        if scenario==4:count=0
        if scenario==5:count=(0x80000000,-3)[variant%2]
        if scenario==8:
            # The caller's saved ESI becomes the live row word.
            index=0
            table=sp-4
            datum=((row['gp'][6]&0xffff)^0x8000)-0x8000
            if datum==-1:
                row['gp'][6]^=2
                datum=-3
        row['gp'][2]=datum&0xffffffff
        put(enabled,0 if scenario==1 else (1,0x80,0xff)[variant%3],1)
        put(index_cell,index)
        put(selected,-1 if scenario==3 else (0,2,-17)[variant%3],2)
        put(header_cell,header);put(header+0xac,count);put(header+0xb0,table)
        if scenario!=8:put((table+index*2)&0xffffffff,row_word,2)
        # The shared physical observer writes these two legacy arguments.
        put(sp+4,0);put(sp+8,0)
        row.update(entry=ENTRY,name=f'indexed-datum-gate-{scenario}-{variant}',max_steps=40,
                   matrix=0,source=0,banks=[dict(address=base,before=banks[base].hex()) for base,_ in regions])
    return rows


PROFILE=FrameProfile(ENTRY,25,FRAME,DRIVER,KERNEL,BANKS,corpus,(),(
    ('unsigned-row','(read(0x3eb4fu,g[1]+g[6]*2u,2)^0x8000u)-0x8000u','read(0x3eb4fu,g[1]+g[6]*2u,2)'),
    ('unsigned-count','return !s_.flags.zf&&s_.flags.sf==s_.flags.of;','return !s_.flags.zf;'),
    ('wrong-row-stride','g[1]+g[6]*2u,2','g[1]+g[6]*4u,2'),
    ('discard-saved-index','if(saved)s_.gp[6]=pop(0x3eb59u);','if(saved)pop(0x3eb59u);'),
),extra_pins=('tools/validate_native_indexed_datum_gate_frame_tu15.py',))

if __name__=='__main__':
    mappack='--mappack11' in sys.argv
    if mappack:
        sys.argv.remove('--mappack11')
    profile=replace(PROFILE,driver='tests/native_tu15_indexed_datum_gate_mappack_frame_tests.cpp',
                    regions=MAPPACK_BANKS,inputs=lambda image:corpus(image,True),image_sha256=MAPPACK_SHA) if mappack else PROFILE
    raise SystemExit(main(profile))
