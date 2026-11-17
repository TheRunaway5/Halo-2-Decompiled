#!/usr/bin/env python3
"""Compare full actual 30E930 returns and genuine native descendants."""
import copy
import struct
import types
from pathlib import Path

import validate_native_simplex_fallback_frame_tu15 as fallback
import validate_native_simplex_hull_owner_frame_tu15 as hull
import validate_native_simplex_hull_reduction_route_tu15 as route
import validate_native_simplex_hull_seed_frame_tu15 as seed

ENTRY,LAST,STOP=0x30e930,0x30f6dd,0x63000000
CHILDREN={**fallback.CHILDREN,0x1f6690:3,0x1f6610:2}
REGIONS=(*seed.REGIONS,(0x44a000,0x1000),(0x462000,0x1000))
FRAME=seed.ROOT/'src/physics/runtime/native_tu15_simplex_owner_frame.hpp'
DEPS=tuple(FRAME.with_name(name)for name in (
    'native_tu15_polygon_features_abi.hpp','native_tu15_polygon_support_frame.hpp',
))
INDIRECT={**seed.INDIRECT_CALLS,0x30e96d:3,0x30e985:3,0x30eeae:3,0x30f312:3,0x30f48e:3}
DEPTH={**seed.DEPTH,**dict.fromkeys((0x30ea06,0x30ea0a,0x30ea13,0x30ee44,0x30ee48,0x30ee4c,
                                   0x30ee6b,0x30ee6f,0x30ee76,0x30ee7a),1),
       0x30ee50:2,0x30ee56:2}
FIRST_SCORE_FP=(0x30eef4,0x30eef8,0x30ef49,0x30ef4d,0x30ef8a,0x30ef8e)
SECOND_SCORE_FP=(0x30f360,0x30f364,0x30f3b8,0x30f3bc,0x30f3fc,0x30f400)
DEPTH.update(dict.fromkeys((*FIRST_SCORE_FP,*SECOND_SCORE_FP,*(pc+0x5e0 for pc in FIRST_SCORE_FP)),1))


def parent(pc):
    return ENTRY<=pc<=LAST or hull.parent(pc)


def inputs(image,smoke):
    bases=seed.inputs(image,False);rows=[]
    vtable=image.read(0x4620d0,0x38)
    if struct.unpack_from('<2I',vtable,0x30)!=(0x1f6610,0x1f6690):
        raise ValueError('Actual polygon support/feature vtable drift')
    branch_bytes=image.read(0x30f6e0,44);selector=image.read(0x30f70c,25)
    if selector!=route.TABLE or struct.unpack('<11I',branch_bytes)[-1]!=0x30edd0:
        raise ValueError('Actual simplex reduction tables drift')
    ro=[dict(address=at,before=image.read(at,size).hex())for at,size in REGIONS[-2:]]
    forms=((1,1),(1,2),(1,3),(1,4),(2,1),(2,2),(2,3),(3,1),(3,2),(4,1))
    for f,(a,b) in enumerate(forms[:3] if smoke else forms):
        for n in range(2 if smoke else 6):
            row=copy.deepcopy(bases[len(rows)%len(bases)])
            bank=next(bank for bank in row['banks']if bank['address']==seed.BASE)
            raw=bytearray.fromhex(bank['before']);gp=row['gp'];sp,work=gp[4],gp[1]
            first,second,relative=seed.BASE+0x5000,seed.BASE+0x6000,seed.BASE+0x1000
            cache,plane=seed.BASE+0x3000,seed.BASE+0x4000
            def words(at,*values):struct.pack_into('<'+'I'*len(values),raw,at-seed.BASE,*values)
            def vec(at,values):struct.pack_into('<4f',raw,at-seed.BASE,*values,0.)
            words(sp,STOP,first,second,relative,cache,plane)
            words(first,0x4620d0);words(second,0x4620d0);words(first+0x14,4);words(second+0x14,4)
            for i in range(3):vec(relative+16*i,tuple(float(i==j)for j in range(3)))
            shifts=((4.,.25,.125),(2.,1.,.25),(-4.,.25,-.125),(.5,.25,.125),(0.,0.,0.),(2.,-1.,1.))
            vec(relative+48,shifts[n]);words(cache,0x00010000,0x00030002)
            raw[cache+8-seed.BASE:cache+12-seed.BASE]=bytes((a,b,4,4))
            raw[work+12-seed.BASE]=0 if n==4 else 1
            vertices=((-1.,-1.,-1.),(1.,1.,-1.),(1.,-1.,1.),(-1.,1.,1.))
            if n&1:vertices=((0.,0.,0.),(1.,0.,0.),(0.,1.,0.),(0.,0.,1.))
            for i,p in enumerate(vertices):
                vec(first+0x20+16*i,p);vec(second+0x20+16*i,tuple(v*(.625 if n&2 else 1.)for v in p))
            bank['before']=raw.hex();row['banks']=[b for b in row['banks']if b['address']not in {0x44a000,0x462000}]+ro
            row.update(id=len(rows),name=f'simplex-owner-{a}-{b}-{n}',entry=ENTRY,stop_before_entries=[STOP],
                return_pc=STOP,matrix=first,source=second,selector_table=image.read(0x30e908,25).hex(),
                branch_table=image.read(0x30e8dc,44).hex(),main_selector=selector.hex(),main_branches=branch_bytes.hex())
            rows.append(row)
    return rows


def driver(scratch):
    path=hull.driver(scratch);text=path.read_text()
    def replace(old,new):
        nonlocal text
        if text.count(old)!=1:raise ValueError('Private complete simplex proof overlay anchor drift')
        text=text.replace(old,new,1)
    header='#include "physics/runtime/native_tu15_simplex_hull_owner_frame.hpp"'
    replace(header,header+'\n'+'\n'.join('#include "physics/runtime/'+p.name+'"'for p in (FRAME,*DEPS)))
    replace('(pc>=0x30de60u&&pc<=0x30e8d8u)','(pc>=0x30de60u&&pc<=0x30f6ddu)')
    replace('    SourcePhysicsWorldRuntimeServices services;unsigned callbacks{},clock_index{};', '''    SourcePhysicsWorldRuntimeServices services;unsigned callbacks{},clock_index{};
    services.reciprocal_sqrt=[](std::uint32_t bits){
        auto v=_mm_castsi128_ps(_mm_cvtsi32_si128(static_cast<int>(bits)));
        asm volatile("rsqrtss %0,%0":"+x"(v));
        return static_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(v)));
    };''')
    replace('        check(target==0x2e8d20u,"Wrong actual Zero target");enter(s,target,site,2u,3u);', '''        if(target==0x1f6610u){enter(s,target,site,2u,3u);native_polygon_support_detail::support_retained<true>(s,before);return;}
        check(target==0x2e8d20u,"Wrong actual support target");enter(s,target,site,2u,3u);''')
    begin='    if(row.at("entry")==0x30de60u)native_simplex_hull_owner_detail::run_private_hull_owner(state,hull_child,before);'
    end='    m.banks.erase(0x30e908u);'
    at=text.index(begin);until=text.index(end,at)
    text=text[:at]+'''    m.banks[0x30f70cu]=bytes(row.at("main_selector"));m.banks[0x30f6e0u]=bytes(row.at("main_branches"));
    const auto simplex_child=[&](WorldState& next,std::uint32_t target,std::uint32_t site){
        switch(target){
        case 0x1f6690u:
            check(site==0x30e96du||site==0x30e985u,"Wrong actual feature callsite");enter(next,target,site,3u,3u);
            native_polygon_features_detail::run_private_owner<true>(next,before);return;
        case 0x1f6610u:
            check(site==0x30eeaeu||site==0x30f312u||site==0x30f48eu,"Wrong actual support callsite");
            enter(next,target,site,2u,3u);native_polygon_support_detail::support_retained<true>(next,before);return;
        case 0x30de60u:
            check(site==0x30f59au,"Wrong actual full hull callsite");enter(next,target,site,4u);
            native_simplex_hull_owner_detail::run_private_hull_owner(next,hull_child,before);return;
        case 0x30d830u:
            check(site==0x30ea7bu,"Wrong actual paired reducer callsite");enter(next,target,site,6u);
            native_simplex_paired_detail::run_private_owner(next,paired_leaf,before);return;
        case 0x30d5d0u:
            check(site==0x30eb3fu||site==0x30ecb0u,"Wrong actual Face Select callsite");enter(next,target,site,1u);
            native_simplex_face_select_detail::run_private_owner(next,refine,before);return;
        case 0x30d3f0u:
            check(site==0x30eb85u||site==0x30ece2u,"Wrong actual Triangle callsite");
            boundary_leaf(next,target,site);return;
        case 0x312620u:
            check(site==0x30eaccu,"Wrong actual Closest callsite");boundary_leaf(next,target,site);return;
        default:throw std::runtime_error("Unexpected full 30E930 native child");
        }
    };
    const auto value=native_simplex_owner_detail::run_private_simplex_owner(state,simplex_child,before);
    check(value==state.gp[0],"Full simplex result lost returned EAX");
    m.banks.erase(0x30f70cu);m.banks.erase(0x30f6e0u);
'''+text[until:]
    path.write_text(text);return path


def run_validation(input_factory=inputs,*,subject=__file__,additional_inputs=(),witness_pcs=()):
    globals_=seed.main.__globals__.copy()
    globals_.update(__file__=str(Path(subject).resolve()),ENTRY=ENTRY,LAST=LAST,STOP=STOP,FRAME=FRAME,
                    BODY_PIN=fallback.JOINED_PIN,CHILDREN=CHILDREN,ACCEPTED_STOPS={STOP},
                    EXTRA_INPUTS=(*hull.DEPS,*DEPS,Path(hull.__file__).resolve(),Path(seed.__file__).resolve(),
                                  Path(fallback.__file__).resolve(),*additional_inputs),
                    READONLY_HULL_REDUCTION=True,READONLY_SIMPLEX_REDUCTION=True,REFERENCE_REGIONS=REGIONS,
                    EXTRA_IO=(0x30f612,),INDIRECT_CALLS=INDIRECT,DEPTH=DEPTH,REQUIRE_PARENT_SITES=False,
                    WITNESS_PCS=witness_pcs,
                    authored=fallback.authored,parent=parent,inputs=input_factory,driver=driver,
                    SCOPE='Complete actual 30E930 entry through RET20 on declared backed polygon inputs. Actual CPU RSQRTSS; genuine virtual features/support and genuine descendants, never supplied child outcomes.')
    run=types.FunctionType(seed.main.__code__,globals_,seed.main.__name__,seed.main.__defaults__,seed.main.__closure__)
    return run()


if __name__=='__main__':
    raise SystemExit(run_validation())
