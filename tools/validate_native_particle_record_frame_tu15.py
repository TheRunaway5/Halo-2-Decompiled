#!/usr/bin/env python3
"""Compare the native 1E4C0 particle record append with unchanged TU1.5 i386."""
from validate_native_flight_property_query_frame_tu15 import FrameProfile, main
from validate_native_projectile_function_query_frame_tu15 import corpus as input_rows

REGIONS=((0x491000,4096),(0x4c2000,4096),(0x61000000,16384),(0x62000000,4096))
FRAME='src/rendering/effects/native_tu15_particle_record_frame.hpp'
DRIVER='tests/native_tu15_particle_record_frame_tests.cpp'
KERNEL='include/halo2/native_tu15_particle_record_kernel.hpp'
SOURCE=0x61002800
VALUES=(0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc0a55a,0x7fa05678,
        0xffc01234,0x00000001,0x7f7fffff,0x3e800000)
# count, payload bytes, overflow byte, direction flag
SCENARIOS=((0,0,0,0),(1,4,0,0),(5,7,1,0),(0x3ff,13,0,0),(0x400,0,0,0),(0x400,9,1,0),
           (0x7fffffff,0,0x80,0),(0xffffffff,3,0,0),(0x80000000,16,0,0),(2,33,0,1),
           (3,5,0,1),(0x401,2,0xff,1))


def corpus(image):
    rows=input_rows(image,True)[:144]
    for n,row in enumerate(rows):
        count,length,overflow,direction=SCENARIOS[n%12]
        variant=n//12
        banks={r['address']:bytearray.fromhex(r['before']) for r in row['banks']}
        banks={base:banks.get(base,bytearray(size)) for base,size in REGIONS}

        def put(address,value,width=4):
            for base,data in banks.items():
                if base<=address<=base+len(data)-width:
                    data[address-base:address-base+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
                    return
            raise ValueError(f'1E4C0 fixture escaped backed banks at {address:x}')

        for address in range(0x491e38,0x491e50,4):
            put(address,int.from_bytes(image.read(address,4),'little'))
        record=0x61001000+0x80*(variant%8)
        put(0x4c2aa0,count)
        put(0x4c2aa8,(record-count*0x70)&0xffffffff)
        put(0x4c2ab1,overflow,1)
        point=0x61003000+4*(n%8)
        put(point,VALUES[variant])
        put(point+4,VALUES[(variant+5)%12])
        put(point+8,VALUES[(variant+7)%12])
        identity=(0x12345678+0x01010101*n)&0xffffffff
        sp=row['gp'][4]
        # The physical runner publishes the return and both declared words.
        put(sp,0x63000000);put(sp+4,identity);put(sp+8,SOURCE)
        row['gp'][1]=length
        row['gp'][2]=point
        row['eflags']=(row['eflags']&~0x400)|(0x400 if direction else 0)
        row.update(entry=0x1e4c0,name=f'particle-record-{n%12}-{variant}',max_steps=160,
            matrix=identity,source=SOURCE,banks=[dict(address=base,before=banks[base].hex()) for base,_ in REGIONS])
    return rows


PROFILE=FrameProfile(0x1e4c0,64,FRAME,DRIVER,KERNEL,REGIONS,corpus,(),(
    ('unflagged-slot-multiply','g[0]=multiply_slot(s_,g[6]);','g[0]=g[6]*0x70u;'),
    ('ignored-direction','(s_.other_flags&1024u)?0u-width:width','width'),
    ('commuted-plane-sum','x[3]=add(x[3],x[2]);','x[3]=add(x[2],x[3]);'),
    ('wrong-overflow-flag','store(0x1e5a6u,0x4c2ab1u,0u,1)','store(0x1e5a6u,0x4c2ab0u,0u,1)'),
    ('modeled-shift','g[1]=shift_count(s_,g[1]);','g[1]=s_.flags.right(g[1],0x2u,4,false);'),
),extra_pins=('tools/validate_native_particle_record_frame_tu15.py',
              'src/rendering/effects/native_tu15_particle_record_flags.hpp'))


if __name__=='__main__':
    raise SystemExit(main(PROFILE))
