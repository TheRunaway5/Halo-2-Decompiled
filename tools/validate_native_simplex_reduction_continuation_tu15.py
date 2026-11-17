#!/usr/bin/env python3
"""Compare actual 30E930 segment normal, triangle compaction and count paths."""
import types
from pathlib import Path

import validate_native_simplex_hull_paired_call_tu15 as paired
import validate_native_simplex_hull_plane_tu15 as plane
from validate_native_simplex_reduction_paired_tu15 import BODY_PIN, authored

EXITS={0x30edd5,0x30f066,0x30f175}
DRIVER=paired.ROOT/'tests/native_tu15_simplex_reduction_continuation_tests.cpp'
SITES={0x30e606:0x30efa5,0x30e68b:0x30f032,0x30e7a7:0x30f155}


def parent(pc):
    return 0x30efa5<=pc<=0x30f062 or 0x30f155<=pc<=0x30f171 or 0x30f268<=pc<=0x30f26f


def inputs(image):
    base=paired.REGIONS[0][0];rows=[]
    for row in plane.inputs(image):
        if row['entry'] not in SITES:continue
        if row['entry']==0x30e606:
            sp=row['gp'][4]-base;raw=bytearray.fromhex(row['banks'][0]['before'])
            raw[sp+0x50:sp+0x60]=raw[sp+0xc0:sp+0xd0]
            raw[sp+0x60:sp+0x70]=raw[sp+0xb0:sp+0xc0]
            row['banks'][0]['before']=raw.hex()
        row.update(id=len(rows),name='simplex-'+row['name'],entry=SITES[row['entry']],stop_before_entries=sorted(EXITS))
        rows.append(row)
    return rows


if __name__=='__main__':
    globals_=paired.main.__globals__.copy()
    globals_.update(__file__=str(Path(__file__).resolve()),EXITS=EXITS,ORIGINAL_ENTRY=0x30e930,CHILDREN={},
                    DRIVER=DRIVER,CHECK_CALL=(0x30f015,'0f28056019'),BODY_PIN=BODY_PIN,authored=authored,
                    EXTRA_INPUTS=(Path(paired.__file__).resolve(),Path(plane.__file__).resolve()),parent=parent,inputs=inputs,
                    SCOPE='Actual 30E930 segment normal and two triangle compaction paths through their exact projection/edge continuations. Includes four already qualified shared count-tail sites.')
    run=types.FunctionType(paired.main.__code__,globals_,paired.main.__name__,paired.main.__defaults__,paired.main.__closure__)
    raise SystemExit(run())
