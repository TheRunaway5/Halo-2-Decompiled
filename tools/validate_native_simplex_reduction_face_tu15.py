#!/usr/bin/env python3
"""Compare the actual 30E930 face schedules through genuine Face Select/Refine."""
import types
from pathlib import Path

import validate_native_simplex_hull_face_call_tu15 as face
import validate_native_simplex_hull_paired_call_tu15 as paired
from validate_native_simplex_reduction_paired_tu15 import BODY_PIN, authored

ENTRIES=(0x30eb31,0x30eca2)
EXITS={0x30eb76,0x30ecd3,0x30edd0}
DRIVER=paired.ROOT/'tests/native_tu15_simplex_reduction_face_tests.cpp'


def parent(pc):
    return 0x30eb31<=pc<=0x30eb72 or 0x30eca2<=pc<=0x30eccf


def inputs(image):
    if image.read(0x30ecb0,5).hex()!='e81be9ffff':
        raise ValueError('Actual first simplex face CALL drift')
    rows=face.inputs(image)
    for n,row in enumerate(rows):
        row.update(name=f'simplex-face-{n}',entry=ENTRIES[n//40],stop_before_entries=sorted(EXITS))
    return rows


if __name__=='__main__':
    globals_=paired.main.__globals__.copy()
    globals_.update(__file__=str(Path(__file__).resolve()),ENTRIES=ENTRIES,EXITS=EXITS,ORIGINAL_ENTRY=0x30e930,
                    CHILDREN={0x30d5d0:1,0x30d1a0:2},DRIVER=DRIVER,CHECK_CALL=(0x30eb3f,'e88ceaffff'),
                    BODY_PIN=BODY_PIN,authored=authored,EXTRA_INPUTS=(Path(paired.__file__).resolve(),Path(face.__file__).resolve()),
                    parent=parent,inputs=inputs,
                    SCOPE='Both actual 30E930 face paths using the shared native schedule, original instruction sites and genuine Face Select/Refine.')
    run=types.FunctionType(paired.main.__code__,globals_,paired.main.__name__,paired.main.__defaults__,paired.main.__closure__)
    raise SystemExit(run())
