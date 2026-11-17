#!/usr/bin/env python3
"""Compose real 30DE60 seed, copyback, duplicate compaction and boundary reduction."""
import copy
import os
import struct
import types
from pathlib import Path

import validate_native_simplex_hull_seed_frame_tu15 as seed

LAST,STOP=0x30e230,0x30e235
CHILDREN={**seed.CHILDREN,0x30dc10:0,0x30d3f0:3,0x312620:5}
DEPS=tuple(seed.ROOT/'src/physics/runtime'/name for name in (
    'native_tu15_simplex_hull_copyback_frame.hpp','native_tu15_simplex_hull_compact_frame.hpp',
    'native_tu15_simplex_boundary_frame.hpp','native_tu15_simplex_triangle_abi.hpp','native_tu15_closest_segments_frame.hpp',
))


def parent(pc):
    return seed.ENTRY<=pc<=LAST or seed.parent(pc)


def inputs(image,smoke):
    rows=seed.inputs(image,smoke)
    short=copy.deepcopy(rows[0]);short.update(id=len(rows),name='hull-compact-single-pair')
    main=next(bank for bank in short['banks']if bank['address']==seed.BASE);raw=bytearray.fromhex(main['before'])
    struct.pack_into('<4f',raw,0x1020+0x10,0.,0.,0.,0.) # original relative translation
    struct.pack_into('<4f',raw,0x2020,0.,0.,0.,0.)
    main['before']=raw.hex();rows.append(short)
    for row in rows:row['stop_before_entries']=[STOP]
    # Use genuine setup and copyback to acquire the compact frame. The
    # additional backed rows vary only six vertex values at this declared
    # mid-owner entry, then invoke the real Boundary child as selected.
    genuine=copy.deepcopy(rows[1]);genuine.update(id=0,stop_before_entries=[0x30e0a3])
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])/'compact-before';scratch.mkdir()
    globals_=seed.observe_original.__globals__.copy();globals_.update(OUTER=seed.ENTRY,ALL_CHILDREN=CHILDREN)
    original=types.FunctionType(seed.observe_original.__code__,globals_,seed.observe_original.__name__,
                                seed.observe_original.__defaults__,seed.observe_original.__closure__)
    observed,_=original(image,[genuine],scratch,[]);case=observed['cases'][0]
    if case.get('stopped_at')!=0x30e0a3 or not case['live_x87_unchanged']:
        raise ValueError('Actual copyback did not reach the declared compaction entry')
    for variant in range(3 if smoke else 8):
        row=copy.deepcopy(genuine);gp=case['boundary_gp'];sp=gp[4];context=gp[6]
        banks=[dict(address=bank['address'],before=bank['after'])for bank in case['banks']]
        main=next(bank for bank in banks if bank['address']==seed.BASE);raw=bytearray.fromhex(main['before'])
        for n,point in enumerate(((0.,0.,0.),(1.,0.,0.),(0.,1.,0.))):
            struct.pack_into('<4f',raw,context-seed.BASE+0x20+n*16,*point,0.)
            struct.pack_into('<4f',raw,context-seed.BASE+0xe0+n*16,point[0]+(variant%3)*.25,point[1],1.+(variant%2)*.125,0.)
        main['before']=raw.hex();words=struct.unpack_from('<3I',raw,sp-seed.BASE)
        row.update(id=len(rows),name=f'declared-compact-six-{variant}',entry=0x30e0a3,gp=gp,eflags=case['boundary_eflags'],
                   xmm=case['boundary_xmm'],before_fp=case['after_fp'],return_pc=words[0],matrix=words[1],source=words[2],
                   banks=banks,stop_before_entries=[STOP])
        rows.append(row)
    return rows


def driver(scratch):
    path=seed.driver(scratch);text=path.read_text()
    def replace(old,new):
        nonlocal text
        if text.count(old)!=1:raise ValueError('Private simplex compaction proof overlay anchor drift')
        text=text.replace(old,new,1)
    header='#include "physics/runtime/native_tu15_simplex_hull_seed_frame.hpp"'
    replace(header,header+'\n'+'\n'.join('#include "physics/runtime/'+path.name+'"'for path in DEPS))
    replace('pc<=0x30e013u','pc<=0x30e230u')
    replace('    native_simplex_hull_seed_detail::run_private_hull_seed(state,actual_context,before);\n    check(clock_index==clocks.size(),"Context skipped an actual observed clock");\n    m.banks.erase(0x3146f8u);\n    constexpr auto exit=0x30e018u;', '''    const auto boundary_leaf=[&](WorldState& next,std::uint32_t target,std::uint32_t site){
        if(target==0x30d3f0u){enter(next,target,site,3u);native_simplex_triangle_detail::run_private_owner(next,before);return;}
        check(target==0x312620u,"Wrong true simplex boundary leaf");enter(next,target,site,5u);
        native_closest_segments_detail::run_private_owner(next,before);
    };
    const auto boundary=[&](WorldState& next,std::uint32_t target,std::uint32_t site){
        check(target==0x30dc10u&&site==0x30e230u,"Wrong real simplex boundary child");enter(next,target,site,0u);
        native_simplex_boundary_detail::run_private_owner(next,boundary_leaf,before);
    };
    bool returned=false;
    if(row.at("entry")==0x30de60u){
        native_simplex_hull_seed_detail::run_private_hull_seed(state,actual_context,before);
        returned=native_simplex_hull_copyback_detail::run_private_hull_copyback(state,before);
    }else check(row.at("entry")==0x30e0a3u,"Unexpected simplex compact proof entry");
    if(!returned)native_simplex_hull_compact_detail::run_private_hull_compact(state,boundary,before);
    check(clock_index==clocks.size(),"Context skipped an actual observed clock");
    m.banks.erase(0x3146f8u);
    const auto exit=returned?0x63000000u:0x30e235u;''')
    path.write_text(text);return path


if __name__=='__main__':
    globals_=seed.main.__globals__.copy()
    globals_.update(__file__=str(Path(__file__).resolve()),LAST=LAST,STOP=STOP,CHILDREN=CHILDREN,
                    ACCEPTED_STOPS={STOP,0x63000000},EXTRA_INPUTS=(*DEPS,Path(seed.__file__).resolve()),
                    REQUIRE_PARENT_SITES=True,parent=parent,inputs=inputs,driver=driver,
                    SCOPE='Actual 30DE60 backed seed/copyback/compaction, with declared six-vertex mid-owner cases and genuine Boundary descendants. No complete parent or source/default claim.')
    run=types.FunctionType(seed.main.__code__,globals_,seed.main.__name__,seed.main.__defaults__,seed.main.__closure__)
    raise SystemExit(run())
