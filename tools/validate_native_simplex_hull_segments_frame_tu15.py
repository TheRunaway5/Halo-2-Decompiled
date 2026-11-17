#!/usr/bin/env python3
"""Compare actual 30DE60 segment dispatch through real Closest Segments."""
import copy
import struct
import types
from pathlib import Path

import validate_native_simplex_hull_paired_call_tu15 as paired

ENTRY,LAST=0x30e2af,0x30e35d
EXITS={0x30e5ec,0x30e606}
DRIVER=paired.ROOT/'tests/native_tu15_simplex_hull_segments_frame_tests.cpp'


def parent(pc):
    return ENTRY<=pc<=LAST


def inputs(image):
    seeds=paired.inputs(image);rows=[];base=0x61000000
    for x in (-1.,.5,2.):
        for endpoints in ((-1.,1.),(1.,2.),(-2.,-1.)):
            for variant in range(8):
                row=copy.deepcopy(seeds[len(rows)]);context=row['gp'][6];raw=bytearray.fromhex(row['banks'][0]['before'])
                left=[(0.,0.,0.),(1.,0.,0.)]
                right=[(x,endpoints[0],float(variant>>2)*.5),(x,endpoints[1],float(variant>>2)*.5)]
                if variant&1:left.reverse()
                if variant&2:right.reverse()
                for i,point in enumerate(left):struct.pack_into('<4f',raw,context-base+0x20+i*16,*point,0.)
                for i,point in enumerate(right):struct.pack_into('<4f',raw,context-base+0xa0+i*16,*point,0.)
                struct.pack_into('<2I',raw,context-base,2,2)
                row['banks'][0]['before']=raw.hex();row.update(id=len(rows),name=f'hull-segments-{x}-{endpoints}-{variant}',
                         entry=ENTRY,stop_before_entries=sorted(EXITS))
                rows.append(row)
    return rows


if __name__=='__main__':
    globals_=paired.main.__globals__.copy()
    globals_.update(__file__=str(Path(__file__).resolve()),FIRST=ENTRY,LAST=LAST,ENTRIES=(ENTRY,),EXITS=EXITS,
                    CHILDREN={0x312620:5},DRIVER=DRIVER,CHECK_CALL=(0x30e2fd,'e81e430000'),
                    EXTRA_INPUTS=(Path(paired.__file__).resolve(),),parent=parent,inputs=inputs,
                    SCOPE='Declared actual mid-owner segment dispatch through genuine Closest Segments, including all endpoint reductions and two continuations.')
    run=types.FunctionType(paired.main.__code__,globals_,paired.main.__name__,paired.main.__defaults__,paired.main.__closure__)
    raise SystemExit(run())
