#!/usr/bin/env python3
"""Compare complete actual 30DE60 returns with genuine native descendants."""
import struct
import types
from pathlib import Path

import validate_native_simplex_hull_compact_frame_tu15 as compact
import validate_native_simplex_hull_reduction_route_tu15 as route
import validate_native_simplex_hull_seed_frame_tu15 as seed

LAST,STOP=0x30e8d8,0x63000000
CHILDREN={**compact.CHILDREN,0x30d830:6,0x30d5d0:1,0x30d1a0:2}
DEPS=tuple(seed.ROOT/'src/physics/runtime'/name for name in (
    'native_tu15_simplex_hull_owner_frame.hpp','native_tu15_simplex_paired_frame.hpp',
    'native_tu15_simplex_face_select_abi.hpp','native_tu15_simplex_face_refine_abi.hpp',
))


def parent(pc):
    return seed.ENTRY<=pc<=LAST or seed.parent(pc)


def inputs(image,smoke):
    rows=compact.inputs(image,smoke)
    selector=image.read(route.SELECTOR,25);branches=image.read(route.BRANCHES,44)
    if selector!=route.TABLE or struct.unpack('<11I',branches)!=route.DESTINATIONS:
        raise ValueError('Actual full-owner readonly reduction table drift')
    for row in rows:
        row.update(stop_before_entries=[STOP],selector_table=selector.hex(),branch_table=branches.hex())
    return rows


def driver(scratch):
    path=compact.driver(scratch);text=path.read_text()
    def replace(old,new):
        nonlocal text
        if text.count(old)!=1:raise ValueError('Private simplex hull owner proof overlay anchor drift')
        text=text.replace(old,new,1)
    header='#include "physics/runtime/native_tu15_simplex_hull_compact_frame.hpp"'
    replace(header,header+'\n'+'\n'.join('#include "physics/runtime/'+path.name+'"'for path in DEPS))
    replace('pc<=0x30e230u','pc<=0x30e8d8u')
    replace('    bool returned=false;', '''    m.banks[0x30e908u]=bytes(row.at("selector_table"));m.banks[0x30e8dcu]=bytes(row.at("branch_table"));
    const auto paired_leaf=[&](WorldState& next,std::uint32_t target,std::uint32_t canonical_site){
        boundary_leaf(next,target,canonical_site+0x3af0u);
    };
    const auto refine=[&](WorldState& next,std::uint32_t target,std::uint32_t canonical_site){
        check(target==0x30d1a0u&&canonical_site==0x309d2fu,"Wrong true Face Select descendant");
        enter(next,target,0x30d81fu,2u);native_simplex_face_refine_detail::run_private_owner(next,before);
    };
    const auto hull_child=[&](WorldState& next,std::uint32_t target,std::uint32_t site){
        switch(target){
        case 0x314710u:actual_context(next,target,site);return;
        case 0x30dc10u:boundary(next,target,site);return;
        case 0x30d830u:
            check(site==0x30e29cu,"Wrong actual paired reducer callsite");enter(next,target,site,6u);
            native_simplex_paired_detail::run_private_owner(next,paired_leaf,before);return;
        case 0x30d5d0u:
            check(site==0x30e370u||site==0x30e4e1u,"Wrong actual Face Select callsite");enter(next,target,site,1u);
            native_simplex_face_select_detail::run_private_owner(next,refine,before);return;
        case 0x30d3f0u:
            check(site==0x30e3b6u||site==0x30e513u,"Wrong actual Triangle callsite");
            boundary_leaf(next,target,site);return;
        case 0x312620u:
            check(site==0x30e2fdu,"Wrong actual Closest callsite");boundary_leaf(next,target,site);return;
        default:throw std::runtime_error("Unexpected full 30DE60 native child");
        }
    };''')
    begin='    if(row.at("entry")==0x30de60u){'
    end='    check(clock_index==clocks.size(),"Context skipped an actual observed clock");'
    at=text.index(begin);until=text.index(end,at)
    text=text[:at]+'''    if(row.at("entry")==0x30de60u)native_simplex_hull_owner_detail::run_private_hull_owner(state,hull_child,before);
    else {
        check(row.at("entry")==0x30e0a3u,"Unexpected complete simplex hull proof entry");
        native_simplex_hull_owner_detail::Frame frame{state,hull_child,before};
        frame.compact();native_tu15_simplex_hull_reductions(frame);
    }
    m.banks.erase(0x30e908u);m.banks.erase(0x30e8dcu);
'''+text[until:]
    replace('const auto exit=returned?0x63000000u:0x30e235u;','constexpr auto exit=0x63000000u;')
    path.write_text(text);return path


if __name__=='__main__':
    globals_=seed.main.__globals__.copy()
    globals_.update(__file__=str(Path(__file__).resolve()),LAST=LAST,STOP=STOP,CHILDREN=CHILDREN,ACCEPTED_STOPS={STOP},
                    EXTRA_INPUTS=(*DEPS,Path(seed.__file__).resolve(),Path(compact.__file__).resolve(),Path(route.__file__).resolve()),
                    REQUIRE_PARENT_SITES=False,READONLY_HULL_REDUCTION=True,parent=parent,inputs=inputs,driver=driver,
                    SCOPE='Complete actual 30DE60 entry through RET16, plus separately declared backed compaction entries through the same real return. Genuine descendants only. No default/source or wider allocation/provider claim.')
    run=types.FunctionType(seed.main.__code__,globals_,seed.main.__name__,seed.main.__defaults__,seed.main.__closure__)
    raise SystemExit(run())
