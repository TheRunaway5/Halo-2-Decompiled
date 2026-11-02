#include "halo2/native_tu15_draw_units.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <bit>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <emmintrin.h>
#endif

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory& m,std::uint32_t at,unsigned size=4) {
    std::array<std::byte,4> bytes{};m.read(at,std::span(bytes).first(size));std::uint32_t v{};
    for(unsigned i=0;i<size;++i)v|=std::to_integer<unsigned>(bytes[i])<<(8*i);return v;
}
void byte(SourcePhysicsContactMemory& m,std::uint32_t at,std::uint32_t value) {
    const auto b=std::byte(value);m.write(at,std::span(&b,1));
}
std::int32_t sx16(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using Scalar=__m128;
Scalar scalar(std::uint32_t b){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(b)));}
std::uint32_t bits(Scalar v){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar sub(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar mul(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar div(Scalar a,Scalar b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool above(Scalar a,Scalar b){unsigned char v;asm volatile("comiss %2,%1; seta %0":"=qm"(v):"x"(a),"x"(b):"cc","memory");return v;}
bool below(Scalar a,Scalar b){unsigned char v;asm volatile("comiss %2,%1; setb %0":"=qm"(v):"x"(a),"x"(b):"cc","memory");return v;}
bool ordered_equal(Scalar a,Scalar b) {
    unsigned char equal;asm volatile("ucomiss %2,%1; lahf; testb $0x44,%%ah; setnp %0":"=qm"(equal):"x"(a),"x"(b):"eax","cc","memory");return equal;
}
Scalar absolute_depth(Scalar v){const auto zero=scalar(0);return below(v,zero)?sub(zero,v):v;}
Scalar limited_depth(const SourcePhysicsContactMemory& m,Scalar v) {
    v=absolute_depth(v);const auto epsilon=scalar(read(m,0x46a178u));return above(v,epsilon)?v:epsilon;
}
std::uint8_t alpha(std::uint32_t value,std::uint32_t factor) {
    std::uint32_t stored{},delta{},result{};std::int64_t integer{};
    // Whole136173..32CC0C. The x87 stack stays inside this block, and the
    // last FSTP preserves the original C1 and sticky status for its caller.
    asm volatile(
        "flds %[value]\n\tfmuls %[factor]\n\tfld %%st(0)\n\tfsts %[stored]\n\tfistpq %[integer]\n\tfildq %[integer]\n\t"
        "movl %[stored],%%edx\n\tmovl %[integer],%%eax\n\ttestl %%eax,%%eax\n\tjnz 1f\n\t"
        "movl 4+%[integer],%%edx\n\ttestl $0x7fffffff,%%edx\n\tjnz 1f\n\t"
        "fstps %[stored]\n\tfstps %[stored]\n\tjmp 4f\n\t"
        "1: .byte 0xde,0xe9\n\ttestl %%edx,%%edx\n\tjns 2f\n\t"
        "fstps %[delta]\n\tmovl %[delta],%%ecx\n\txorl $0x80000000,%%ecx\n\taddl $0x7fffffff,%%ecx\n\tadcl $0,%%eax\n\tjmp 4f\n\t"
        "2: fstps %[delta]\n\tmovl %[delta],%%ecx\n\taddl $0x7fffffff,%%ecx\n\tsbbl $0,%%eax\n\t"
        "4: movl %%eax,%[result]"
        :[stored]"=m"(stored),[integer]"=m"(integer),[delta]"=m"(delta),[result]"=m"(result)
        :[value]"m"(value),[factor]"m"(factor):"eax","ecx","edx","cc","memory","st","st(1)");
    return std::uint8_t(result);
}
#endif
}
std::uint32_t native_tu15_draw_flags_135e00(const SourcePhysicsContactMemory& m,
    std::uint16_t queue,std::uint32_t context,NativeTu15PhysicsFrameFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint32_t result=(((queue>>2)&0x2c00u)|(queue&0x4000u))>>9;
    result|=(queue&0x200u)<<5;
    if(read(m,context+0x2a60u,1)&128u)result|=1u;
    if(read(m,0x4c6710u)!=3u)return result;
    const auto datum=read(m,0x4c67acu);
    if(datum!=0xffffffffu) {
        bool equal;{
            detail::NativeTu15ArithmeticEnvironment environment(fp);
            const auto fraction=read(m,0x4c681cu),zero=read(m,0x46a0e8u);
            equal=ordered_equal(scalar(fraction),scalar(zero));
        }
        if(equal && !(queue&0x400u)) {
            const auto tags=read(m,0x4f0414u),definition=read(m,tags+16u*(datum&65535u)+8u);
            if(!(read(m,definition,1)&2u))result|=0x100u;
        }
    }
    if(!read(m,0x4c6824u,1)||(queue&0x400u))result|=0x80u;
    return result;
#else
    (void)m;(void)queue;(void)context;(void)fp;throw std::runtime_error("Native135E00 arithmetic backend unavailable");
#endif
}
void native_tu15_draw_clusters_135ea0(SourcePhysicsContactMemory& m,std::uint32_t context,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15DrawUnitServices& services) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    auto queue=read(m,context+0xcu);
    if(sx16(read(m,queue+4u,2))<=0)return;
    for(std::uint32_t index=0;;) {
        const auto at=std::uint32_t(sx16(index));
        const auto flags=read(m,queue+8u),input_flags=read(m,flags+2u*at,2);
        const auto unit_flags=native_tu15_draw_flags_135e00(m,std::uint16_t(input_flags),context,fp);
        const auto ids=read(m,queue+0xcu),cluster=read(m,ids+4u*at),bsp=read(m,0x4ecc18u);
        std::uint32_t coverage{},association{},center{};
        {
            detail::NativeTu15ArithmeticEnvironment environment(fp);
            const auto half=scalar(read(m,0x46a0ccu));const auto clusters=read(m,bsp+0xa0u),row=clusters+176u*cluster;
            auto x=scalar(read(m,row+0x54u));x=add(x,scalar(read(m,row+0x58u)));
            auto y=scalar(read(m,row+0x5cu)),z=scalar(read(m,row+0x64u));
            z=add(z,scalar(read(m,row+0x68u)));y=add(y,scalar(read(m,row+0x60u)));
            const auto ymax=scalar(read(m,row+0x60u)),zmax=scalar(read(m,row+0x68u));
            x=mul(x,half);z=mul(z,half);y=mul(y,half);
            const auto dx=bits(sub(scalar(read(m,row+0x58u)),x)),dy=bits(sub(ymax,y)),dz=bits(sub(zmax,z));
            auto depth=scalar(read(m,0x4c6658u));
            association=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(read(m,context+0x1cu+std::uint32_t(sx16(cluster)),1)))));
            depth=mul(depth,z);auto term=mul(scalar(read(m,0x4c6640u)),x);
            auto camera_y=scalar(read(m,0x4c664cu));depth=add(depth,term);camera_y=mul(camera_y,y);depth=add(depth,camera_y);
            std::uint32_t radius{};
            asm volatile("flds %1; fmuls %1; flds %2; fmuls %2; faddp; flds %3; fmuls %3; faddp; fsqrt; fstps %0"
                :"=m"(radius):"m"(dx),"m"(dy),"m"(dz):"memory","st","st(1)");
            depth=add(depth,scalar(read(m,0x4c6664u)));depth=limited_depth(m,depth);
            center=read(m,0x474ce8u);auto scale=scalar(read(m,0x4c66f0u));scale=div(scale,depth);
            scale=mul(scale,scalar(radius));scale=mul(scale,scalar(read(m,0x46a11cu)));coverage=bits(scale);
        }
        NativeTu15DrawUnitCall call;call.geometry=cluster;call.instance=0xffffffffu;
        call.stack={unit_flags|0x40u,0xffffffffu,0xffffffffu,0,0,association,255,0,0xffffffffu,coverage,
            0xffffffffu,0xffffffffu,0,0,0,0,0,0,0,center,0};
        native_tu15_draw_unit_2cbd0(m,call,fp,services);
        queue=read(m,context+0xcu);const auto count=sx16(read(m,queue+4u,2));
        if(std::int32_t(++index)>=count)return;
    }
#else
    (void)m;(void)context;(void)fp;(void)services;throw std::runtime_error("Native135EA0 arithmetic backend unavailable");
#endif
}
void native_tu15_draw_instances_136030(SourcePhysicsContactMemory& m,std::uint32_t context,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15DrawUnitServices& services) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto bsp=read(m,0x4ecc18u);auto queue=read(m,context+0x18u);
    if(sx16(read(m,queue+4u,2))<=0)return;
    for(std::uint32_t index=0;;) {
        const auto at=std::uint32_t(sx16(index));queue=read(m,context+0x18u);
        const auto ids=read(m,queue+0xcu),id=std::uint32_t(sx16(read(m,ids+4u*at,2)));
        const auto flags=read(m,queue+8u),instances=read(m,bsp+0x144u),input_flags=read(m,flags+2u*at,2),row=instances+88u*id;
        auto unit_flags=native_tu15_draw_flags_135e00(m,std::uint16_t(input_flags),context,fp);
        std::uint32_t coverage{},association{};std::uint8_t opacity{};
        {
            detail::NativeTu15ArithmeticEnvironment environment(fp);
            auto depth=scalar(read(m,row+0x40u));depth=mul(depth,scalar(read(m,0x4c664cu)));
            auto term=scalar(read(m,row+0x44u));term=mul(term,scalar(read(m,0x4c6658u)));depth=add(depth,term);
            term=scalar(read(m,row+0x3cu));term=mul(term,scalar(read(m,0x4c6640u)));depth=add(depth,term);
            depth=add(depth,scalar(read(m,0x4c6664u)));depth=limited_depth(m,depth);
            queue=read(m,context+0x18u);auto projected=scalar(read(m,row+0x48u));
            const auto associations=read(m,queue+0x14u);association=read(m,associations+2u*at,2);
            projected=div(projected,depth);projected=mul(projected,scalar(read(m,0x4c66f0u)));
            const auto threshold=scalar(read(m,0x46a218u)),radius=scalar(read(m,row+0x48u));
            const bool small=above(threshold,radius);projected=mul(projected,scalar(read(m,0x46a11cu)));
            if(small)projected=mul(projected,scalar(read(m,0x46a364u)));
            coverage=bits(projected);auto fade=sub(projected,scalar(read(m,0x46a638u)));
            fade=mul(fade,scalar(read(m,0x449ea4u)));const auto zero=scalar(0);
            if(above(zero,fade))fade=zero;
            else {const auto one=scalar(read(m,0x46a0d0u));if(above(fade,one))fade=one;}
            opacity=alpha(bits(fade),read(m,0x46a148u));
        }
        queue=read(m,context+0x18u);const auto current_flags=read(m,queue+8u);
        unit_flags|=(read(m,current_flags+2u*at+1u,1)&8u)?1u:0x40u;
        if(opacity) {
            const auto radius=read(m,row+0x48u);NativeTu15DrawUnitCall call;call.instance=id;
            call.stack={unit_flags|0x1008u,0xffffffffu,0xffffffffu,0,0,association,opacity,row,0xffffffffu,coverage,
                0xffffffffu,0xffffffffu,0,0,0,0,0,0,0,row+0x3cu,radius};
            native_tu15_draw_unit_2cbd0(m,call,fp,services);
        } else {
            queue=read(m,context+0x18u);const auto skipped=read(m,queue+8u)+2u*at;
            byte(m,skipped,read(m,skipped,1)|1u);
        }
        queue=read(m,context+0x18u);const auto count=sx16(read(m,queue+4u,2));
        if(std::int32_t(++index)>=count)return;
    }
#else
    (void)m;(void)context;(void)fp;(void)services;throw std::runtime_error("Native136030 arithmetic backend unavailable");
#endif
}
}
