#!/usr/bin/env python3
"""Whole-entry triangle and genuine support geometry with opposed tips."""
import copy
import struct
from pathlib import Path

import validate_native_simplex_owner_frame_tu15 as owner


def inputs(image,smoke):
    bases=owner.inputs(image,False);base=owner.seed.BASE;rows=[]
    for n in range(8 if smoke else 32):
        row=copy.deepcopy(bases[n]);bank=next(b for b in row['banks']if b['address']==base)
        raw=bytearray.fromhex(bank['before']);sp,work=row['gp'][4],row['gp'][1]
        _,first,second,relative,cache,_=struct.unpack_from('<6I',raw,sp-base)
        def vec(at,point):struct.pack_into('<4f',raw,at-base,*point,0.)
        raw[cache+8-base:cache+12-base]=bytes((1,3,1,4));raw[work+12-base]=1
        struct.pack_into('<I',raw,first+0x14-base,1);struct.pack_into('<I',raw,second+0x14-base,5)
        struct.pack_into('<4H',raw,cache-base,0,0,2 if n&1 else 1,1 if n&1 else 2)
        z=(.5,2.,-1.,-4.)[(n//2)%4]
        vec(first+0x20,((n//8)*.125,0.,z));vec(relative+48,(0.,0.,0.))
        points=((-1.,-1.,0.),(1.,-1.,0.),(0.,1.,0.),(0.,0.,abs(z)*.5),(0.,0.,-abs(z)*.5))
        for i,p in enumerate(points):vec(second+0x20+16*i,p)
        bank['before']=raw.hex();row.update(id=len(rows),name=f'simplex-triangle-support-{n}')
        rows.append(row)
    return rows


if __name__=='__main__':
    raise SystemExit(owner.run_validation(inputs,subject=__file__,additional_inputs=(Path(owner.__file__).resolve(),),
                                          witness_pcs=(0x30f317,0x30f356,0x30f381,0x30f3ae)))
