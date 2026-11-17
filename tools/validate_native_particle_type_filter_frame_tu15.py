#!/usr/bin/env python3
"""Compare 17DE60 and its genuine BCDF0 child against unchanged TU1.5."""
from validate_native_flight_property_query_frame_tu15 import FrameProfile, main
from validate_native_projectile_function_query_frame_tu15 import corpus as input_rows

REGIONS=((0x4ec000,4096),(0x4f5000,4096),(0x4f7000,4096),
         (0x61000000,16384),(0x62000000,4096))
FRAME='src/rendering/effects/native_tu15_particle_type_filter_frame.hpp'
DRIVER='tests/native_tu15_particle_type_filter_frame_tests.cpp'
KERNEL='include/halo2/native_tu15_particle_type_filter_kernel.hpp'


def corpus(image):
    rows=input_rows(image,True)[:144]
    for n,row in enumerate(rows):
        scenario=n%9
        variant=n//9
        sp=row['gp'][4]
        declared={base for base,_ in REGIONS}
        banks={r['address']:bytearray.fromhex(r['before']) for r in row['banks'] if r['address'] in declared}
        for base,size in REGIONS:
            banks.setdefault(base,bytearray(size))

        def put(address,value,width=4):
            for base,data in banks.items():
                if base<=address<=base+len(data)-width:
                    at=address-base
                    data[at:at+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
                    return
            raise ValueError(f'17DE60 original fixture escaped backed banks at {address:x}')

        particle_header=0x61000100
        object_header=0x61000200
        type_header=0x61000300
        particle=0x61000800+(variant&7)
        object_record=0x61000c00+(variant&7)
        object_body=0x61001000+(variant&7)
        type_record=0x61002000+(variant&7)
        particle_index=(0,1,255,65535)[variant%4]
        object_index=(0,1,31,65535)[(variant//2)%4]
        type_index=(0,1,255,65535)[(variant//3)%4]
        salt=(1,0x7fff,0x8000,0xfffe)[variant%4]
        kind=(0,1,32,33)[variant%4] if scenario!=4 else 7
        type_word=(0,1,-1,32767,-32768,-127,321,-256)[variant%8]
        desired=type_word if scenario in (6,8) else type_word+1
        row['gp'][0]=(row['gp'][0]&0xffff0000)|particle_index
        put(0x4f720c,particle_header)
        put(particle_header+0x44,particle-particle_index*0x190)
        put(particle+0x48,0xffffffff if scenario==0 else (salt<<16)|object_index)
        put(0x4ecbd0,object_header)
        put(object_header+0x38,object_index if scenario==1 else object_index+1)
        put(object_header+0x24,12)
        put(object_header+0x44,object_record-object_index*12)
        put(object_record,0 if scenario==2 else salt^1 if scenario==3 else salt,2)
        put(object_record+3,kind,1)
        put(object_record+8,object_body)
        put(object_body+0x13c,0xffffffff if scenario==5 else (row['gp'][2]&0xffff0000)|type_index)
        put(0x4f54f4,type_header)
        put(type_header+0x44,type_record-type_index*0x21c)
        put(type_record+0x28,type_word,2)
        put(sp+4,desired);put(sp+8,0)
        row.update(entry=0x17de60,name=f'particle-type-{scenario}-{variant}',max_steps=90,
            matrix=desired&0xffffffff,source=0,
            banks=[dict(address=base,before=banks[base].hex()) for base,_ in REGIONS])
    return rows


PROFILE=FrameProfile(0x17de60,27,FRAME,DRIVER,KERNEL,REGIONS,corpus,(),(
    ('wrong-particle-stride','g[0],0x190u','g[0],0x18cu'),
    ('wrong-lookup-type-mask','push(0x17de79u,3u)','push(0x17de79u,1u)'),
    ('wrong-signed-type','(read(0x17dea9u,g[0]+g[2]+0x28u,2)^0x8000u)-0x8000u',
     'read(0x17dea9u,g[0]+g[2]+0x28u,2)'),
    ('wrong-ret-cleanup','pop(exit);s_.gp[4]+=4u;','pop(exit);s_.gp[4]+=8u;'),
),extra_pins=('tools/validate_native_particle_type_filter_frame_tu15.py',),
    child_edges=((0x17de7d,0xbcdf0,0x17de82,35),),
    explicit_reads=((0xbce3d,bytes.fromhex('8554240c')),))


if __name__=='__main__':
    raise SystemExit(main(PROFILE))
