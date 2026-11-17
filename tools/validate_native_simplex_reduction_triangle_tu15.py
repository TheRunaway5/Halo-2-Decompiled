#!/usr/bin/env python3
"""Compare the original 30E930 triangle calls and signed edge schedules."""
import types
from pathlib import Path

import validate_native_simplex_hull_paired_call_tu15 as paired
import validate_native_simplex_hull_triangle_edges_tu15 as edges
from validate_native_simplex_reduction_paired_tu15 import BODY_PIN, authored

EXITS={0x30edbb,0x30f032,0x30f155,0x30f268}
DRIVER=paired.ROOT/'tests/native_tu15_simplex_reduction_triangle_tests.cpp'


def parent(pc):
    return 0x30eb76<=pc<=0x30ec9d or 0x30ecd3<=pc<=0x30edb8


def inputs(image):
    if image.read(0x30ece2,5).hex()!='e809e7ffff':
        raise ValueError('Actual first simplex Triangle CALL drift')
    rows=edges.inputs(image)
    for n,row in enumerate(rows):
        row.update(name=f'simplex-triangle-{n}',entry=row['entry']+0x7cf,stop_before_entries=sorted(EXITS))
    return rows


if __name__=='__main__':
    globals_=paired.main.__globals__.copy()
    globals_.update(__file__=str(Path(__file__).resolve()),EXITS=EXITS,ORIGINAL_ENTRY=0x30e930,
                    CHILDREN={0x30d3f0:3},DRIVER=DRIVER,CHECK_CALL=(0x30eb85,'e866e8ffff'),
                    BODY_PIN=BODY_PIN,authored=authored,
                    EXTRA_INPUTS=(Path(paired.__file__).resolve(),Path(edges.__file__).resolve(),Path(edges.triangle.__file__).resolve()),
                    parent=parent,inputs=inputs,
                    SCOPE='Both actual 30E930 Triangle/edge paths using their own I/O sites and frame scratch, 80 genuine Triangle returns and separate declared backed edge geometry.')
    run=types.FunctionType(paired.main.__code__,globals_,paired.main.__name__,paired.main.__defaults__,paired.main.__closure__)
    raise SystemExit(run())
