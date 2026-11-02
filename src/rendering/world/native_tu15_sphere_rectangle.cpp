#include "halo2/native_tu15_sphere_rectangle.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <bit>
#include <optional>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <emmintrin.h>
#endif

namespace halo2 {
std::uint32_t native_tu15_sphere_rectangle_return_flags(std::uint32_t stack,std::uint32_t incoming) {
    const auto left=stack-0x34u,sum=stack;
    const auto flags=std::uint32_t(sum<left) | (std::uint32_t(!(std::popcount(sum&255u)&1u))<<2) |
        ((left^0x34u^sum)&0x10u) | (std::uint32_t(sum==0)<<6) | ((sum>>24)&0x80u) |
        (((~(left^0x34u)&(left^sum))>>20)&0x800u);
    return (incoming&~0x8d5u)|flags;
}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory& m,std::uint32_t at,unsigned size=4) {
    std::array<std::byte,4> bytes{};m.read(at,std::span(bytes).first(size));std::uint32_t value{};
    for(unsigned i=0;i<size;++i)value|=std::to_integer<unsigned>(bytes[i])<<(8*i);return value;
}
std::int32_t signed_half(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using Scalar=__m128;
Scalar scalar(std::uint32_t b){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(b)));}
std::uint32_t bits(Scalar v){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar sub(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar mul(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar div(Scalar a,Scalar b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar integer(std::int32_t i){auto v=scalar(0);asm volatile("cvtsi2ss %1,%0":"+x"(v):"r"(i):"memory");return v;}
bool above(Scalar a,Scalar b){unsigned char v;asm volatile("comiss %2,%1; seta %0":"=qm"(v):"x"(a),"x"(b):"cc","memory");return v;}
std::uint32_t compare_byte(Scalar a,Scalar b) {
    std::uint32_t flags;asm volatile("ucomiss %2,%1; lahf; movl %%eax,%0":"=r"(flags):"x"(a),"x"(b):"eax","cc","memory");
    return (flags>>8)&255u;
}
Scalar root(Scalar v) {
    const auto input=bits(v);std::uint32_t output{};
    asm volatile("flds %1; fsqrt; fstps %0":"=m"(output):"m"(input):"memory","st");return scalar(output);
}
#endif
}
NativeTu15SphereRectangleResidue native_tu15_sphere_rectangle_30270(SourcePhysicsContactMemory& m,
    std::uint32_t center,std::uint32_t projection,std::uint32_t camera,std::uint32_t output,
    std::uint32_t radius_bits,NativeTu15PhysicsFrameFp& fp,const SourcePhysicsContactRecordObserver& observe) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::optional<detail::NativeTu15ArithmeticEnvironment> environment;environment.emplace(fp);
    const auto load=[&](auto at){return scalar(read(m,at));};
    const auto put=[&](std::uint32_t at,Scalar value,std::uint32_t pc) {
        const auto word=bits(value);std::array<std::byte,4> bytes{};
        for(unsigned i=0;i<4;++i)bytes[i]=std::byte(word>>(8*i));
        if(!observe){m.write(at,bytes);return;}
        SourcePhysicsContactRecordStore event{pc,at,{},std::vector(bytes.begin(),bytes.end())};
        event.before.resize(4);m.read(at,event.before);
        m.write(at,bytes);
        // Original store observers can enter other source services. Release
        // the guest environment at that boundary, then restore its actual FP.
        environment.reset();observe(event);environment.emplace(fp);
    };
    auto dx=load(center),dy=load(center+4u),dz=load(center+8u);const auto scale=load(projection);
    dx=sub(dx,load(camera));dy=sub(dy,load(camera+4u));dz=sub(dz,load(camera+8u));
    const auto ah=compare_byte(scale,load(0x46a0d0u));
    const auto early_eax=(center&0xffff00ffu)|(ah<<8);
    if((ah&0x44u)!=0x40u){dx=mul(scale,dx);dy=mul(scale,dy);dz=mul(scale,dz);}
    auto x=load(projection+0x1cu),term=load(projection+0x10u),y=load(projection+0x20u);
    term=mul(term,dy);x=mul(x,dz);x=add(x,term);term=mul(load(projection+4u),dx);x=add(x,term);
    term=mul(load(projection+0x14u),dy);y=mul(y,dz);y=add(y,term);term=mul(load(projection+8u),dx);y=add(y,term);
    auto depth=mul(load(projection+0x24u),dz);auto depth_y=mul(load(projection+0x18u),dy);
    auto depth_x=mul(load(projection+0xcu),dx);depth=add(depth,depth_y);depth=add(depth,depth_x);
    depth=mul(depth,load(0x46a0dcu));const auto radius=scalar(radius_bits);
    if(!above(add(depth,radius),load(0x46a18cu))) {
        const auto upper=load(0x45695cu),lower=load(0x44aac8u);
        put(output,upper,0x306d2u);put(output+4u,lower,0x306d6u);
        put(output+8u,upper,0x306dbu);put(output+12u,lower,0x306e0u);
        return {early_eax,projection,0};
    }
    const auto p00=load(projection+0x78u);auto eax=signed_half(read(m,camera+0x32u,2));
    const auto p20=load(projection+0x98u),p11=load(projection+0x8cu),p21=load(projection+0x9cu);
    auto ecx=signed_half(read(m,camera+0x30u,2));const auto half=load(0x46a0ccu);
    auto left=integer(eax);eax=signed_half(read(m,camera+0x36u,2));left=mul(left,half);
    auto top=integer(ecx);ecx=signed_half(read(m,camera+0x34u,2));top=mul(top,half);
    const auto right=mul(integer(eax),half),bottom=mul(integer(ecx),half);
    const auto depth_squared=mul(depth,depth),radius_squared=mul(radius,radius);
    auto length=mul(x,x);length=add(length,depth_squared);const auto discriminant=sub(length,radius_squared);
    std::uint8_t triples{};auto one=scalar(0);
    if(above(discriminant,load(0x46a0ecu))) {
        const auto ratio=div(radius,length),depth_radius=mul(depth,radius),q=root(discriminant);++triples;
        const auto qx=mul(q,x);auto tangent_a=mul(add(depth_radius,qx),ratio);
        auto tangent_b=mul(sub(depth_radius,qx),ratio);const auto depth_a=sub(depth,tangent_a);
        const auto q_depth=mul(q,depth),x_radius=mul(x,radius);
        auto lateral=mul(sub(x_radius,q_depth),ratio);const auto x_high=sub(x,lateral),depth_b=sub(depth,tangent_b);
        if(above(depth_a,load(0x46a18cu))) {
            auto tangent=mul(add(x_radius,q_depth),ratio);auto u=div(sub(x,tangent),depth_a);
            u=mul(u,p00);u=add(u,p20);one=load(0x46a0d0u);
            auto value=mul(sub(one,u),left);u=mul(add(u,one),right);value=add(value,u);
            put(output,value,0x304e4u);
        } else {const auto lower=load(0x44aac8u);one=load(0x46a0d0u);put(output,lower,0x304fau);}
        if(above(depth_b,load(0x46a18cu))) {
            auto u=div(x_high,depth_b);u=mul(u,p00);u=add(u,p20);
            auto value=mul(sub(one,u),left);u=mul(add(u,one),right);value=add(value,u);
            put(output+4u,value,0x3055cu);
        } else put(output+4u,load(0x45695cu),0x3055cu);
    } else {put(output,load(0x44aac8u),0x30550u);put(output+4u,load(0x45695cu),0x3055cu);}
    length=mul(y,y);length=add(length,depth_squared);const auto y_discriminant=sub(length,radius_squared);
    if(above(y_discriminant,load(0x46a0ecu))) {
        const auto ratio=div(radius,length),depth_radius=mul(depth,radius),q=root(y_discriminant);++triples;
        const auto qy=mul(q,y);auto tangent_a=mul(add(qy,depth_radius),ratio);const auto depth_a=sub(depth,tangent_a);
        const auto y_radius=mul(y,radius),q_depth=mul(q,depth);auto tangent_b=sub(depth_radius,qy);
        const auto epsilon=load(0x46a18cu);tangent_b=mul(tangent_b,ratio);
        auto lateral=sub(y_radius,q_depth);const auto depth_b=sub(depth,tangent_b);const bool first=above(depth_a,epsilon);
        lateral=mul(lateral,ratio);const auto y_high=sub(y,lateral);
        if(first) {
            auto tangent=mul(add(y_radius,q_depth),ratio);auto v=div(sub(y,tangent),depth_a);
            v=mul(v,p11);v=add(v,p21);one=load(0x46a0d0u);
            auto value=mul(sub(one,v),top);v=mul(add(v,one),bottom);value=add(value,v);
            put(output+8u,value,0x3064bu);
        } else {put(output+8u,load(0x44aac8u),0x3065au);one=load(0x46a0d0u);}
        if(above(depth_b,epsilon)) {
            auto v=div(y_high,depth_b);v=mul(v,p11);v=add(v,p21);
            auto value=mul(sub(one,v),top);v=mul(add(v,one),bottom);value=add(value,v);
            put(output+12u,value,0x30697u);
        } else put(output+12u,load(0x45695cu),0x306b7u);
    } else {put(output+8u,load(0x44aac8u),0x306aau);put(output+12u,load(0x45695cu),0x306b7u);}
    return {std::bit_cast<std::uint32_t>(eax),std::bit_cast<std::uint32_t>(ecx),triples};
#else
    (void)m;(void)center;(void)projection;(void)camera;(void)output;(void)radius_bits;(void)fp;(void)observe;
    throw std::runtime_error("Exact native30270 arithmetic backend unavailable");
#endif
}
}
