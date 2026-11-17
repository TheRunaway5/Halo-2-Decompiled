#!/usr/bin/env python3
"""Compare the original 30DE60 plane branches after segment or vertex reduction."""
import copy
import random
import struct
import types
from pathlib import Path

import validate_native_simplex_hull_paired_call_tu15 as paired

ENTRIES=(0x30e606,0x30e68b,0x30e6bf,0x30e7a7,0x30e7c7)
EXITS={0x30e89b,0x30e8a9,0x30e8b0}
DRIVER=paired.ROOT/'tests/native_tu15_simplex_hull_plane_tests.cpp'


def parent(pc):
    return 0x30e606<=pc<=0x30e899


def inputs(image):
    seeds=paired.inputs(image);rows=[];base=paired.REGIONS[0][0];rng=random.Random(0x30e606)
    for route,entry in enumerate(ENTRIES):
        for n in range(32):
            row=copy.deepcopy(seeds[(route*32+n)%len(seeds)]);gp=row['gp'];context,sp=gp[6],gp[4]
            raw=bytearray.fromhex(row['banks'][0]['before'])
            def v(address,xyz,w=0.):
                struct.pack_into('<4f',raw,address-base,*xyz,w)
            struct.pack_into('<2I',raw,context-base,3 if route==3 else 2,3 if route==1 else 2)
            gp[3]=context+0x20
            gp[0]=(-1,-6,-7,-8)[n%4]&0xffffffff
            if route==0:
                v(sp+0xb0,(1.,0.,0.));v(sp+0xc0,(0.,1.,0.))
                v(context+0xa0,(0.,0.,0.));v(context+0x20,(0.,0.,-1. if n&1 else 1.))
            else:
                segment,opposite=(0x20,0xa0)if route>=3 else (0xa0,0x20)
                v(context+segment,(0.,0.,0.));v(context+segment+0x10,(1.,0.,0.))
                v(context+segment+0x20,(.875,.125,.0625))
                v(context+opposite,((-1.,.5,2.)[n%3],.125,1.))
                v(context+segment+0x50,(2.,3.,4.));v(context+segment+0x60,(5.,6.,7.))
            if n>=20:
                for offset in (0x20,0x30,0x40,0x60,0x70,0xa0,0xb0,0xc0,0xe0,0xf0,0x100):
                    v(context+offset,tuple(rng.randrange(-16,17)/8 for _ in range(3)),rng.randrange(-4,5)/8)
            row['banks'][0]['before']=raw.hex()
            row.update(id=len(rows),name=f'hull-plane-{route}-{n}',entry=entry,stop_before_entries=sorted(EXITS))
            rows.append(row)
    return rows


if __name__=='__main__':
    globals_=paired.main.__globals__.copy()
    globals_.update(__file__=str(Path(__file__).resolve()),ENTRIES=ENTRIES,EXITS=EXITS,CHILDREN={},
                    DRIVER=DRIVER,CHECK_CALL=(0x30e67c,'0f28056019'),
                    EXTRA_INPUTS=(Path(paired.__file__).resolve(),),parent=parent,inputs=inputs,
                    SCOPE='All five backed mid-owner plane entries, original cross-product/sign/collapse paths, and three exact output continuations.')
    run=types.FunctionType(paired.main.__code__,globals_,paired.main.__name__,paired.main.__defaults__,paired.main.__closure__)
    raise SystemExit(run())
