#!/usr/bin/env python3
"""Private full1373F0 render-view queue reset proof against the physical original.

Unchanged TU1.5 instructions of 1373F0 run single-stepped on physical i386
(pure leaf, no callees). See tools/native_shadow_frame_oracle.py for the
comparison.
"""
import random,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from native_shadow_frame_oracle import Spec,LIVE,main

REGIONS=((0x61000000,0x10000),(0x62000000,0x4000))
VIEWS=0x61002000;QUEUES=0x6100c000;STACK=0x62002000

def build(image,rng,family,v):
    banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
    def put(at,value,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
        raise ValueError('Unbacked view reset fixture '+hex(at))
    def get(at,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:return int.from_bytes(bank[at-base:at-base+width],'little')
        raise ValueError('Unbacked view reset input '+hex(at))
    view=VIEWS+v*0x1000
    for k in range(4):
        q=QUEUES+v*0x100+k*0x20
        put(view+0x0c+k*4,q)
        put(q+4,0x1234+k+v,2)
    count=(0,1,2,3,4,7,16,32)[v] if family==0 else (0,1,5,12,24,48,64,128)[v]
    put(view+0xa5c,count,2)
    put(view+0xa5e,0x5678,2)
    df=(family==1 and v==0)
    sp=STACK+4*(v%4)
    put(sp,0x63000000)
    gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp;gp[2]=view
    xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
    depth=(v+family)%4;cw=(0x7f,0x27f,0x37f)[v%3]|((v%4)<<10);status=((v%8)<<11)|0x4500
    return dict(name=f'family{family}-variant{v}',entry=0x1373f0,gp=gp,eflags=0x202|(0x400 if df else 0)|(rng.getrandbits(12)&0x8d5),
        matrix=get(sp+4),source=get(sp+8),max_steps=100000,xmm=xmm,
        before_fp=[cw,status,0x1f80|((family%4)<<13)|(0,0x40,0x8000,0x8040)[v%4]],live_x87=list(LIVE[:depth]),
        stub=0,domain=family if family==0 else'ordinary',
        banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)])

def corpus(image):
    rng=random.Random(0x1373f0);return[build(image,rng,family,v)for family in range(3)for v in range(8)]

LOGIC='src/rendering/native_tu15_view_reset_systems.hpp';ADAPTER='src/rendering/native_tu15_view_reset.hpp'
SPEC=Spec(name='view_reset',label='1373F0 view reset',entry=0x1373f0,binding='native_view_reset_replacements',replace='replace_view_reset',
    body_pin='3ecb0a9ac67f906dc05f359a40d7ef55f9c3f8e88f398a7b159089556fc86e4c',owned={0x1373f0:0x13745d},entries=(0x1373f0,),
    callees={},regions=REGIONS,
    inputs=(LOGIC,ADAPTER,'src/rendering/native_tu15_receiver_transfers.hpp','tests/native_tu15_view_reset_tests.cpp','tools/validate_native_view_reset_tu15.py','tools/native_view_reset_replacements.py'),
    reference_variable='HALO2_REFERENCE_VIEW_RESET',reference_test_variable='HALO2_VIEW_RESET_REFERENCE_TEST',census_test_variable='HALO2_VIEW_RESET_CENSUS_TEST',
    census_range=(0x1373f0,0x13745c),
    helpers='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){s.required(site,target,tail);}\n',
    mutants=(
     ('wrong-bitmask-dwords',LOGIC,'inline constexpr std::uint32_t bitmask_dwords=0x10u;','inline constexpr std::uint32_t bitmask_dwords=0x0fu;'),
     ('wrong-slot-dwords',LOGIC,'inline constexpr std::uint32_t cluster_slot_dwords=0x80u;','inline constexpr std::uint32_t cluster_slot_dwords=0x7fu;'),
     ('wrong-queue0-clear',LOGIC,'memory.store16(clear_queue0,at(q0,offsetof(RenderQueue,count)),0u);','memory.store16(clear_queue0,at(q0,offsetof(RenderQueue,count)),1u);'),
     ('wrong-queue3-clear',LOGIC,'memory.store16(clear_queue3,at(q3,offsetof(RenderQueue,count)),0u);','memory.store16(clear_queue3,at(q3,offsetof(RenderQueue,count)),1u);'),
     ('wrong-entry-clear',LOGIC,'memory.store16(clear_entry_count,at(view,offsetof(RenderView,entry_count)),0u);','memory.store16(clear_entry_count,at(view,offsetof(RenderView,entry_count)),1u);'),
     ('wrong-active-clear',LOGIC,'memory.store16(clear_active_count,at(view,offsetof(RenderView,active_count)),0u);','memory.store16(clear_active_count,at(view,offsetof(RenderView,active_count)),1u);'),
     ('wrong-slot-fill-value',LOGIC,'frame.fill_cluster_slots(at(view,offsetof(RenderView,cluster_slots)),cluster_slot_dwords,none);','frame.fill_cluster_slots(at(view,offsetof(RenderView,cluster_slots)),cluster_slot_dwords,0u);'),
     ('software-shift-roundtrip',ADAPTER,'const auto rt=shift_roundtrip(count&0xffffu,s_.flags.bits());\n        r(ebx)=rt.bytes;s_.flags.load(rt.flags);\n        stosd(0x137435u,dst,rt.value,value);','r(ebx)=s_.flags.shift(count&0xffffu,2u,4);\n        stosd(0x137435u,dst,s_.flags.right(r(ebx),2u,4,false),value);'),
     ('uncertified-required-owner',ADAPTER,'if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;','if(!s.services.required_owner)return false;'),
     ('census-without-switch',ADAPTER,'if(native_census_detail::uncursored(s,0x1373f0u,0x13745cu)){','if(!s.instruction_cursor&&s.services.instruction){'),
    ),guard_mutants=('uncertified-required-owner','census-without-switch'))
if __name__=='__main__':raise SystemExit(main(SPEC,corpus))
