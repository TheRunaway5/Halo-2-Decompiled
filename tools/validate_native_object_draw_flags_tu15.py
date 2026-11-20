#!/usr/bin/env python3
"""Private full135E00 render-object draw flags proof against the physical original.

Unchanged TU1.5 instructions of 135E00 run single-stepped on physical i386
(pure leaf, no callees). See tools/native_shadow_frame_oracle.py for the
comparison.
"""
import random,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from native_shadow_frame_oracle import Spec,LIVE,main

REGIONS=((0x46a000,0x1000),(0x4c6000,0x1000),(0x4f0000,0x1000),(0x61000000,0x10000),(0x62000000,0x4000))
IMAGE_BANKS=(0x46a000,)
VIEWS=0x61000000;TAGS=0x61004000;DEFINITIONS=0x61005000;STACK=0x62002000
NONE=0xffffffff
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
SNAN=0x7fa00001;QNAN=0xffc00002;DENORMAL=0x00000003;NEGZERO=0x80000000

def build(image,rng,family,v):
    banks=[bytearray(image.read(base,size))if base in IMAGE_BANKS else bytearray(rng.randbytes(size))for base,size in REGIONS]
    def put(at,value,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
        raise ValueError('Unbacked object draw flags fixture '+hex(at))
    def get(at,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:return int.from_bytes(bank[at-base:at-base+width],'little')
        raise ValueError('Unbacked object draw flags input '+hex(at))
    # Exercise all row flag bits, view byte sign bit, fog modes, fog tags,
    # densities (0, -0, positive, negative, NaN, SNaN, denormal), definition
    # flags, and fog override byte.
    row=(0x0000,0x0200,0x1000,0x2000,0x4000,0x8000,0xf200,0xffff)[v]
    view_byte=(0x00,0x80,0x7f,0xff,0x01,0x81,0x40,0xc0)[(v+family)%8]
    fog_mode=3
    fog_tag=(0x666f0000|(1+v))
    density=0
    def_flags=0
    override=0
    if family==0:fog_mode=(0,1,2,4,0xffffffff,3,3,0x10)[v]
    elif family==1:fog_tag=(NONE,NONE,0x666f0001,NONE,0x666f0002,NONE,0x666f0003,NONE)[v];override=v%2
    elif family==2:density=(0,NEGZERO,bits(0.5),bits(-0.5),QNAN,SNAN,DENORMAL,0)[v];row&=~0x0400
    elif family==3:row=(0x0000,0x0400,0x0600,0x1400,0x2000,0x4400,0x8000,0xf600)[v];override=1;def_flags=0
    elif family==4:row&=~0x0400;def_flags=(0,2,1,3,0xfd,0xff,0x00,0x02)[v];override=v%2
    elif family==5:row=(0x0000,0x0400,0x0200,0x0400,0x1000,0x2400,0x4000,0x8400)[v];override=(0,0,1,1,0xff,0x80,2,0)[v]
    else:
        row=rng.getrandbits(16)
        if family==6:row&=~0x0400
        density=(0,NEGZERO,bits(1.0),QNAN,0,DENORMAL,0,bits(-2.0))[v]
        def_flags=(0,2,0,4,2,0,0xfe,0xff)[v]
        override=(0,1,0,0xff,1,0,0x80,0)[v]
    view=VIEWS+v*0x20
    put(view+0x2a60,view_byte,1)
    put(0x4c6710,fog_mode)
    put(0x4c67ac,fog_tag)
    put(0x4c681c,density)
    put(0x4c6824,override,1)
    put(0x4f0414,TAGS)
    for k in range(1,12):
        put(TAGS+k*16+8,DEFINITIONS+k*0x40)
        put(DEFINITIONS+k*0x40,def_flags,1)
    sp=STACK+4*(v%4)
    put(sp,0x63000000);put(sp+4,view)
    gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp;gp[2]=(rng.getrandbits(16)<<16)|row
    xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
    depth=(v+family)%4;cw=(0x7f,0x27f,0x37f)[v%3]|((v%4)<<10);status=((v%8)<<11)|0x4500
    return dict(name=f'family{family}-variant{v}',entry=0x135e00,gp=gp,eflags=0x202|(0x400 if v%2 else 0)|(rng.getrandbits(12)&0x8d5),matrix=get(sp+4),source=get(sp+8),max_steps=100000,
        xmm=xmm,before_fp=[cw,status,0x1f80|((family%4)<<13)|(0,0x40,0x8000,0x8040)[v%4]],live_x87=list(LIVE[:depth]),
        stub=0,domain=family if family in(0,1)else'ordinary',banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)])
def corpus(image):
    rng=random.Random(0x135e00);return[build(image,rng,family,v)for family in range(8)for v in range(8)]

LOGIC='src/rendering/native_tu15_object_draw_flags_systems.hpp';ADAPTER='src/rendering/native_tu15_object_draw_flags.hpp'
SPEC=Spec(name='object_draw_flags',label='135E00 object draw flags',entry=0x135e00,binding='native_object_draw_flags_replacements',replace='replace_object_draw_flags',
    body_pin='01e33c0f5f84ff83ea22dd97711f156fc5ea527af9d213c9237b14ed215bce2b',owned={0x135e00:0x135e9e},entries=(0x135e00,),
    callees={},regions=REGIONS,
    inputs=(LOGIC,ADAPTER,'tests/native_tu15_object_draw_flags_tests.cpp','tools/validate_native_object_draw_flags_tu15.py','tools/native_object_draw_flags_replacements.py'),
    reference_variable='HALO2_REFERENCE_OBJECT_DRAW_FLAGS',reference_test_variable='HALO2_OBJECT_DRAW_FLAGS_REFERENCE_TEST',census_test_variable='HALO2_OBJECT_DRAW_FLAGS_CENSUS_TEST',
    census_range=(0x135e00,0x135e9b),
    helpers='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){s.required(site,target,tail);}\n',
    mutants=(
     ('wrong-shifted-mask',LOGIC,'inline constexpr std::uint32_t shifted_mask=0x2c00u;','inline constexpr std::uint32_t shifted_mask=0x2800u;'),
     ('wrong-bit14',LOGIC,'inline constexpr std::uint32_t bit_14=0x4000u;','inline constexpr std::uint32_t bit_14=0x2000u;'),
     ('wrong-draw14',LOGIC,'inline constexpr std::uint32_t draw_flag_14=0x0200u;','inline constexpr std::uint32_t draw_flag_14=0x0100u;'),
     ('ignore-view-byte',LOGIC,'if(memory.load8(view_flag_byte,at(view,offsetof(View,draw_flag_byte)))&0x80u)','if(memory.load8(view_flag_byte,at(view,offsetof(View,draw_flag_byte)))&0x40u)'),
     ('wrong-fog-mode',LOGIC,'inline constexpr std::uint32_t planar_fog_mode=3u;','inline constexpr std::uint32_t planar_fog_mode=2u;'),
     ('skip-fog-tag-check',LOGIC,'if(tag!=none) {','if(tag==0u) {'),
     ('invert-density-zero',LOGIC,'if(frame.density_is_zero(density,zero) && !(row&row_flags::inside_fog))','if(!frame.density_is_zero(density,zero) && !(row&row_flags::inside_fog))'),
     ('invert-def-bit',LOGIC,'if(!(memory.load8(fog_definition_flags,at(definition,offsetof(FogDefinition,flags)))&\n                     fog_definition_no_skip_bit))','if((memory.load8(fog_definition_flags,at(definition,offsetof(FogDefinition,flags)))&\n                     fog_definition_no_skip_bit))'),
     ('wrong-override-logic',LOGIC,'if(!override_byte || (row&row_flags::inside_fog))','if(!override_byte && (row&row_flags::inside_fog))'),
     ('software-shift-right2',ADAPTER,'const auto shifted=shift_right2(r(ecx),f.bits());r(ecx)=shifted.value;f.load(shifted.flags);','r(ecx)=f.right(r(ecx),2u,4,false);'),
     ('software-tag-shift',ADAPTER,'const auto row=tag_row(r(esi),f.bits());r(esi)=row.value;f.load(row.flags);','r(esi)=f.shift(f.logic(r(esi)&0xffffu,4),4u,4);'),
     ('comiss-instead-of-ucomiss',ADAPTER,'asm volatile("ucomiss %4,%3;setb %0;sete %1;setp %2"','asm volatile("comiss %4,%3;setb %0;sete %1;setp %2"'),
     ('uncertified-required-owner',ADAPTER,'if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;','if(!s.services.required_owner)return false;'),
     ('census-without-switch',ADAPTER,'if(native_census_detail::uncursored(s,0x135e00u,0x135e9bu)){','if(!s.instruction_cursor&&s.services.instruction){'),
    ),guard_mutants=('uncertified-required-owner','census-without-switch'))
if __name__=='__main__':raise SystemExit(main(SPEC,corpus))
