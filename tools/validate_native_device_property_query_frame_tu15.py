#!/usr/bin/env python3
"""Compare the TU1.5 10BE20 device property frame with original hardware."""
from validate_native_flight_property_query_frame_tu15 import FrameProfile, main
from validate_native_projectile_function_query_frame_tu15 import corpus as projectile_inputs

ENTRY=0x10be20
REGIONS=((0x46a000,4096),(0x4ec000,4096),(0x4f0000,4096),(0x51d000,4096),(0x61000000,16384),(0x62000000,4096))


def corpus(image):
    rows=projectile_inputs(image,True)[:384]
    keys=(0x5000561,0x40005ab,0x5000081,0x6000560,0x8000080,0xf00055e,0x1200055f,0)
    values=(0,0x3f800000,0xbf800000,0x3e800000,0x40000000,0x7fc01234,0x7fa05678,1,0x7f800000,0x80000000,0x3f000000)
    for n,row in enumerate(rows):
        banks={bank['address']:bytearray.fromhex(bank['before'])for bank in row['banks']}
        banks[0x51d000]=bytearray(4096)
        def locate(address,width):
            for base,data in banks.items():
                if base<=address<=base+len(data)-width:return data,address-base
            raise ValueError('10BE20 input escaped declared banks')
        def get(address):
            data,at=locate(address,4);return int.from_bytes(data[at:at+4],'little')
        def put(address,value,width=4):
            data,at=locate(address,width);data[at:at+width]=value.to_bytes(width,'little')
        branch=n%8;v=n//8
        key=keys[branch] if branch!=7 else (0,0x8000560,0xffffffff,0x7fffffff)[v%4]
        sp=row['gp'][4];slots=get(get(0x4ecbd0)+0x44)
        body=get((slots+(row['matrix']&0xffff)*12+8)&0xffffffff)
        definition=get(get(0x4f0414)+(get(body)&0xffff)*16+8)
        row.update(id=n,name=f'device-query-{n}-key{key:x}',entry=ENTRY,source=key,max_steps=250)
        put(sp+8,key)
        for constant in (0x46a0d0,0x46a0ec,0x46a194):
            put(constant,int.from_bytes(image.read(constant,4),'little'))
        put(0x51df54,0x61000700);put(0x61000704,values[(v+1)%len(values)])
        put(body+0x148,(0x0001,0xffff,0x100,0x7fff,0x8000)[v%5],2)
        put(definition+0x10c,values[v%len(values)])
        put(body+0x134,values[(v+1)%len(values)])
        put(body+0x140,values[v%len(values)])
        put(body+0xaa,7 if v%8!=3 else 2,1)
        put(body+0x13c,0xffffffff if v%9==4 else 0x70300002)
        put(0x4ecbf8,0x61000180);put(0x610001c4,0x61000600)
        put(0x6100061a,(0,1,3,2)[(v//2)%4],2)
        put(body+0x1cc,(0,1,8,0x10,0x18,4,0,0x10)[v%8])
        put(body+0x144,values[(v//3)%len(values)])
        put(body+0x12c,(0,8,0x28)[v%3])
        put(body+0x164,values[(v+4)%len(values)])
        put(definition+0xc8,values[(v+6)%len(values)])
        put(body+0x138,values[v%len(values)])
        put(definition+0xc0,values[(v+3)%len(values)])
        row['live_x87']=row['live_x87'][:v%6]
        row['banks']=[dict(address=base,before=banks[base].hex())for base,_ in REGIONS]
    return rows


DEVICE=FrameProfile(ENTRY,171,'src/gameplay/objects/native_tu15_device_property_query_frame.hpp',
    'tests/native_tu15_device_property_query_frame_tests.cpp','include/halo2/native_tu15_device_property_query_kernel.hpp',
    REGIONS,corpus,((0x10bfcf,2),(0x10bfdf,1),(0x10bfe9,2),(0x10bfed,1),(0x10bfff,1),
                    (0x10c013,1),(0x10c019,1),(0x10c052,1),(0x10c058,1)),(
        ('wrong-lahf-test','s_.reg(0,1,8)&0x44u','s_.reg(0,1,8)&0x40u'),
        ('wrong-register-divide','fdiv %%st(1),%%st','fmul %%st(1),%%st'),
        ('wrong-definition-reread','div_extended(0x10c013u,g[2]+0xc8u)','div_extended(0x10c013u,g[2]+0xc0u)'),
        ('missed-spill-pop','fp_.stage_single(true)','fp_.stage_single(false)'),
        ('early-spill-retirement','Base::store(pc,s_.gp[4]+8u,pending.bits,[&]{fp_.commit(pending);},true);',
         'fp_.commit(pending);Base::store(pc,s_.gp[4]+8u,pending.bits,[]{},true);'),
        ('wrong-ret-cleanup','s_.gp[4]+=16u','s_.gp[4]+=12u'),
    ),after_x87=((0x10bfe9,0x10bfed),),
    extra_pins=('tools/validate_native_device_property_query_frame_tu15.py',))

if __name__=='__main__':raise SystemExit(main(DEVICE))
