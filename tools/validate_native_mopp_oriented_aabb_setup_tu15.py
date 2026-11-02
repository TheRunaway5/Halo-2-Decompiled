#!/usr/bin/env python3
"""Native30A020/2E9640 oriented query setup to real309940, exact FP/fault phases."""
import inspect,struct
from pathlib import Path
import validate_native_mopp_aabb_setup_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
OWNERS=(0x30a020,0x2e9640)
def metadata(image):
 parent.support.OWNERS=OWNERS;parent.support.BOUNDARIES=(0x309940,)
 text=inspect.getsource(parent.support.metadata).replace('assert counts==[276,38,20];','assert counts==[134,19];')
 text=text.replace("read=bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)","read=False if ins.mnemonic in('fst','fstp','fistp')else bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)or ins.mnemonic in('fst','fstp','fistp')")
 env=dict(parent.support.__dict__);exec(text,env);return env['metadata'](image)
def cases(image):
 rows=[];values=(0,0x80000000,0x3f800000,0xbf800000,0x3f000000,0x7fc12345,0x7f812345,0x7f800000,0xff800000,1,0x80000001,0x7f7fffff,0xff7fffff)
 for entry in OWNERS:
  for pattern in range(len(values)):
   for rounding in range(4):
    data=bytearray((n*37+11)&255 for n in range(0x3000));shape=0x1000100;cache=0x1000300 if entry==0x30a020 else parent.base.physical.SP-0x50;code=0x1000800;matrix=0x1001000;extent=0x1001200;keys=0x1001400;tolerance=values[(pattern+4)%len(values)]
    def put(p,v):struct.pack_into('<I',data,p-0x1000000,v&0xffffffff)
    put(shape+16,code)
    for k in range(4):put(code+k*4,values[(pattern+k)%len(values)]);put(extent+k*4,values[(pattern+2*k)%len(values)])
    for k in range(16):put(matrix+k*4,values[(pattern+3*k)%len(values)])
    banks=[dict(address=0x1000000,before=data.hex()),dict(address=0x461950,before=image.read(0x461950,32).hex())]
    if entry==0x2e9640:banks.append(dict(address=cache,before=bytes((n*11+pattern)&255 for n in range(0x40)).hex()))
    n=len(rows);rows.append(dict(name=f'oriented_setup_{entry:x}_{pattern}_{rounding}',entry=entry,receiver=cache if entry==0x30a020 else shape,cache=cache,arguments=[code,matrix,extent,tolerance,keys]if entry==0x30a020 else[matrix,extent,tolerance,keys],banks=banks,before_fp=[(0x7f,0x27f,0x37f)[n%3]|(rounding<<10),((n%8)<<11)|0x4500|(n%32),0x1f80|(rounding<<13)],stop_pc=parent.base.physical.STOP,boundary=None))
 for index in(0,4,20,52):c=dict(rows[index]);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def main():
 text=inspect.getsource(parent.main)
 text=text.replace("src/physics/native_tu15_mopp_aabb_setup.cpp","src/physics/native_tu15_mopp_oriented_aabb_setup.cpp")
 text=text.replace("include/halo2/native_tu15_mopp_aabb_setup.hpp","include/halo2/native_tu15_mopp_oriented_aabb_setup.hpp")
 text=text.replace('#include "halo2/native_tu15_mopp_aabb_setup.hpp"','#include "halo2/native_tu15_mopp_oriented_aabb_setup.hpp"')
 text=text.replace("J::array({100,15})","J::array({134,19})")
 text=text.replace("('no_minus_one','return integer-1u;','return integer;')","('wrong_extent','expanded=add(expanded,scalar)','expanded=add(expanded,expanded)')")
 text=text.replace("('wrong_scale','multiply_scalar(m.word(current+12u))','multiply_scalar(m.word(current+8u))')","('wrong_upper','const auto maximum=words(add(expanded,translation))','const auto maximum=words(add(negative,translation))')")
 text=text.replace("('upper_high','cache+0x12u','cache+0x22u')","('wrong_count','return integer-1u','return integer')")
 position=text.index(' env=dict(base.__dict__')
 calls='const auto a=c.at("arguments");if(c.at("entry")==0x30a020)native_tu15_prepare_mopp_oriented_aabb(m,c.at("cache"),a[0],a[1],a[2],a[3],a[4],fp,0,services,observer);else native_tu15_query_mopp_oriented_shape(m,c.at("receiver"),a[0],a[1],a[2],a[3],c.at("cache"),fp,0,services,observer);'
 text=text[:position]+' calls='+repr(calls)+'\n'+text[position:]
 env=dict(parent.__dict__,__file__=__file__,metadata=metadata,cases=cases,OWNERS=OWNERS);exec(text,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
