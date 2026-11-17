#!/usr/bin/env python3
"""Compare actual 30E930 reduction re-entry and the degenerate marker."""
import types
from pathlib import Path

import validate_native_simplex_hull_reduction_route_tu15 as route
from validate_native_simplex_reduction_paired_tu15 import BODY_PIN, authored

ENTRY,SELECTOR,BRANCHES=0x30edbb,0x30f70c,0x30f6e0
DESTINATIONS=(0x30f253,0x30f066,0x30eb76,0x30eb31,0x30f175,0x30ea8e,0x30ea3e,0x30ecd3,0x30ea5c,0x30eca2,0x30edd0)
CONTINUATIONS=(*DESTINATIONS[:-1],0x30edd5)
IO=(0x30edbb,0x30edbd,0x30ea30,0x30ea37)
SITES=set(IO)|{0x30edbf,0x30edc2,0x30edc4,0x30edc7,0x30edca,0x30edd0}
DRIVER=route.ROOT/'tests/native_tu15_simplex_reduction_redispatch_tests.cpp'


def inputs(image):
    globals_=route.inputs.__globals__.copy()
    globals_.update(ENTRY=ENTRY,SELECTOR=SELECTOR,BRANCHES=BRANCHES,DESTINATIONS=DESTINATIONS)
    base=types.FunctionType(route.inputs.__code__,globals_,route.inputs.__name__)
    rows=base(image)
    for row in rows:row['stop_before_entries']=list(CONTINUATIONS)
    return rows


if __name__=='__main__':
    observer_globals=route.original.__globals__.copy()
    observer_globals.update(SELECTOR=SELECTOR,BRANCHES=BRANCHES,ORIGINAL_OWNER=0x30e930)
    original=types.FunctionType(route.original.__code__,observer_globals,route.original.__name__)
    globals_=route.main.__globals__.copy()
    frame=route.ROOT/'src/physics/runtime/native_tu15_simplex_reduction_continuation_frame.hpp'
    deps=(Path(route.__file__).resolve(),frame,frame.with_name('native_tu15_simplex_iteration_abi.hpp'),
          frame.with_name('native_tu15_simplex_hull_plane_frame.hpp'))
    globals_.update(__file__=str(Path(__file__).resolve()),ENTRY=ENTRY,DRIVER=DRIVER,BODY_PIN=BODY_PIN,
                    TABLE_OPCODES=((0x30ea30,'0fb6880cf73000'),(0x30ea37,'ff248de0f63000')),
                    REPEAT_ENTRY=ENTRY,REPEAT_IO=IO,REPEAT_SITES=SITES,REPEAT_BRANCH=(0x30edca,'0f8660fcffff'),
                    DESTINATIONS=CONTINUATIONS,EXTRA_INPUTS=deps,original=original,authored=authored,inputs=inputs)
    run=types.FunctionType(route.main.__code__,globals_,route.main.__name__,route.main.__defaults__,route.main.__closure__)
    raise SystemExit(run())
