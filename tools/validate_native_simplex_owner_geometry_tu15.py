#!/usr/bin/env python3
"""Whole-entry polygon geometry witnesses for the remaining simplex branches."""
import copy
import random
import struct
from pathlib import Path

import validate_native_simplex_owner_frame_tu15 as owner


def inputs(image,smoke):
    cases=owner.inputs(image,False)
    base=owner.seed.BASE;rows=[];rng=random.Random(0x30f381)
    for n in range(16 if smoke else 96):
        row=copy.deepcopy(cases[n%len(cases)]);bank=next(b for b in row['banks']if b['address']==base)
        raw=bytearray.fromhex(bank['before']);sp,work=row['gp'][4],row['gp'][1]
        _,first,second,relative,cache,_=struct.unpack_from('<6I',raw,sp-base)
        def vec(at,point):struct.pack_into('<4f',raw,at-base,*point,0.)
        counts=((1,3),(3,1),(1,3),(3,1),(2,3),(3,2),(1,4),(4,1))
        a,b=counts[n%len(counts)]
        raw[cache+8-base:cache+12-base]=bytes((a,b,4,4));raw[work+12-base]=1
        if n<16:
            # A point facing a cached triangle, with the fourth actual
            # support vertex behind or ahead of the cached feature plane.
            offset=(.125,.25,.375,.5)[n%4];z=(.5,1.,2.,-2.)[(n//4)%4]
            point=[(offset,.25,z)]*4
            triangle=[(0.,0.,0.),(1.,0.,0.),(0.,1.,0.),(offset,.25,z*.5)]
            a,b=(1,3)if n%2==0 else (3,1)
            raw[cache+8-base:cache+12-base]=bytes((a,b,1 if a==1 else 4,1 if b==1 else 4))
            struct.pack_into('<I',raw,first+0x14-base,1 if a==1 else 4)
            struct.pack_into('<I',raw,second+0x14-base,1 if b==1 else 4)
            shapes=(point,triangle)if a==1 else (triangle,point)
            vec(relative+48,(0.,0.,0.))
        else:
            shapes=tuple([tuple(rng.randrange(-32,33)/8 for _ in range(3))for _ in range(4)]for _ in range(2))
            if n%8 in (0,1):
                shape=shapes[1]if n%8==0 else shapes[0];shape[1]=shape[2]=shape[0]
            vec(relative+48,tuple(rng.randrange(-8,9)/8 for _ in range(3)))
        for object_,points in zip((first,second),shapes,strict=True):
            for i,p in enumerate(points):vec(object_+0x20+16*i,p)
        bank['before']=raw.hex();row.update(id=len(rows),name=f'simplex-geometry-{a}-{b}-{n}')
        rows.append(row)
    return rows


if __name__=='__main__':
    raise SystemExit(owner.run_validation(inputs,subject=__file__,additional_inputs=(Path(owner.__file__).resolve(),)))
