#!/usr/bin/env python3
"""Compare actual 30DE60 calls and feature selection through genuine Triangle."""
import copy
import random
import struct
import types
from pathlib import Path

import validate_native_simplex_hull_paired_call_tu15 as paired

ENTRIES=(0x30e3a7,0x30e504)
EXITS={0x30e3d3,0x30e530,0x30e68b,0x30e7a7,0x30e8b0}
DRIVER=paired.ROOT/'tests/native_tu15_simplex_hull_triangle_call_tests.cpp'


def parent(pc):
    return 0x30e3a7<=pc<=0x30e3cd or 0x30e504<=pc<=0x30e52a


def inputs(image):
    if image.read(0x30e513,5).hex()!='e8d8eeffff':
        raise ValueError('Actual first triangle CALL drift')
    seeds=paired.inputs(image);rows=[];base=paired.REGIONS[0][0];rng=random.Random(0x30e3a7)
    for first,entry in enumerate(ENTRIES):
        for n in range(40):
            row=copy.deepcopy(seeds[len(rows)]);context=row['gp'][6]
            raw=bytearray.fromhex(row['banks'][0]['before'])
            triangle,point=(0x20,0xa0)if first else (0xa0,0x20)
            def v(offset,xyz):
                struct.pack_into('<4f',raw,context-base+offset,*xyz,0.)
            struct.pack_into('<2I',raw,context-base,3 if first else 1,1 if first else 3)
            vertices=[(0.,0.,0.),(1.,0.,0.),(0.,1.,0.)]
            origin=((.25,.25,.25),(2.,2.,2.),(-1.,0.,0.),(.125,.125,-.125))[n%4]
            if 8<=n<16:vertices[1],vertices[2]=vertices[2],vertices[1]
            if n>=16:
                vertices=[tuple(rng.randrange(-16,17)/8 for _ in range(3))for _ in range(3)]
                origin=tuple(rng.randrange(-16,17)/8 for _ in range(3))
            for i,xyz in enumerate(vertices):v(triangle+16*i,xyz)
            v(point,origin)
            row['banks'][0]['before']=raw.hex()
            row.update(id=len(rows),name=f'hull-triangle-{first}-{n}',entry=entry,stop_before_entries=sorted(EXITS))
            rows.append(row)
    return rows


if __name__=='__main__':
    globals_=paired.main.__globals__.copy()
    globals_.update(__file__=str(Path(__file__).resolve()),ENTRIES=ENTRIES,EXITS=EXITS,
                    CHILDREN={0x30d3f0:3},DRIVER=DRIVER,CHECK_CALL=(0x30e3b6,'e835f0ffff'),
                    EXTRA_INPUTS=(Path(paired.__file__).resolve(),),parent=parent,inputs=inputs,
                    SCOPE='Declared actual mid-owner triangle dispatches through genuine Triangle, with the original signed feature table and five continuations.')
    run=types.FunctionType(paired.main.__code__,globals_,paired.main.__name__,paired.main.__defaults__,paired.main.__closure__)
    raise SystemExit(run())
