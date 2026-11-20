#!/usr/bin/env python3
"""Private fullC3AB0 light volume shape proof against the physical original.

Unchanged TU1.5 instructions of C3AB0 run single-stepped on physical i386
(pure leaf, no callees). See tools/native_shadow_frame_oracle.py for the
comparison.
"""
import random,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from native_shadow_frame_oracle import Spec,LIVE,main

REGIONS=((0x46a000,0x1000),(0x474000,0x1000),(0x4f0000,0x1000),(0x61000000,0x10000),(0x62000000,0x4000))
IMAGE_BANKS=(0x46a000,)
TRANSFORMS=0x61000000;SHAPES=0x61002000;DEFS=0x61004000;TAGS=0x61005000;GROUPS=0x61006000;BDATA=0x61007000;PAR_TAN=0x61008000;STACK=0x62002000
NONE=0xffffffff
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
QNAN=0x7fc00055;NEGZERO=0x80000000;DENORMAL=0x00000009

def build(image,rng,family,v):
    banks=[bytearray(image.read(base,size))if base in IMAGE_BANKS else bytearray(rng.randbytes(size))for base,size in REGIONS]
    def put(at,value,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
        raise ValueError('Unbacked light shape fixture '+hex(at))
    def get(at,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:return int.from_bytes(bank[at-base:at-base+width],'little')
        raise ValueError('Unbacked light shape input '+hex(at))

    put(0x474cd4,PAR_TAN)
    put(PAR_TAN,bits(0.75))
    put(PAR_TAN+4,bits(0.5))
    put(0x4f0414,TAGS)
    for k in range(1,10):
        g=GROUPS+k*0x80;b=BDATA+k*0x20
        put(TAGS+k*16+8,g)
        put(g+0x48,b)
        put(b+4,(64,128,256,512,32,64,128,256,16)[k-1],2)
        put(b+6,(32,64,256,128,64,96,128,192,48)[k-1],2)

    xf=TRANSFORMS+v*0x80
    sh=SHAPES+v*0x100
    ld=DEFS+v*0x100

    # Transform vectors
    ox,oy,oz=1.0+0.25*v,-0.5*family,2.0-0.125*v
    tx,ty,tz=ox+2.0,oy+0.5,oz-1.0
    fx,fy,fz=0.8,0.6,0.0
    ux,uy,uz=0.0,0.0,1.0
    scale=1.5 if v%2==0 else 0.75

    kind=(0,0,1,1,2,2,3,2)[v]
    ra,rb=1.25+0.25*v,2.0-0.25*v
    fov=0.75+0.125*v
    nd,fd=0.5,10.0+v
    gel=NONE if v%2==0 else (0x62690000|(1+v))

    if family==0:
        # Sphere (kind==0): a>b, a<b, a==b, NaN
        kind=0
        ra,rb=[(3.0,1.0),(1.0,3.0),(2.0,2.0),(0.0,-0.0),(5.0,2.5),(0.25,4.0),(1.0,1.0),(2.0,1.0)][v]
    elif family==1:
        # Fallback direction (|proj| <= eps or proj < 0)
        kind=(1,2,2,3,1,2,2,3)[v]
        if v<4:tx,ty,tz=ox,oy,oz # zero diff -> |proj| <= eps
        else:tx,ty,tz=ox-3.0*fx,oy-3.0*fy,oz-3.0*fz # negative proj
    elif family==2:
        # Parallel frustum (kind==1) with & without gel bitmap
        kind=1;gel=NONE if v<4 else (0x62690000|(1+v))
    elif family==3:
        # Spot frustum (kind==2) with tan_x > eps vs tan_x <= eps (fov==0 / negative)
        kind=2
        fov=(0.8,1.2,0.0,-0.5,0.5,0.00001,1.0,0.25)[v]
        gel=NONE if v%2==0 else (0x62690000|(1+v))
    else:
        # General frustum (kind==2,3) with varied NaN/denormal/negzero
        kind=(2,3,2,1,2,0,2,3)[v]
        if family==4 and v==7:fov=0.5

    put(xf+0x00,QNAN if (family==4 and v==6) else bits(ox))
    put(xf+0x04,DENORMAL if (family==4 and v==5) else bits(oy))
    put(xf+0x08,NEGZERO if (family==4 and v==4) else bits(oz))
    put(xf+0x0c,bits(tx));put(xf+0x10,bits(ty));put(xf+0x14,bits(tz))
    put(xf+0x28,bits(fx));put(xf+0x2c,bits(fy));put(xf+0x30,bits(fz))
    put(xf+0x34,bits(ux));put(xf+0x38,bits(uy));put(xf+0x3c,bits(uz))
    put(xf+0x40,bits(scale))

    put(sh+0x00,kind)
    put(sh+0x04,QNAN if (family==0 and v==7) else bits(ra))
    put(sh+0x08,bits(rb))
    put(sh+0x0c,bits(fov))
    put(sh+0x10,bits(nd))
    put(sh+0x14,bits(fd))
    put(ld+0x7c,gel)

    sp=STACK+4*(v%4)
    put(sp,0x63000000);put(sp+4,ld)
    gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp;gp[1]=xf;gp[2]=sh
    xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
    depth=(v+family)%4;cw=(0x7f,0x27f,0x37f)[v%3]|((v%4)<<10);status=((v%8)<<11)|0x4500
    return dict(name=f'family{family}-variant{v}',entry=0xc3ab0,gp=gp,eflags=0x202|(0x400 if v%2 else 0)|(rng.getrandbits(12)&0x8d5),
        matrix=get(sp+4),source=get(sp+8),max_steps=100000,xmm=xmm,
        before_fp=[cw,status,0x1f80|((family%4)<<13)|(0,0x40,0x8000,0x8040)[v%4]],live_x87=list(LIVE[:depth]),
        stub=0,domain=family if family in(0,3)else'ordinary',
        banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)])

def corpus(image):
    rng=random.Random(0xc3ab0);return[build(image,rng,family,v)for family in range(5)for v in range(8)]

LOGIC='src/rendering/native_tu15_light_shape_systems.hpp';ADAPTER='src/rendering/native_tu15_light_shape.hpp'
HELPERS='''static std::uint32_t native_single_spill(long double x,std::uint16_t &status) {
    float result;std::uint16_t actual;
    asm volatile("fldt %2\\n\\tfstps %0\\n\\tfnstsw %1":"=m"(result),"=m"(actual):"m"(x):"st");
    status=(status&~0x200u)|(actual&0x200u);
    return std::bit_cast<std::uint32_t>(result);
}
static long double native_fptan(long double x,std::uint16_t &status) {
    long double result;
    asm volatile("fldt %2\\n\\tfptan\\n\\tfnstsw %%ax\\n\\ttestw $0x400,%%ax\\n\\tjnz 1f\\n\\tfstp %%st(0)\\n\\t1: fstpt %1":"=a"(status),"=m"(result):"m"(x):"cc","st","st(1)");
    return result;
}
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){s.required(site,target,tail);}
'''
SPEC=Spec(name='light_shape',label='C3AB0 light shape',entry=0xc3ab0,binding='native_light_shape_replacements',replace='replace_light_shape',
    body_pin='57cb71c616c429f69bc65c54d926e848d981ea4839537b97daef44ea709949a1',owned={0xc3ab0:0xc3db5},entries=(0xc3ab0,),
    callees={},regions=REGIONS,
    inputs=(LOGIC,ADAPTER,'tests/native_tu15_light_shape_tests.cpp','tools/validate_native_light_shape_tu15.py','tools/native_light_shape_replacements.py'),
    reference_variable='HALO2_REFERENCE_LIGHT_SHAPE',reference_test_variable='HALO2_LIGHT_SHAPE_REFERENCE_TEST',census_test_variable='HALO2_LIGHT_SHAPE_CENSUS_TEST',
    census_range=(0xc3ab0,0xc3db4),helpers=HELPERS,free_x87_topology=True,
    x87_deltas={0xc3b5d:1,0xc3b64:1,0xc3c21:1,0xc3c2b:1,0xc3c2e:1,0xc3c31:1},
    mutants=(
     ('wrong-sphere-choice',LOGIC,'choose_a?a_check:b','choose_a?b:a_check'),
     ('skip-fallback-check',LOGIC,'if(!valid_proj) {','if(false && !valid_proj) {'),
     ('skip-gel-aspect',LOGIC,'if(gel!=none) {','if(gel==0u) {'),
     ('wrong-parallel-kind',LOGIC,'inline constexpr std::uint32_t parallel=1u;','inline constexpr std::uint32_t parallel=2u;'),
     ('wrong-spot-kind',LOGIC,'inline constexpr std::uint32_t spot=2u;','inline constexpr std::uint32_t spot=3u;'),
     ('wrong-cross-rz',LOGIC,'const auto rz=frame.sub_scalar(uy_fx,f_y_ux);','const auto rz=frame.add_scalar(uy_fx,f_y_ux);'),
     ('wrong-spot-neg',LOGIC,'const auto neg_off=frame.sub_scalar(0.0f,off);','const auto neg_off=frame.add_scalar(0.0f,off);'),
     ('wrong-far-center',LOGIC,'const auto far_cx=frame.add_scalar(fdx_fc,','const auto far_cx=frame.sub_scalar(fdx_fc,'),
     ('software-tag-row',ADAPTER,'const auto row=tag_row(r(edi),f.bits());r(edi)=row.value;f.load(row.flags);','r(edi)=f.shift(f.logic(r(edi)&0xffffu,4),4u,4);'),
     ('zero-upper-cvtsi2ss',ADAPTER,'s_.xmm[3]=cvtsi2ss(s_.xmm[3],std::int32_t(r(edi)));','s_.xmm[3]=cvtsi2ss(_mm_setzero_ps(),std::int32_t(r(edi)));'),
     ('ucomiss-instead-of-comiss',ADAPTER,'asm volatile("comiss %4,%3;setb %0;sete %1;setp %2"','asm volatile("ucomiss %4,%3;setb %0;sete %1;setp %2"'),
     ('uncertified-required-owner',ADAPTER,'if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;','if(!s.services.required_owner)return false;'),
     ('census-without-switch',ADAPTER,'if(native_census_detail::uncursored(s,0xc3ab0u,0xc3db4u)){','if(!s.instruction_cursor&&s.services.instruction){'),
    ),guard_mutants=('uncertified-required-owner','census-without-switch'))
if __name__=='__main__':raise SystemExit(main(SPEC,corpus))
