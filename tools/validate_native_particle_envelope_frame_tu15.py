#!/usr/bin/env python3
"""Compare the native 252EC0 particle envelope with unchanged TU1.5 i386."""
import struct

from validate_native_flight_property_query_frame_tu15 import FrameProfile, main
from validate_native_projectile_function_query_frame_tu15 import corpus as input_rows

REGIONS=((0x46a000,4096),(0x51d000,4096),(0x61000000,16384),(0x62000000,4096))
FRAME='src/rendering/effects/native_tu15_particle_envelope_frame.hpp'
DRIVER='tests/native_tu15_particle_envelope_frame_tests.cpp'
KERNEL='include/halo2/native_tu15_particle_envelope_kernel.hpp'
OVERRIDE=0x61003000

NAN=0x7fc0a55a
SIGNALING=0x7fa05678
NEGATIVE_NAN=0xffc01234


def bits(value):
    return struct.unpack('<I',struct.pack('<f',value))[0]


# start, attack length, attack rate, end, release length, release rate
PARAMETERS=(
    (bits(0.0),bits(1.0),bits(1.0),bits(8.0),bits(2.0),bits(0.5)),
    (bits(1.0),bits(0.5),bits(2.0),bits(4.0),bits(1.0),bits(1.0)),
    (bits(-2.0),bits(2.0),bits(0.5),bits(6.0),bits(4.0),bits(0.25)),
    (0x80000000,bits(0.25),bits(4.0),bits(1.0),bits(0.25),bits(4.0)),
    (bits(100.0),bits(16.0),bits(0.0625),bits(164.0),bits(32.0),bits(0.03125)),
    (0x00000001,bits(1.0),bits(1.0),bits(3.0),bits(1.0),0x7f800000),
    (bits(0.0),0x00000000,bits(1.0),bits(2.0),0x00000000,bits(1.0)),
    (bits(0.0),bits(1.0),NAN,bits(4.0),bits(1.0),SIGNALING),
    (bits(0.0),0x7f800000,bits(1.0),0x7f800000,0x7f800000,bits(1.0)),
)


def times(start,attack,end,release):
    s,a,e,r=(struct.unpack('<f',struct.pack('<I',v))[0] for v in (start,attack,end,release))
    finite=lambda x:x==x and abs(x)!=float('inf')
    pick=lambda x,default:bits(x) if finite(x) else default
    return (
        pick(s+a/2 if finite(a) else s+1,bits(0.5)),   # 0 attack, null override
        pick(s+a/2 if finite(a) else s+1,bits(0.5)),   # 1 attack, clear override
        pick(s+a/2 if finite(a) else s+1,bits(0.5)),   # 2 set override
        start,                                          # 3 at start
        pick(s-1,bits(-1.0)),                          # 4 before start
        end,                                            # 5 at end
        pick(e+1,0x7f800000),                          # 6 after end
        pick(s+a/4 if finite(a) else s+1,bits(0.25)),  # 7 early attack
        pick(s+a,bits(1.0)),                           # 8 attack boundary
        pick((s+a+e-r)/2 if finite(a) and finite(r) else s+1,bits(1.0)),  # 9 sustain
        pick(e-r,bits(2.0)),                           # 10 release boundary
        pick(e-r/2 if finite(r) else e-1,bits(1.5)),   # 11 release
        NAN,                                            # 12 quiet NaN time
        SIGNALING,                                      # 13 signaling NaN time
        NEGATIVE_NAN,                                   # 14 negative NaN time
        0xff800000,                                     # 15 negative infinity time
    )


def corpus(image):
    rows=input_rows(image,True)[:144]
    for n,row in enumerate(rows):
        scenario=n%16
        variant=n//16
        start,attack,attack_rate,end,release,release_rate=PARAMETERS[variant]
        banks={r['address']:bytearray.fromhex(r['before']) for r in row['banks']}
        banks={base:banks.get(base,bytearray(size)) for base,size in REGIONS}

        def put(address,value,width=4):
            for base,data in banks.items():
                if base<=address<=base+len(data)-width:
                    data[address-base:address-base+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
                    return
            raise ValueError(f'252EC0 fixture escaped backed banks at {address:x}')

        record=0x61001000+0x40*variant+4*(n%4)
        put(0x46a0d0,int.from_bytes(image.read(0x46a0d0,4),'little'))
        put(0x51df50,0 if scenario==0 else OVERRIDE+(n%8))
        put(OVERRIDE+(n%8)+5,(0,1,0x80,0xff)[variant%4] if scenario==2 else 0,1)
        put(record+0x18,start)
        put(record+0x1c,attack)
        put(record+0x20,attack_rate)
        put(record+0x24,end)
        put(record+0x28,release)
        put(record+0x2c,release_rate)
        row['gp'][0]=record
        row['xmm'][1][0]=times(start,attack,end,release)[scenario]
        sp=row['gp'][4]
        # The physical runner publishes the return and two declared words.
        put(sp,0x63000000);put(sp+4,0);put(sp+8,0)
        row.update(entry=0x252ec0,name=f'particle-envelope-{scenario}-{variant}',max_steps=60,
            matrix=0,source=0,banks=[dict(address=base,before=banks[base].hex()) for base,_ in REGIONS])
    return rows


PROFILE=FrameProfile(0x252ec0,30,FRAME,DRIVER,KERNEL,REGIONS,corpus,(),(
    ('wrong-override-byte','word(g[1]+5u,1)','word(g[1]+4u,1)'),
    ('wrong-start-compare','below_or_equal(s_.xmm[1],read(0x252ed4u','below_or_equal(s_.xmm[0],read(0x252ed4u'),
    ('commuted-release-difference','x[0]=subtract(x[0],x[1]);','x[0]=subtract(x[1],x[0]);'),
    ('wrong-release-rate','s_.gp[0]+0x2cu','s_.gp[0]+0x20u'),
    ('kept-upper-lanes','s_.xmm[0]=_mm_setzero_ps();','s_.xmm[0]=_mm_move_ss(s_.xmm[0],_mm_setzero_ps());'),
),extra_pins=('tools/validate_native_particle_envelope_frame_tu15.py',))


if __name__=='__main__':
    raise SystemExit(main(PROFILE))
