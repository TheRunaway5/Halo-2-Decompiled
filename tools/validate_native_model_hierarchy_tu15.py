#!/usr/bin/env python3
"""Private full1365C0 render-model hierarchy submission proof against the physical original.

Unchanged TU1.5 instructions of 1365C0 (including its recursive layer and
attached-child calls) run single-stepped on physical i386. Each run stops
before 136A80 (model draw submission) and 4B5C0 (model build); deterministic
stubs supply their effects. See tools/native_shadow_frame_oracle.py for the
comparison.
"""
import random,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from native_shadow_frame_oracle import Spec,Callee,LIVE,main

REGIONS=((0x4c6000,0x1000),(0x4ec000,0x1000),(0x4f0000,0x1000),(0x61000000,0x10000),(0x62000000,0x10000))
VIEWS=0x61000000;SUBMISSIONS=0x61004000;OBJECTS=0x61006000;OBJ_TABLE=0x61008000;OBJ_DATA=0x61008100
TAGS=0x61009000;TAG_DEFS=0x6100a000;COUNTERS=0x6100c000;STACK=0x6200c000
NONE=0xffffffff
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
QNAN=0x7fc00021;NEGZERO=0x80000000;DENORMAL=0x00000005

def submit_model_stub(ctx):
    """136A80(submission in EAX, suppress_children on stack)."""
    sub=ctx.entry_gp[0]
    suppress=ctx.get(ctx.entry_gp[4]+4)
    ctx.put(sub+0x1a0,(0x55aa0000|(ctx.call<<8)|(suppress&0xff))&0xffffffff)

def model_build_stub(ctx):
    """4B5C0(handle, build_param, build_flags, &sub+4, &pos, vis_mask)."""
    sp=ctx.entry_gp[4]
    out=ctx.get(sp+0x10)
    sub=out-4
    # Alternate populated vs empty built child models; keep layer_flags 0 on
    # built children so recursion depth stays bounded.
    sections=1 if (ctx.call%2==0) else 0
    ctx.put(sub+0x166,sections,2)
    ctx.put(sub+0x170,0)
    ctx.put(sub+0x1a4,(0x77cc0000+ctx.call*13)&0xffffffff)

def build(image,rng,family,v):
    banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
    def put(at,value,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
        raise ValueError('Unbacked model hierarchy fixture '+hex(at))
    def get(at,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:return int.from_bytes(bank[at-base:at-base+width],'little')
        raise ValueError('Unbacked model hierarchy input '+hex(at))
    # Object hierarchy:
    #   obj 1: parent with 2 siblings (obj 2 -> obj 3 -> NONE), obj 2 has grandchild obj 4
    #   obj 5: parent with 1 child (obj 6 -> NONE), obj 6 is type-2 with shadow_override != 0
    #   obj 7: parent with 2 children (obj 8 type-2 override==0 -> obj 9 type-1 -> NONE)
    #   obj 10: childless (first_child == NONE)
    put(0x4ecbd0,OBJ_TABLE)
    put(OBJ_TABLE+0x44,OBJ_DATA)
    put(0x4f0414,TAGS)
    def author_object(idx,first_child,next_sibling,obj_type,tag_idx,shadow_override):
        obj=OBJECTS+idx*0x100
        put(OBJ_DATA+idx*12+8,obj)
        put(obj+0x00,0x74610000|tag_idx)
        put(obj+0x0c,next_sibling)
        put(obj+0x10,first_child)
        put(obj+0xaa,obj_type,1)
        tdef=TAG_DEFS+tag_idx*0x300
        put(TAGS+tag_idx*16+8,tdef)
        put(tdef+0x290,shadow_override,2)
    author_object(1,0x00010002,NONE,0,1,0)
    author_object(2,0x00020004,0x00030003,2,2,1)
    author_object(3,NONE,NONE,2,3,0)
    author_object(4,NONE,NONE,1,4,1)
    author_object(5,0x00050006,NONE,0,5,0)
    author_object(6,NONE,NONE,2,6,3)
    author_object(7,0x00070008,NONE,0,7,0)
    author_object(8,NONE,0x00080009,2,8,0)
    author_object(9,NONE,NONE,1,9,5)
    author_object(10,NONE,NONE,0,10,0)

    # Camera forward globals
    fwd_sets=((1.0,0.0,0.0),(0.25,-0.5,0.75),(-0.125,0.875,-0.25),(0.0,-0.0,1.5))
    fx,fy,fz=fwd_sets[(family+v)%len(fwd_sets)]
    put(0x4c65cc,QNAN if (family==2 and v==6) else bits(fx))
    put(0x4c65d0,DENORMAL if (family==2 and v==7) else bits(fy))
    put(0x4c65d4,NEGZERO if (family==2 and v==5) else bits(fz))

    view=VIEWS+v*0x40
    put(view+0x2a68,bits(float(v)-2.5))
    put(view+0x2a6c,bits(float(family)*0.5-1.0))
    put(view+0x2a70,bits(1.25-float(v)*0.25))
    put(view+0x2a78,v%3)

    sub=SUBMISSIONS+v*0x200
    counter=COUNTERS+v*16
    put(counter,10+v)

    section_count=1+(v%3)
    include_children=1
    direct_submit=0
    skip_children=0
    layer_flags=0
    has_sort=0
    sort_kind=0
    parent_handle=0x000a000a # obj 10 (childless) by default
    df=False

    if family==0:
        # section_count <= 0 and direct_submit == 1 paths
        section_count=(0,-1,-32768,1,2,0,1,-2)[v]&0xffff
        direct_submit=(0,0,0,1,1,1,0,1)[v]
        include_children=(0,1,0,0,1,1,1,0)[v]
        parent_handle=0x0001000a if v%2==0 else 0x00010005
        df=(v==0)
    elif family==1:
        # Translucent (0x80) and Alpha (0x02) passes, with & without sort slot
        layer_flags=(0x80,0x80,0x02,0x02,0x82,0x82,0x80,0x02)[v]
        has_sort=v%2
        include_children=0
        df=(v==1)
    elif family==2:
        # Overlay A (0x01) and Overlay B (0x20) + combinations with 0x02 & NaN/denormal
        layer_flags=(0x01,0x20,0x21,0x03,0x22,0x23,0x81,0xa3)[v]
        has_sort=(v>>1)&1
        include_children=0
    elif family==3:
        # Children traversal: skip_children, childless, 1 child, 2 siblings + grandchild
        section_count=(1,0,1,1,0,1,1,2)[v]
        direct_submit=1
        include_children=(1,1,1,1,1,0,1,1)[v]
        skip_children=(0,0,1,0,0,0,0,0)[v]
        parent_handle=(0x00010001,0x00010001,0x00010001,0x00010005,0x00010007,0x00010001,0x0001000a,0x00010007)[v]
        sort_kind=(1,1,1,1,1,1,0,2)[v]
    elif family==4:
        # Children with sort_kind==1 triggering type-2 shadow_override reset vs non-reset
        section_count=0
        include_children=1
        parent_handle=(0x00010005,0x00010007,0x00010001,0x00010005,0x00010007,0x00010001,0x00010005,0x00010007)[v]
        sort_kind=(1,1,1,0,1,2,1,1)[v]
    else:
        # Combined layers + children + varied direct/skip/has_sort
        layer_flags=(0x00,0x80,0x02,0x01,0x20,0xa1,0x83,0xa3)[v]
        has_sort=(v+family)%2
        direct_submit=1 if v==0 else 0
        include_children=1 if v%2==0 else 0
        skip_children=1 if v==6 else 0
        parent_handle=(0x00010005,0x0001000a,0x00010007,0x0001000a,0x00010005,0x0001000a,0x00010001,0x0001000a)[v]
        sort_kind=1 if v in(0,4) else 0

    put(sub+0x000,parent_handle)
    put(sub+0x166,section_count,2)
    put(sub+0x16c,skip_children,1)
    put(sub+0x170,layer_flags)
    for k in range(4):
        put(sub+0x174+k*4,0x53480000|((family<<4)|(v<<2)|k))
        put(sub+0x184+k*4,bits(0.125*(k+1+v)))
    put(sub+0x194,bits(float(v)-1.25))
    put(sub+0x198,bits(float(family)+0.75))
    put(sub+0x19c,bits(-0.5*float(v)+2.0))
    put(sub+0x1a8,0x00001000|v)
    put(sub+0x1ac,0x00000015|(v<<1))
    put(sub+0x1b0,0x00000042|(v<<2))
    put(sub+0x1b4,0x00000080|v)
    put(sub+0x1b8,0x00000020|v)
    put(sub+0x1c4,sort_kind)
    put(sub+0x1c8,counter)
    put(sub+0x1cc,has_sort,1)

    sp=STACK+4*(v%4)
    put(sp+0x00,0x63000000)
    put(sp+0x04,sub)
    put(sp+0x08,include_children)
    put(sp+0x0c,direct_submit)
    put(sp+0x10,0xaa000000|(family<<8)|v)

    gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp;gp[1]=view
    xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
    depth=(v+family)%4;cw=(0x7f,0x27f,0x37f)[v%3]|((v%4)<<10);status=((v%8)<<11)|0x4500
    return dict(name=f'family{family}-variant{v}',entry=0x1365c0,gp=gp,eflags=0x202|(0x400 if df else 0)|(rng.getrandbits(12)&0x8d5),
        matrix=get(sp+4),source=get(sp+8),max_steps=100000,xmm=xmm,
        before_fp=[cw,status,0x1f80|((family%4)<<13)|(0,0x40,0x8000,0x8040)[v%4]],live_x87=list(LIVE[:depth]),
        stub=v%5,domain=family if family in(0,3)else'ordinary',
        banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)])

def corpus(image):
    rng=random.Random(0x1365c0);return[build(image,rng,family,v)for family in range(6)for v in range(8)]

LOGIC='src/rendering/native_tu15_model_hierarchy_systems.hpp';ADAPTER='src/rendering/native_tu15_model_hierarchy.hpp'
HELPERS='''static void run_1365c0(WorldState&);
static void run_136a80(WorldState&s){s.required(0x1368f2u,0x136a80u,false);}
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){if(target==selected(0x1365c0u,s.image)){run_1365c0(s);return;}if(target==selected(0x136a80u,s.image)){run_136a80(s);return;}s.required(site,target,tail);}
'''
SPEC=Spec(name='model_hierarchy',label='1365C0 model hierarchy',entry=0x1365c0,binding='native_model_hierarchy_replacements',replace='replace_model_hierarchy',
    body_pin='dd1a000d6ead4947ac3a4e3f2adf386a0f371ca386e68a4bcbc2fe4a372f8d75',owned={0x1365c0:0x136a77},entries=(0x1365c0,),
    callees={0x136a80:Callee(sites=(0x1368f2,),argument_bytes=4,effect=submit_model_stub),
             0x4b5c0:Callee(sites=(0x136a27,),argument_bytes=24,effect=model_build_stub)},regions=REGIONS,
    inputs=(LOGIC,ADAPTER,'tests/native_tu15_model_hierarchy_tests.cpp','tools/validate_native_model_hierarchy_tu15.py','tools/native_model_hierarchy_replacements.py'),
    reference_variable='HALO2_REFERENCE_MODEL_HIERARCHY',reference_test_variable='HALO2_MODEL_HIERARCHY_REFERENCE_TEST',census_test_variable='HALO2_MODEL_HIERARCHY_CENSUS_TEST',
    census_range=(0x1365c0,0x136a76),helpers=HELPERS,
    mutants=(
     ('unsigned-section-count',LOGIC,'if(sections>0) {','if(sections!=0) {'),
     ('wrong-translucent-bit',LOGIC,'inline constexpr std::uint32_t translucent_pass=0x80u;','inline constexpr std::uint32_t translucent_pass=0x40u;'),
     ('wrong-alpha-bit',LOGIC,'inline constexpr std::uint32_t alpha_pass=0x02u;','inline constexpr std::uint32_t alpha_pass=0x04u;'),
     ('wrong-overlay-a-bit',LOGIC,'inline constexpr std::uint32_t overlay_a=0x01u;','inline constexpr std::uint32_t overlay_a=0x04u;'),
     ('wrong-overlay-b-bit',LOGIC,'inline constexpr std::uint32_t overlay_b=0x20u;','inline constexpr std::uint32_t overlay_b=0x10u;'),
     ('wrong-overlay-flag',LOGIC,'inline constexpr std::uint32_t overlay_bit=0x8000u;','inline constexpr std::uint32_t overlay_bit=0x4000u;'),
     ('wrong-translucent-kind',LOGIC,'inline constexpr std::uint32_t translucent=3u;','inline constexpr std::uint32_t translucent=2u;'),
     ('wrong-alpha-kind',LOGIC,'inline constexpr std::uint32_t alpha=1u;','inline constexpr std::uint32_t alpha=2u;'),
     ('wrong-shadowed-type',LOGIC,'inline constexpr std::uint8_t shadowed_object_type=2u;','inline constexpr std::uint8_t shadowed_object_type=1u;'),
     ('wrong-child-clear-mask',LOGIC,'inline constexpr std::uint32_t child_clear_mask=0xffffffbfu;','inline constexpr std::uint32_t child_clear_mask=0xffffffffu;'),
     ('wrong-suppress-children',LOGIC,'frame.submit_base(kept,include_children==0u?1u:0u);','frame.submit_base(kept,include_children==0u?0u:1u);'),
     ('wrong-depth-op',LOGIC,'const auto depth=frame.add_scalar(frame.add_scalar(wz,wy),wx);\n                memory.store_float(t_store_depth,','const auto depth=frame.sub_scalar(frame.add_scalar(wz,wy),wx);\n                memory.store_float(t_store_depth,'),
     ('software-shift-right1',ADAPTER,'const auto shifted=shift_right1(cleared_flags,f.bits());r(eax)=shifted.value;f.load(shifted.flags);\n        push(0x13682eu,','r(eax)=f.right(cleared_flags,1u,4,false);\n        push(0x13682eu,'),
     ('software-tag-row',ADAPTER,'const auto row=tag_row(r(ecx),f.bits());r(ecx)=row.value;f.load(row.flags);','r(ecx)=f.shift(f.logic(r(ecx)&0xffffu,4),4u,4);'),
     ('uncertified-required-owner',ADAPTER,'if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;','if(!s.services.required_owner)return false;'),
     ('census-without-switch',ADAPTER,'if(native_census_detail::uncursored(s,0x1365c0u,0x136a76u)){','if(!s.instruction_cursor&&s.services.instruction){'),
    ),guard_mutants=('uncertified-required-owner','census-without-switch'))
if __name__=='__main__':raise SystemExit(main(SPEC,corpus))
