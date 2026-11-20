#!/usr/bin/env python3
"""Private full4BAE0 object fade proof against the physical original.

Unchanged TU1.5 instructions of 4BAE0 run single-stepped on physical i386.
Each run stops before the separately owned BCFB0 (root object) and 32CC0C
(the CRT's _ftol2). Deterministic stubs supply them: BCFB0 returns the case's
root handle (per mode it also changes a callee-saved register); _ftol2 pops
ST(0) and returns its value truncated to a 64-bit integer in EDX:EAX (the
indefinite integer for NaN, infinity and out-of-range values). See
tools/native_shadow_frame_oracle.py for the comparison.
"""
import random,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from native_shadow_frame_oracle import Spec,Callee,LIVE,main

REGIONS=((0x456000,0x1000),(0x46a000,0x1000),(0x4c6000,0x1000),(0x4ec000,0x1000),(0x4f0000,0x1000),(0x61000000,0x10000),(0x62000000,0x4000))
IMAGE_BANKS=(0x456000,0x46a000)
TABLE=0x61000000;ROWS=0x61000100;OBJECTS=0x61001000;TAGS=0x61004000;DEFINITIONS=0x61005000;OUTPUTS=0x61008000;STACK=0x62002000
NONE=0xffffffff
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
SNAN=0x7fa00001;QNAN=0xffc00002;DENORMAL=0x00000003;INF=0x7f800000

def root(ctx):
    """BCFB0(ECX) -> EAX: the case's root object."""
    chosen=ctx.row['root'];ctx.gp[0]=ctx.entry_gp[1]if chosen is None else chosen
    if ctx.mode==2:ctx.gp[6]=(ctx.gp[6]+4)&0xffffffff        # ESI: the fade fields move
    if ctx.mode==3:ctx.gp[5]=(ctx.gp[5]+4)&0xffffffff        # EBP: the object moves
    if ctx.mode==4:ctx.gp[3]^=1                               # EBX: the handle changes
def extended_integer(text):
    raw=bytes.fromhex(text);significand=int.from_bytes(raw[:8],'little');head=int.from_bytes(raw[8:10],'little')
    sign=head>>15;exponent=head&0x7fff
    if exponent==0x7fff:return 1<<63
    shift=exponent-16383-63
    value=significand<<shift if shift>=0 else significand>>(-shift)
    if value>=1<<63:return 1<<63
    return(-value if sign else value)&((1<<64)-1)
def to_integer(ctx):
    """_ftol2: pop ST(0), return it truncated in EDX:EAX."""
    if not ctx.live:raise ValueError('_ftol2 stub without ST(0)')
    value=extended_integer(ctx.live[0]);ctx.live=ctx.live[1:]
    top=(ctx.fp[1]>>11)&7;ctx.fp[1]=(ctx.fp[1]&~0x3800)|(((top+1)&7)<<11)
    ctx.gp[0]=value&0xffffffff;ctx.gp[2]=value>>32

def build(image,rng,family,v):
    banks=[bytearray(image.read(base,size))if base in IMAGE_BANKS else bytearray(rng.randbytes(size))for base,size in REGIONS]
    def put(at,value,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
        raise ValueError('Unbacked object fade fixture '+hex(at))
    def get(at,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:return int.from_bytes(bank[at-base:at-base+width],'little')
        raise ValueError('Unbacked object fade input '+hex(at))
    p=dict(disabled=0,never=False,model=True,level=(0,1,2,3,4,5)[v%6],distance=bits((5.,40.,95.,150.,300.,1000.,0.,62.)[v]),radius=bits((1.,2.5,8.,0.5)[v%4]),
           lods=[bits(x)for x in(30.,60.,90.,120.,200.)],disappear=bits(500.),begin=bits(100.),root=None,root_model=True,root_index=None)
    if family==0:p['disabled']=(1,0x80,0xff,2,0x40,1,0x10,0x20)[v]
    if family==1:p['never']=True;p['disabled']=(0,0,1,0,0,0,0,0)[v]
    if family==2:p['model']=False
    if family==3:p['level']=(0,1,2,3,4,0xffff,0x8000,7)[v];p['distance']=bits((25.,65.,95.,125.,205.,1.,2.,50.)[v])
    if family==4:
        p['level']=2;choice=[(bits(92.),None),(bits(150.),None),(bits(100.),None),(SNAN,None),(bits(95.),bits(-1.)),(bits(95.),0),(bits(91.),QNAN),(bits(98.),bits(1.))][v]
        p['distance']=choice[0]
        if choice[1]is not None:p['radius']=choice[1]
        if v==6:p['lods'][2]=QNAN
        if v==7:p['lods'][2]=DENORMAL
    if family==5:
        p['level']=0;p['distance']=bits((50.,150.,600.,500.,100.,120.,2.,300.)[v])
        p['disappear']=(bits(500.),bits(500.),bits(500.),bits(500.),0,QNAN,bits(-3.),bits(100.))[v]
        p['begin']=(bits(100.),bits(100.),bits(100.),bits(100.),bits(100.),bits(100.),bits(100.),bits(100.))[v]
    if family==6:
        p['root_index']=5;p['root']=((0x2222<<16)|5);p['root_model']=v%3!=0;p['distance']=bits((95.,450.,150.,20.)[v%4]);p['level']=1+v%3
    if family==7:
        p['level']=v%5;p['distance']=(DENORMAL,SNAN,INF,bits(-5.),bits(250.),bits(99.),bits(150.),bits(1e30))[v];p['radius']=(DENORMAL,bits(3.),bits(1e-30),INF,bits(2.),bits(0.25),SNAN,bits(1.))[v]
    index=1+v;handle=(0x1234<<16)|index
    put(0x4ecbd0,TABLE);put(TABLE+0x44,ROWS);put(0x4f0414,TAGS);put(0x4c6839,p['disabled'],1)
    def make_object(slot,flags,model,fade):
        obj=OBJECTS+slot*0x100;tags=(4*slot+1,4*slot+2)
        put(ROWS+slot*12+8,obj);put(obj,0x6f620000|tags[0]);put(obj+4,flags);put(obj+0x3c,p['radius'])
        put(TAGS+tags[0]*16+8,DEFINITIONS+tags[0]*0x80);put(TAGS+tags[1]*16+8,DEFINITIONS+tags[1]*0x80)
        put(DEFINITIONS+tags[0]*0x80+0x38,(0x686c0000|tags[1])if model else NONE)
        data=DEFINITIONS+tags[1]*0x80
        put(data+0x28,fade[0]);put(data+0x2c,fade[1])
        for n,lod in enumerate(p['lods']):put(data+0x34+4*n,lod)
        put(data+0x4c,fade[2],2)
    flags=rng.getrandbits(32)&~(1<<19)|((1<<19)if p['never']else 0)
    make_object(index,flags,p['model'],(p['disappear'],p['begin'],p['level']))
    if p['root_index']:
        make_object(p['root_index'],0,p['root_model'],(bits(800.),bits(400.),(p['level']+1)&0xffff))
    sp=STACK+4*(v%4);out=OUTPUTS+v*0x10
    put(sp,0x63000000);put(sp+4,out);put(sp+8,out+5)
    gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp;gp[3]=handle
    xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)];xmm[1][0]=p['distance']
    depth=(v+family)%4;cw=(0x7f,0x27f,0x37f,0x37f|0xc00)[v%4];status=((v%8)<<11)|(0x4700 if v%2 else 0x4500)
    return dict(name=f'family{family}-variant{v}',entry=0x4bae0,gp=gp,eflags=0x202|(0x400 if v%2 else 0)|(rng.getrandbits(12)&0x8d5),matrix=get(sp+4),source=get(sp+8),max_steps=100000,
        xmm=xmm,before_fp=[cw,status,0x1f80|((family%4)<<13)|(0,0x40,0x8000,0x8040)[v%4]],live_x87=list(LIVE[:depth]),
        stub=v%5,root=p['root'],domain=family if family in(0,2)else'ordinary',banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)])
def corpus(image):
    rng=random.Random(0x4bae0);return[build(image,rng,family,v)for family in range(8)for v in range(8)]

LOGIC='src/rendering/native_tu15_object_fade_systems.hpp';ADAPTER='src/rendering/native_tu15_object_fade.hpp'
HELPERS='''static void run_bcfb0(WorldState&s){s.required(0x4bb61u,0xbcfb0u,false);}
static void run_32cc0c(WorldState&s){s.required(static_cast<shadow_frame_harness::Memory&>(s.memory).word(s.gp[4])-5u,0x32cc0cu,false);}
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){s.required(site,target,tail);}
'''
SPEC=Spec(name='object_fade',label='4BAE0 object fade',entry=0x4bae0,binding='native_object_fade_replacements',replace='replace_object_fade',
    body_pin='14be5242fc1af001cf7fce35935bbc687a3e55bf359ea534a3cbb9fb5b5341a0',owned={0x4bae0:0x4bca9},entries=(0x4bae0,),
    callees={0xbcfb0:Callee(sites=(0x4bb61,),argument_bytes=0,effect=root),0x32cc0c:Callee(sites=(0x4bc80,0x4bc95),argument_bytes=0,effect=to_integer)},regions=REGIONS,
    inputs=(LOGIC,ADAPTER,'tests/native_tu15_object_fade_tests.cpp','tools/validate_native_object_fade_tu15.py','tools/native_object_fade_replacements.py'),
    reference_variable='HALO2_REFERENCE_OBJECT_FADE',reference_test_variable='HALO2_OBJECT_FADE_REFERENCE_TEST',census_test_variable='HALO2_OBJECT_FADE_CENSUS_TEST',
    # MOV EAX,4 after CMP EAX,4/JLE (4 minus a zero-extended word is never
    # above 4), and the clamp of a shadow fade above 1 (the fade is 1 minus a
    # non-negative ratio once the distance passed a lod below its band end)
    # are unreachable.
    unreachable=(0x4bbd8,0x4bc23,0x4bc29),
    census_range=(0x4bae0,0x4bca8),helpers=HELPERS,free_x87_topology=True,
    x87_deltas={0x4bc7a:1,0x4bc80:1,0x4bc89:1,0x4bc8d:1,0x4bc93:1,0x4bc95:1},
    mutants=(
     ('ignore-never-fade',LOGIC,'const auto unfaded=(never_fade|disabled)&0xffu;','const auto unfaded=disabled&0xffu;'),
     ('own-model-always',LOGIC,'        if(root_model!=none)k.fade=','        if(root_model==0x12345u)k.fade='),
     ('shadow-level-zero',LOGIC,'const auto lod=level>0?','const auto lod=level>=0?'),
     ('shadow-cut-inclusive',LOGIC,'if(frame.compare(Op::shadow_cut,cutoff,fade).below)return;','if(!above(frame.compare(Op::shadow_cut,fade,cutoff)))return;'),
     ('shadow-fade-no-store',LOGIC,'    frame.set_local(faded_shadow_store,fade);\n    if(above(negative))','    if(above(negative))'),
     ('band-unscaled',LOGIC,'const auto band=frame.multiply(Op::shadow_band,radius,memory.load_float(shadow_band_scale,globals::shadow_band_scale));','const auto band=frame.multiply(Op::shadow_band,radius,(memory.load_float(shadow_band_scale,globals::shadow_band_scale),1.0f));'),
     ('object-byte-late',LOGIC,'    memory.store8(object_byte_store,object_target,object_byte);\n    const auto shadow_byte=frame.to_integer(k);','    const auto shadow_byte=frame.to_integer(k);\n    memory.store8(object_byte_store,object_target,object_byte);'),
     ('draw-fade-reversed-span',LOGIC,'const auto ratio=frame.divide(Op::object_ratio,excess,span);','const auto ratio=frame.divide(Op::object_ratio,span,excess);'),
     ('software-tag-shift',ADAPTER,'case root_definition:tag_offset(eax);return 0x4bb88u;','case root_definition:r(eax)=f.shift(f.logic(r(eax)&0xffffu,4),4u,4);return 0x4bb88u;'),
     ('fmul-wrong-scale',ADAPTER,'s_.fp.st(0)=x87_multiply(s_.fp.st(0),bits,s_.fp.status);','s_.fp.st(0)=x87_multiply(s_.fp.st(0),0x3f800000u,s_.fp.status);'),
     ('missing-xorps',ADAPTER,'        if(op==Op::past_shadow_end)x(4)=_mm_setzero_ps();                         // XORPS XMM4,XMM4\n',''),
     ('kept-distance-ignored',ADAPTER,'k.tags=r(edi);k.distance=low(x(1));','k.tags=r(edi);'),
     ('kept-fade-ignored',ADAPTER,'k.handle=r(ebx);k.object=r(ebp);k.fade=r(esi);k.tags=r(edi);k.distance=low(x(1));','k.handle=r(ebx);k.object=r(ebp);k.tags=r(edi);k.distance=low(x(1));'),
     ('uncertified-required-owner',ADAPTER,'if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;','if(!s.services.required_owner)return false;'),
     ('census-without-switch',ADAPTER,'if(native_census_detail::uncursored(s,0x4bae0u,0x4bca8u)){','if(!s.instruction_cursor&&s.services.instruction){'),
    ),guard_mutants=('uncertified-required-owner','census-without-switch'))
if __name__=='__main__':raise SystemExit(main(SPEC,corpus))
