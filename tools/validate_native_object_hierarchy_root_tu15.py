#!/usr/bin/env python3
"""Private fullBBD10 object parent-hierarchy attached predicate proof against the physical original.

Unchanged TU1.5 instructions of BBD10 run single-stepped on physical i386
(pure leaf, no callees). See tools/native_shadow_frame_oracle.py for the
comparison.
"""
import random,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from native_shadow_frame_oracle import Spec,LIVE,main

REGIONS=((0x4ec000,0x1000),(0x61000000,0x10000),(0x62000000,0x4000))
TABLE=0x61000000;HEADERS=0x61001000;OBJECTS=0x61004000;STACK=0x62002000
NONE=0xffffffff

def build(image,rng,family,v):
    banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
    def put(at,value,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
        raise ValueError('Unbacked object hierarchy root fixture '+hex(at))
    def get(at,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:return int.from_bytes(bank[at-base:at-base+width],'little')
        raise ValueError('Unbacked object hierarchy root input '+hex(at))
    put(0x4ecbd0,TABLE)
    put(TABLE+0x44,HEADERS)
    # Build a chain of 1..5 objects: indices i0 -> i1 -> i2 -> i3 -> NONE
    indices=[((family*11+v*7+step*5)&0x3f)+1 for step in range(5)]
    chain_len=(1,2,3,4,1,2,3,4)[v]
    # Decide termination cause:
    #   family 0: reaches root NONE -> returns 0 (AL=0)
    #   family 1: stops on first node header flag 0x10 -> returns 1
    #   family 2: stops on first node object flag 0x01 -> returns 1
    #   family 3: stops on ancestor (step >= 1) header flag 0x10 -> returns 1
    #   family 4: stops on ancestor (step >= 1) object flag 0x01 -> returns 1
    #   family 5..7: mixed other bits in header_flags (~0x10) and object_flags (~0x01)
    for step in range(chain_len):
        idx=indices[step]
        row=HEADERS+idx*12
        obj=OBJECTS+idx*0x40
        h_flag=(rng.getrandbits(8)&~0x10)
        o_flag=(rng.getrandbits(8)&~0x01)
        parent=NONE if step+1==chain_len else ((0xabcd0000+step*0x11110000)|indices[step+1])
        if family==1 and step==0:h_flag|=0x10
        elif family==2 and step==0:o_flag|=0x01
        elif family==3 and step==chain_len-1:h_flag|=0x10
        elif family==4 and step==chain_len-1:o_flag|=0x01
        elif family==5 and step==v%chain_len:h_flag=0xef if v%2 else 0xff
        elif family==6 and step==v%chain_len:o_flag=0xfe if v%2 else 0xff
        elif family==7 and step==chain_len-1 and v%2:o_flag|=0x01
        put(row+0,0x1000+idx,2)
        put(row+2,h_flag,1)
        put(row+8,obj,4)
        put(obj+4,o_flag,1)
        put(obj+0x14,parent,4)
    sp=STACK+4*(v%4)
    put(sp,0x63000000)
    gp=[rng.getrandbits(32)for _ in range(8)]
    gp[4]=sp
    gp[1]=((rng.getrandbits(16)<<16)|indices[0])
    depth=(v+family)%4;cw=(0x7f,0x27f,0x37f)[v%3]|((v%4)<<10);status=((v%8)<<11)|0x4500
    return dict(name=f'family{family}-variant{v}',entry=0xbbd10,gp=gp,eflags=0x202|(0x400 if v%2 else 0)|(rng.getrandbits(12)&0x8d5),matrix=get(sp+4),source=get(sp+8),max_steps=100000,
        xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[cw,status,0x1f80|((family%4)<<13)|(0,0x40,0x8000,0x8040)[v%4]],live_x87=list(LIVE[:depth]),
        stub=0,domain=family if family in(0,1,2)else'ordinary',banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)])
def corpus(image):
    rng=random.Random(0xbbd10);return[build(image,rng,family,v)for family in range(8)for v in range(8)]

LOGIC='src/rendering/native_tu15_object_hierarchy_root_systems.hpp';ADAPTER='src/rendering/native_tu15_object_hierarchy_root.hpp'
SPEC=Spec(name='object_hierarchy_root',label='BBD10 object hierarchy root',entry=0xbbd10,binding='native_object_hierarchy_root_replacements',replace='replace_object_hierarchy_root',
    body_pin='d172508db4b53046a0a2fea880967003ec23ef2377ab303f2743d96ae7028b08',owned={0xbbd10:0xbbd48},entries=(0xbbd10,),
    callees={},regions=REGIONS,
    inputs=(LOGIC,ADAPTER,'tests/native_tu15_object_hierarchy_root_tests.cpp','tools/validate_native_object_hierarchy_root_tu15.py','tools/native_object_hierarchy_root_replacements.py'),
    reference_variable='HALO2_REFERENCE_OBJECT_HIERARCHY_ROOT',reference_test_variable='HALO2_OBJECT_HIERARCHY_ROOT_REFERENCE_TEST',census_test_variable='HALO2_OBJECT_HIERARCHY_ROOT_CENSUS_TEST',
    census_range=(0xbbd10,0xbbd47),
    helpers='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){s.required(site,target,tail);}\n',
    mutants=(
     ('wrong-header-bit',LOGIC,'inline constexpr std::uint8_t parent_flagged_bit=0x10u;','inline constexpr std::uint8_t parent_flagged_bit=0x20u;'),
     ('wrong-object-bit',LOGIC,'inline constexpr std::uint8_t root_flagged_bit=0x01u;','inline constexpr std::uint8_t root_flagged_bit=0x02u;'),
     ('skip-header-flag-exit',LOGIC,'if(h_flags&header_flags::parent_flagged_bit)return frame.finish(true);','if(false && (h_flags&header_flags::parent_flagged_bit))return frame.finish(true);'),
     ('skip-object-flag-exit',LOGIC,'if(o_flags&object_flags::root_flagged_bit)return frame.finish(true);','if(false && (o_flags&object_flags::root_flagged_bit))return frame.finish(true);'),
     ('no-parent-loop',LOGIC,'if(handle==none)return frame.finish(false);','if(true)return frame.finish(false);'),
     ('invert-result',LOGIC,'if(handle==none)return frame.finish(false);','if(handle==none)return frame.finish(true);'),
     ('clobber-high-eax-on-begin',ADAPTER,'void begin_walk(std::uint32_t){s_.set(eax,1u,1,0);}','void begin_walk(std::uint32_t){r(eax)=1u;}'),
     ('clobber-high-eax-on-false',ADAPTER,'if(!result)s_.set(eax,s_.flags.logic(0u,1),1,0);','if(!result)r(eax)=s_.flags.logic(0u,4);'),
     ('skip-ecx-row-address',ADAPTER,'void step_header(std::uint32_t row,std::uint32_t,std::uint32_t){r(ecx)=row;}','void step_header(std::uint32_t,std::uint32_t,std::uint32_t){}'),
     ('uncertified-required-owner',ADAPTER,'if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;','if(!s.services.required_owner)return false;'),
     ('census-without-switch',ADAPTER,'if(native_census_detail::uncursored(s,0xbbd10u,0xbbd47u)){','if(!s.instruction_cursor&&s.services.instruction){'),
    ),guard_mutants=('uncertified-required-owner','census-without-switch'))
if __name__=='__main__':raise SystemExit(main(SPEC,corpus))
