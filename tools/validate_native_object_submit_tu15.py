#!/usr/bin/env python3
"""Private full1362A0 render-object submission proof against the physical original.

Unchanged TU1.5 instructions of 1362A0 run single-stepped on physical i386.
Each run stops before 4BAE0 (object fade), 135E00 (object draw flags), 4B5C0
(model build) and 1365C0 (model hierarchy); deterministic stubs supply their
effects. See tools/native_shadow_frame_oracle.py for the comparison.
"""
import random,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from native_shadow_frame_oracle import Spec,Callee,LIVE,main

REGIONS=((0x46a000,0x1000),(0x4c6000,0x1000),(0x4ec000,0x1000),(0x61000000,0x10000),(0x62000000,0x4000))
IMAGE_BANKS=(0x46a000,)
VIEWS=0x61000000;QUEUES=0x61004000;HANDLES=0x61004200;ROW_FLAGS=0x61004400;VIS_IDX=0x61004600
OBJ_TABLE=0x61005000;OBJ_DATA=0x61005100;OBJECTS=0x61006000;STACK=0x62002000
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
QNAN=0x7fc00033;NEGZERO=0x80000000;DENORMAL=0x00000007

def fade_stub(ctx):
    """4BAE0(&fade_byte, &shadow_fade)."""
    sp=ctx.entry_gp[4]
    fade_out=ctx.get(sp+4)
    shadow_out=ctx.get(sp+8)
    ctx.put(fade_out,ctx.row['fade_byte'],1)
    ctx.put(shadow_out,0x55^ctx.row['fade_byte'],1)

def draw_flags_stub(ctx):
    """135E00(view) with EDX=row_flags -> EAX=render_flags (real 135E00 preserves EDX)."""
    ctx.gp[0]=0x0000001e|(ctx.entry_gp[2]&0x0100)
    if ctx.mode==0:ctx.gp[2]=ctx.entry_gp[2]

def model_build_stub(ctx):
    """4B5C0(handle, cam_dist, proj_size, &sub+4, &pos, vis_mask)."""
    sp=ctx.entry_gp[4]
    sub=ctx.get(sp+0x10)-4
    ctx.put(sub+0x16c,ctx.row['built_skip'],1)
    ctx.put(sub+0x170,ctx.row['built_layers'])
    ctx.put(sub+0x166,2,2)

def hierarchy_stub(ctx):
    """1365C0(sub, include_children, direct, force)."""
    sp=ctx.entry_gp[4]
    sub=ctx.get(sp+4)
    if ctx.mode==1:ctx.put(sub+0x16c,0x3c,1)

def build(image,rng,family,v):
    banks=[bytearray(image.read(base,size))if base in IMAGE_BANKS else bytearray(rng.randbytes(size))for base,size in REGIONS]
    def put(at,value,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
        raise ValueError('Unbacked object submit fixture '+hex(at))
    def get(at,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:return int.from_bytes(bank[at-base:at-base+width],'little')
        raise ValueError('Unbacked object submit input '+hex(at))

    # Globals: camera, plane, scales
    put(0x4c65c0,bits(1.5-float(v)*0.25))
    put(0x4c65c4,bits(-0.75+float(family)*0.5))
    put(0x4c65c8,bits(2.25+float(v)*0.125))
    put(0x4c65ec,bits(0.5 if v%2==0 else 1.25))
    plane_sign=-1.0 if family==1 else 1.0
    put(0x4c6640,bits(0.25*plane_sign))
    put(0x4c664c,bits(-0.5*plane_sign))
    put(0x4c6658,bits(0.75*plane_sign))
    put(0x4c6664,bits(-10.0 if (family==1 and v<4) else (0.0 if family==2 else 3.5)))
    put(0x4c66f0,bits(640.0))
    put(0x4c6848,bits(1.125))
    put(0x4ecbd0,OBJ_TABLE)
    put(OBJ_TABLE+0x44,OBJ_DATA)

    slot=v
    obj_idx=1+v
    obj=OBJECTS+obj_idx*0x80
    put(OBJ_DATA+obj_idx*12+8,obj)
    cx=bits(float(v)-3.0)
    cy=bits(float(family)-2.0)
    cz=bits(0.5*float(v)+1.0)
    rad=bits(0.75+0.25*float(v))
    if family==2 and v==6:cx=QNAN
    if family==2 and v==7:cz=DENORMAL
    if family==2 and v==5:cy=NEGZERO
    put(obj+0x30,cx);put(obj+0x34,cy);put(obj+0x38,cz);put(obj+0x3c,rad)

    pass_kind=(0,1,2,0,1,2,0,1)[v]
    retry=(0,1,0,1,0,0,1,0)[v]
    rflags=(0x0000,0x0200,0x0800,0x0a00,0x1000,0x0000,0x0800,0x0200)[v]
    vis_idx=(0x0005,0xffff,0x0007,0x0009,0xffff,0x0012,0x0003,0x0004)[v]
    fade_byte=(0xff,0x80,0x01,0x7f,0x00,0xff,0x00,0x40)[v]
    built_skip=(0,1,1,0,1,1,0,1)[v]
    built_layers=(0,2,0,2,0,2,2,0)[v]

    if family==0:pass_kind=(2,0,0,1,2,1,0,2)[v];retry=(0,0,1,0,1,1,0,0)[v]
    elif family==1:pass_kind=0;retry=0;fade_byte=0xff;built_layers=0 # negative plane dist + min clamp
    elif family==2:pass_kind=0;retry=0;fade_byte=0x80;built_layers=0 # near-zero / NaN / denormal plane dist
    elif family==3:pass_kind=1;retry=(0,0,0,0,1,1,1,1)[v];built_skip=(0,1,1,0,0,1,1,0)[v];fade_byte=0xff;built_layers=0
    elif family==4:fade_byte=(0,0,1,0x7f,0x80,0xff,0,2)[v];built_layers=(0,2,2,2,2,2,0,0)[v];pass_kind=(0,1,0,1,2,0,1,1)[v]
    elif family==5:rflags=(0x0000,0x0000,0x0800,0x0800,0x0200,0x0200,0x0a00,0x0a00)[v];vis_idx=(0x0011,0xffff,0x0022,0xffff,0x0033,0xffff,0x0044,0xffff)[v];fade_byte=0xff;built_layers=0

    view=VIEWS+v*0x40
    put(view+0x14,QUEUES)
    put(view+0x2a78,pass_kind)
    put(QUEUES+0x08,ROW_FLAGS)
    put(QUEUES+0x0c,HANDLES)
    put(QUEUES+0x14,VIS_IDX)
    put(HANDLES+slot*4,0x00010000|obj_idx)
    put(ROW_FLAGS+slot*2,rflags,2)
    put(VIS_IDX+slot*2,vis_idx,2)

    sp=STACK+4*(v%4)
    put(sp+0x00,0x63000000)
    put(sp+0x04,slot)
    put(sp+0x08,retry)
    put(sp+0x0c,0x61009000+v*4)
    put(sp+0x10,0xffffffff if v%2==0 else v)
    put(sp+0x14,1 if v%2==0 else 0)

    gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp;gp[6]=view
    xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
    depth=(v+family)%4;cw=(0x7f,0x27f,0x37f)[v%3]|((v%4)<<10);status=((v%8)<<11)|0x4500
    return dict(name=f'family{family}-variant{v}',entry=0x1362a0,gp=gp,eflags=0x202|(0x400 if v%2 else 0)|(rng.getrandbits(12)&0x8d5),
        matrix=get(sp+4),source=get(sp+8),max_steps=100000,xmm=xmm,
        before_fp=[cw,status,0x1f80|((family%4)<<13)|(0,0x40,0x8000,0x8040)[v%4]],live_x87=list(LIVE[:depth]),
        stub=v%2,fade_byte=fade_byte,built_skip=built_skip,built_layers=built_layers,
        domain=family if family in(0,4)else'ordinary',
        banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)])

def corpus(image):
    rng=random.Random(0x1362a0);return[build(image,rng,family,v)for family in range(6)for v in range(8)]

LOGIC='src/rendering/native_tu15_object_submit_systems.hpp';ADAPTER='src/rendering/native_tu15_object_submit.hpp'
HELPERS='''static std::uint32_t native_single_spill(long double x,std::uint16_t &status) {
    float result;std::uint16_t actual;
    asm volatile("fldt %2\\n\\tfstps %0\\n\\tfnstsw %1":"=m"(result),"=m"(actual):"m"(x):"st");
    status=(status&~0x200u)|(actual&0x200u);
    return std::bit_cast<std::uint32_t>(result);
}
static void run_4bae0(WorldState&s){s.required(0x13640fu,0x4bae0u,false);}
static void run_135e00(WorldState&s){s.required(0x13641fu,0x135e00u,false);}
static void run_1365c0(WorldState&s){s.required(0x136591u,0x1365c0u,false);}
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){s.required(site,target,tail);}
'''
SPEC=Spec(name='object_submit',label='1362A0 object submit',entry=0x1362a0,binding='native_object_submit_replacements',replace='replace_object_submit',
    body_pin='aea090432b62fb1db319b00357c90f8ba9934c631e000ff7714876cb4e8e942a',owned={0x1362a0:0x1365c0},entries=(0x1362a0,),
    callees={0x4bae0:Callee(sites=(0x13640f,),argument_bytes=8,effect=fade_stub),
             0x135e00:Callee(sites=(0x13641f,),argument_bytes=4,effect=draw_flags_stub),
             0x4b5c0:Callee(sites=(0x136501,),argument_bytes=24,effect=model_build_stub),
             0x1365c0:Callee(sites=(0x136591,),argument_bytes=16,effect=hierarchy_stub)},regions=REGIONS,
    inputs=(LOGIC,ADAPTER,'tests/native_tu15_object_submit_tests.cpp','tools/validate_native_object_submit_tu15.py','tools/native_object_submit_replacements.py'),
    reference_variable='HALO2_REFERENCE_OBJECT_SUBMIT',reference_test_variable='HALO2_OBJECT_SUBMIT_REFERENCE_TEST',census_test_variable='HALO2_OBJECT_SUBMIT_CENSUS_TEST',
    census_range=(0x1362a0,0x1365bf),helpers=HELPERS,free_x87_topology=True,
    x87_deltas={0x13638a:1,0x136392:1,0x13639c:1,0x1363a3:2,0x1363a4:2,0x1363ae:2,0x1363b5:3,0x1363bd:3,0x1363c3:3,0x1363cf:3,0x1363db:4,0x1363e6:5,0x1363f9:1,0x1363ff:1},
    mutants=(
     ('wrong-forced-pass',LOGIC,'inline constexpr std::uint32_t forced=2u;','inline constexpr std::uint32_t forced=1u;'),
     ('wrong-secondary-pass',LOGIC,'inline constexpr std::uint32_t secondary=1u;','inline constexpr std::uint32_t secondary=2u;'),
     ('signed-fade-check',LOGIC,'if((fade&0xffu)<=(k.zero&0xffu)) {','if(std::int8_t(fade)<=std::int8_t(k.zero&0xffu)) {'),
     ('wrong-culled-bit',LOGIC,'inline constexpr std::uint32_t culled=0x0001u;','inline constexpr std::uint32_t culled=0x0002u;'),
     ('wrong-vis-mask-shift',LOGIC,'inline constexpr std::uint32_t vis_mask_shift=9u;','inline constexpr std::uint32_t vis_mask_shift=8u;'),
     ('wrong-extra-visible',LOGIC,'inline constexpr std::uint32_t visible_indexed=0x0040u;','inline constexpr std::uint32_t visible_indexed=0x0020u;'),
     ('wrong-built-mask',LOGIC,'inline constexpr std::uint32_t built_mask=0x0208u;','inline constexpr std::uint32_t built_mask=0x0200u;'),
     ('wrong-retry-clear',LOGIC,'inline constexpr std::uint32_t retry_clear_mask=0xfffffffdu;','inline constexpr std::uint32_t retry_clear_mask=0xffffffffu;'),
     ('wrong-skip-clear',LOGIC,'inline constexpr std::uint32_t skip_clear_mask=0xfffffffbu;','inline constexpr std::uint32_t skip_clear_mask=0xffffffffu;'),
     ('skip-plane-negate',LOGIC,'if(neg) dist=frame.negate_plane(dist);','if(false && neg) dist=frame.negate_plane(dist);'),
     ('skip-plane-clamp',LOGIC,'if(frame.clamp_plane_min(dist,min_dist)) dist=min_dist;','if(false && frame.clamp_plane_min(dist,min_dist)) dist=min_dist;'),
     ('wrong-x87-sqrt',ADAPTER,'s_.fp.st(0)=x87_sqrt(s_.fp.st(0),s_.fp.status);','s_.fp.st(0)=x87_add_rr(s_.fp.st(0),s_.fp.st(0),s_.fp.status);'),
     ('ucomiss-instead-of-comiss',ADAPTER,'asm volatile("comiss %4,%3;setb %0;sete %1;setp %2"','asm volatile("ucomiss %4,%3;setb %0;sete %1;setp %2"'),
     ('wrong-x87-mul-scale',ADAPTER,'s_.fp.st(0)=x87_mul_m32(s_.fp.st(0),bits,s_.fp.status);','s_.fp.st(0)=x87_sub_m32(s_.fp.st(0),bits,s_.fp.status);'),
     ('uncertified-required-owner',ADAPTER,'if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;','if(!s.services.required_owner)return false;'),
     ('census-without-switch',ADAPTER,'if(native_census_detail::uncursored(s,0x1362a0u,0x1365bfu)){','if(!s.instruction_cursor&&s.services.instruction){'),
    ),guard_mutants=('uncertified-required-owner','census-without-switch'))
if __name__=='__main__':raise SystemExit(main(SPEC,corpus))
