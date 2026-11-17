#!/usr/bin/env python3
"""Run the original 30E930 fallback through the complete genuine 30DE60 owner."""
import copy
import hashlib
import struct
import types
from pathlib import Path

from generated_routine_ownership import function_bodies
import validate_native_simplex_hull_owner_frame_tu15 as hull
import validate_native_simplex_hull_seed_frame_tu15 as seed

ENTRY,LAST,STOP=0x30f581,0x30f5cf,0x30f5d7
CHILDREN={**hull.CHILDREN,0x30de60:4}
FRAME=seed.ROOT/'src/physics/runtime/native_tu15_simplex_fallback_frame.hpp'
PARENT_PIN='4831de546a5dc35bdbd910bbf393eb7438a992a4b2cb8d7c2718ba1a5fed8113'
JOINED_PIN='283a0db684f31e2325892eae080407b33e59ecffffe5f78f2b0435dea713c0d3'


def authored():
    body=next(body for sig,body in function_bodies(seed.SOURCE.read_text())if sig.symbol=='30ae40')
    if hashlib.sha256(body.encode()).hexdigest()!=PARENT_PIN:
        raise ValueError('Complete actual 30E930 parent body drift')
    return body+'\n'+seed.authored()


def parent(pc):
    return ENTRY<=pc<=LAST or hull.parent(pc)


def inputs(image,smoke):
    source_rows=[r for r in hull.inputs(image,smoke)if r['entry']==seed.ENTRY]
    if image.read(0x30f59a,5).hex()!='e8c1e8ffff':
        raise ValueError('Actual hull fallback CALL drift')
    rows=[]
    for row in source_rows:
        bank=next(b for b in row['banks']if b['address']==seed.BASE)
        raw=bytearray.fromhex(bank['before']);gp=row['gp'];old_sp,context=gp[4],gp[1]
        args=struct.unpack_from('<4I',raw,old_sp+4-seed.BASE)
        gp[4],gp[5],gp[6],gp[7]=old_sp+20,old_sp+0x100,context,context+4
        struct.pack_into('<4I',raw,gp[5]+8-seed.BASE,*args)
        raw[context+12-seed.BASE]=0
        words=struct.unpack_from('<3I',raw,gp[4]-seed.BASE)
        row.update(id=len(rows),name='fallback-'+row['name'],entry=ENTRY,stop_before_entries=[STOP],
                   return_pc=words[0],matrix=words[1],source=words[2])
        bank['before']=raw.hex();rows.append(row)
    for n,(first,second) in enumerate(((1,1),(3,2),(2,3),(4,4),(0x80000000,2),(0xffffffff,0))):
        row=copy.deepcopy(rows[n]);context=row['gp'][6]
        bank=next(b for b in row['banks']if b['address']==seed.BASE);raw=bytearray.fromhex(bank['before'])
        struct.pack_into('<2I',raw,context-seed.BASE,first,second);raw[context+12-seed.BASE]=1+n
        bank['before']=raw.hex();row.update(id=len(rows),name=f'fallback-overlap-{first:x}-{second:x}')
        rows.append(row)
    return rows


def driver(scratch):
    path=hull.driver(scratch);text=path.read_text()
    def replace(old,new):
        nonlocal text
        if text.count(old)!=1:raise ValueError('Private simplex fallback proof overlay anchor drift')
        text=text.replace(old,new,1)
    header='#include "physics/runtime/native_tu15_simplex_hull_owner_frame.hpp"'
    replace(header,header+'\n#include "physics/runtime/native_tu15_simplex_fallback_frame.hpp"')
    replace('(pc>=0x30de60u&&pc<=0x30e8d8u)','(pc>=0x30de60u&&pc<=0x30e8d8u)||(pc>=0x30f581u&&pc<=0x30f5cfu)')
    begin='    if(row.at("entry")==0x30de60u)native_simplex_hull_owner_detail::run_private_hull_owner(state,hull_child,before);'
    end='    m.banks.erase(0x30e908u);'
    at=text.index(begin);until=text.index(end,at)
    text=text[:at]+'''    const auto fallback=[&](WorldState& next,std::uint32_t target,std::uint32_t site){
        check(target==0x30de60u&&site==0x30f59au,"Wrong real 30E930 hull fallback");
        enter(next,target,site,4u);native_simplex_hull_owner_detail::run_private_hull_owner(next,hull_child,before);
    };
    native_simplex_fallback_detail::run_private_fallback(state,fallback,before);
'''+text[until:]
    replace('constexpr auto exit=0x63000000u;','constexpr auto exit=0x30f5d7u;')
    path.write_text(text);return path


if __name__=='__main__':
    globals_=seed.main.__globals__.copy()
    globals_.update(__file__=str(Path(__file__).resolve()),ENTRY=ENTRY,LAST=LAST,STOP=STOP,FRAME=FRAME,
                    BODY_PIN=JOINED_PIN,CHILDREN=CHILDREN,ACCEPTED_STOPS={STOP},
                    EXTRA_INPUTS=(*hull.DEPS,Path(hull.__file__).resolve(),Path(seed.__file__).resolve()),
                    ORIGINAL_ENTRY=0x30e930,READONLY_HULL_REDUCTION=True,
                    authored=authored,parent=parent,inputs=inputs,driver=driver,
                    SCOPE='Declared 30E930 backed fallback entry through the actual full 30DE60 and real descendants, or original signed overlap cap. Stops before finalize. No default or provider broadening.')
    run=types.FunctionType(seed.main.__code__,globals_,seed.main.__name__,seed.main.__defaults__,seed.main.__closure__)
    raise SystemExit(run())
