#!/usr/bin/env python3
"""Compare both actual 30DE60 Triangle calls and signed edge reductions."""
import copy
import random
import struct
import types
from pathlib import Path

import validate_native_simplex_hull_paired_call_tu15 as paired
import validate_native_simplex_hull_triangle_call_tu15 as triangle

EXITS={0x30e5ec,0x30e68b,0x30e7a7,0x30e8b0}
DRIVER=paired.ROOT/'tests/native_tu15_simplex_hull_triangle_edges_tests.cpp'


def parent(pc):
    return 0x30e3a7<=pc<=0x30e4ce or 0x30e504<=pc<=0x30e5e9


def inputs(image):
    rows=triangle.inputs(image);base=paired.REGIONS[0][0];rng=random.Random(0x30e3d3)
    for row in rows:row['stop_before_entries']=sorted(EXITS)
    if image.read(0x41d758,11).hex()!='ff0001fa02f9f8ff020001':
        raise ValueError('Actual signed triangle feature/edge table drift')
    for first,entry in enumerate((0x30e3d3,0x30e530)):
        for n in range(48):
            row=copy.deepcopy(rows[first*40+n%40]);context=row['gp'][6]
            raw=bytearray.fromhex(row['banks'][0]['before'])
            point=0xa0 if first else 0x20
            row['gp'][0]=n%3;row['gp'][3]=context+point
            for at in (0x20,0x30,0x40,0xa0,0xb0,0xc0,0xe0,0xf0,0x100):
                value=[rng.randrange(-16,17)/8 for _ in range(3)]
                struct.pack_into('<4f',raw,context-base+at,*value,0.)
            row['banks'][0]['before']=raw.hex()
            row.update(id=len(rows),name=f'hull-edge-declared-{first}-{n}',entry=entry)
            rows.append(row)
    return rows


if __name__=='__main__':
    globals_=paired.main.__globals__.copy()
    globals_.update(__file__=str(Path(__file__).resolve()),EXITS=EXITS,CHILDREN={0x30d3f0:3},
                    DRIVER=DRIVER,CHECK_CALL=(0x30e3b6,'e835f0ffff'),
                    EXTRA_INPUTS=(Path(paired.__file__).resolve(),Path(triangle.__file__).resolve()),parent=parent,inputs=inputs,
                    SCOPE='Two actual Triangle dispatches and both authored signed edge reductions. Eighty genuine Triangle returns and separate declared backed edge geometry rows; no supplied child results or production binding.')
    run=types.FunctionType(paired.main.__code__,globals_,paired.main.__name__,paired.main.__defaults__,paired.main.__closure__)
    raise SystemExit(run())
