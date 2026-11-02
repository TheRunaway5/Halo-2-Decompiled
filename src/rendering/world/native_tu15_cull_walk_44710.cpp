#include "halo2/native_tu15_cull_walk_44710.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <bit>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <emmintrin.h>
#endif

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory& m,std::uint32_t at,unsigned size=4) {
    std::array<std::byte,4> b{};m.read(at,std::span(b).first(size));std::uint32_t v{};
    for(unsigned i=0;i<size;++i)v|=std::to_integer<unsigned>(b[i])<<(8*i);
    return v;
}
std::uint32_t signed_half(std::uint32_t v) {
    return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
__m128 scalar(std::uint32_t v){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(v)));}
std::uint32_t bits(__m128 v){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 div(__m128 a,__m128 b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool ordered_equal(__m128 a,__m128 b) {
    unsigned char equal,ordered;
    asm volatile("ucomiss %3,%2; sete %0; setnp %1":"=qm"(equal),"=qm"(ordered):"x"(a),"x"(b):"cc","memory");
    return equal && ordered;
}
std::array<std::uint32_t,3> inverse_point(const SourcePhysicsContactMemory& m,
    std::uint32_t transform,std::array<std::uint32_t,3> center,NativeTu15PhysicsFrameFp& fp) {
    const auto scale=read(m,transform);
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    // LAHF/test44/JNP tests the parity of the masked AH byte: only ordered
    // equality branches. Unordered values continue through the arithmetic.
    if(ordered_equal(scalar(scale),_mm_setzero_ps()))return {0,0,0};
    const auto reloaded_scale=read(m,transform),one=read(m,0x46a0d0u);
    auto x=sub(scalar(center[0]),scalar(read(m,transform+0x28u)));
    auto y=sub(scalar(center[1]),scalar(read(m,transform+0x2cu)));
    auto z=sub(scalar(center[2]),scalar(read(m,transform+0x30u)));
    if(!ordered_equal(scalar(reloaded_scale),scalar(one))) {
        const auto reciprocal=div(scalar(one),scalar(reloaded_scale));
        x=mul(reciprocal,x);y=mul(reciprocal,y);z=mul(reciprocal,z);
    }
    const auto component=[&](unsigned at) {
        auto v=mul(scalar(read(m,transform+at+8u)),z);
        v=add(v,mul(scalar(read(m,transform+at+4u)),y));
        return bits(add(v,mul(scalar(read(m,transform+at)),x)));
    };
    center[0]=component(4);center[1]=component(0x10);center[2]=component(0x1c);
    return center;
}
#endif
}
void native_tu15_cull_walk_44710(const SourcePhysicsContactMemory& m,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15CullWalkServices& services) {
    const auto prefix=read(m,0x4cd39cu),context=read(m,0x4cd3a0u);
    const auto mode=read(m,context+0x2a78u);
    const auto draw_count=read(m,0x4cd398u); // original CMOV reads its memory operand unconditionally
    const auto end=std::uint16_t(mode?draw_count:prefix);
    for(std::uint32_t unit=mode?prefix:0u;std::uint16_t(unit)<end;++unit) {
        const auto at=0x4c6958u+32u*signed_half(unit),flags=read(m,at);
        if((flags&1u) || !(flags&0x40u))continue;
        if(read(m,context+0x2a7cu)==1) {
            if(!services.sphere)throw std::runtime_error("Native44710 requires its sphere geometry owner");
            std::array<std::uint32_t,3> center{read(m,context+0x2a68u),read(m,context+0x2a6cu),read(m,context+0x2a70u)};
            if(const auto transform=read(m,at+0x1cu)) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
                center=inverse_point(m,transform,center,fp);
#else
                throw std::runtime_error("Exact native44710 inverse-point backend unavailable");
#endif
            }
            services.sphere(unit,center,read(m,context+0x2a74u),fp);
        } else {
            if(!services.frustum)throw std::runtime_error("Native44710 requires its frustum geometry owner");
            if(flags&8u) {
                const auto association=signed_half(read(m,at+0x18u,2));
                const auto row=read(m,0x4cd3a0u)+16u*(association+0xa6u);
                for(unsigned word=0;word<4;++word) {
                    auto remaining=read(m,row+4u*word);
                    while(remaining) {
                        const auto bit=unsigned(std::countr_zero(remaining));
                        remaining&=~(1u<<bit);
                        services.frustum(unit,word*32u+bit,fp);
                    }
                }
            } else services.frustum(unit,read(m,at+0x18u,2),fp);
        }
    }
}
}
