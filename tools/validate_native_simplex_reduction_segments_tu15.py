#!/usr/bin/env python3
"""Compare actual 30E930 segment reduction through genuine Closest Segments."""
import types
from pathlib import Path

import validate_native_simplex_hull_paired_call_tu15 as paired
import validate_native_simplex_hull_segments_frame_tu15 as segments
from validate_native_simplex_reduction_paired_tu15 import BODY_PIN, authored

ENTRY,LAST=0x30ea8e,0x30eb2c
EXITS={0x30edbb,0x30efa5}
DRIVER=paired.ROOT/'tests/native_tu15_simplex_reduction_segments_tests.cpp'


def parent(pc):
    return ENTRY<=pc<=LAST


def inputs(image):
    rows=segments.inputs(image)
    for row in rows:
        row.update(name='simplex-'+row['name'],entry=ENTRY,stop_before_entries=sorted(EXITS))
    return rows


if __name__=='__main__':
    globals_=paired.main.__globals__.copy()
    globals_.update(__file__=str(Path(__file__).resolve()),ENTRIES=(ENTRY,),EXITS=EXITS,ORIGINAL_ENTRY=0x30e930,
                    CHILDREN={0x312620:5},DRIVER=DRIVER,CHECK_CALL=(0x30eacc,'e84f3b0000'),
                    BODY_PIN=BODY_PIN,authored=authored,EXTRA_INPUTS=(Path(paired.__file__).resolve(),Path(segments.__file__).resolve()),
                    parent=parent,inputs=inputs,
                    SCOPE='Actual 30E930 segment setup and endpoint reductions through real Closest Segments, with both continuations.')
    run=types.FunctionType(paired.main.__code__,globals_,paired.main.__name__,paired.main.__defaults__,paired.main.__closure__)
    raise SystemExit(run())
