#!/usr/bin/env python3
"""Compare both actual 30E930 paired branches through the real reducer."""
import types
from pathlib import Path

from generated_routine_ownership import function_bodies
import validate_native_simplex_hull_paired_call_tu15 as paired

ENTRIES=(0x30ea3e,0x30ea5c)
EXITS={0x30ea8e,0x30edd0,0x30f268}
DRIVER=paired.ROOT/'tests/native_tu15_simplex_reduction_paired_tests.cpp'
BODY_PIN='4831de546a5dc35bdbd910bbf393eb7438a992a4b2cb8d7c2718ba1a5fed8113'


def authored():
    path=paired.ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp'
    return next(body for sig,body in function_bodies(path.read_text())if sig.symbol=='30ae40')


def parent(pc):
    return 0x30ea3e<=pc<=0x30ea88


def inputs(image):
    rows=paired.inputs(image)
    for n,row in enumerate(rows):
        row.update(entry=ENTRIES[n//40],name=f'simplex-paired-{n}',stop_before_entries=sorted(EXITS))
    return rows


if __name__=='__main__':
    globals_=paired.main.__globals__.copy()
    globals_.update(__file__=str(Path(__file__).resolve()),ENTRIES=ENTRIES,EXITS=EXITS,ORIGINAL_ENTRY=0x30e930,
                    DRIVER=DRIVER,CHECK_CALL=(0x30ea7b,'e8b0edffff'),BODY_PIN=BODY_PIN,
                    EXTRA_INPUTS=(Path(paired.__file__).resolve(),),parent=parent,inputs=inputs,authored=authored,
                    SCOPE='Both actual 30E930 paired branches with genuine paired/triangle/closest returns and all three continuations.')
    run=types.FunctionType(paired.main.__code__,globals_,paired.main.__name__,paired.main.__defaults__,paired.main.__closure__)
    raise SystemExit(run())
