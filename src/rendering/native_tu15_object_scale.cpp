#include "halo2/native_tu15_object_scale.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <bit>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <emmintrin.h>
#endif

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory& m,std::uint32_t at,unsigned size=4) {
    std::array<std::byte,4> bytes{};m.read(at,std::span(bytes).first(size));std::uint32_t value{};
    for(unsigned n=0;n<size;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8*n);
    return value;
}
std::uint32_t body(const SourcePhysicsContactMemory& m,std::uint32_t object) {
    const auto pool=read(m,0x4ecbd0u),rows=read(m,pool+0x44u);
    return read(m,rows+12u*(object&65535u)+8u);
}
void store(SourcePhysicsContactMemory& m,const NativeTu15ObjectScalePublisher& writer,
    std::uint32_t at,std::uint32_t value) {
    std::array<std::byte,4> bytes{};
    for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8*n));
    if(writer)writer(at,bytes);else m.write(at,bytes);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
__m128 scalar(std::uint32_t v){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(v)));}
std::uint32_t bits(__m128 v){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 div(__m128 a,__m128 b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool above(__m128 a,__m128 b){unsigned char v;asm volatile("comiss %2,%1; seta %0":"=qm"(v):"x"(a),"x"(b):"cc","memory");return v;}
#endif
}
void native_tu15_object_scale_pose(SourcePhysicsContactMemory& m,std::uint32_t object_body,
    std::uint32_t old_scale,std::uint32_t scale,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ObjectScalePublisher& writer) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto offset=std::bit_cast<std::int16_t>(std::uint16_t(read(m,object_body+0x10eu,2)));
    const auto pose=object_body+std::uint32_t(std::int32_t(offset));
    auto component=read(m,pose+0x1cu);std::uint32_t ratio{};
    {detail::NativeTu15ArithmeticEnvironment environment(fp);ratio=bits(div(scalar(old_scale),scalar(scale)));}
    for(const auto at:{pose+0x1cu,pose+0x10u,pose+0x14u,pose+0x18u}) {
        if(at!=pose+0x1cu)component=read(m,at);
        std::uint32_t value{};
        {detail::NativeTu15ArithmeticEnvironment environment(fp);value=bits(mul(scalar(component),scalar(ratio)));}
        store(m,writer,at,value);
    }
#else
    (void)m;(void)object_body;(void)old_scale;(void)scale;(void)fp;(void)writer;
    throw std::runtime_error("Exact native TU1.5 object scale pose backend unavailable");
#endif
}
std::uint32_t native_tu15_object_scale_seconds(const SourcePhysicsContactMemory& m,
    std::uint16_t ticks,NativeTu15PhysicsFrameFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto multiplier=read(m,0x46a17cu);
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    const auto integer=std::int32_t(std::bit_cast<std::int16_t>(ticks));
    __m128 converted=_mm_setzero_ps();
    asm volatile("cvtsi2ss %1,%0":"+x"(converted):"r"(integer):"memory");
    return bits(mul(converted,scalar(multiplier)));
#else
    (void)m;(void)ticks;(void)fp;
    throw std::runtime_error("Exact native TU1.5 object scale duration backend unavailable");
#endif
}
void native_tu15_object_scale(SourcePhysicsContactMemory& m,std::uint32_t object,
    std::uint32_t scale,std::uint32_t seconds,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ObjectScaleServices& services,const NativeTu15ObjectScalePublisher& writer) {
    if(object==0xffffffffu)return;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto zero=read(m,0x46a0e8u);
    {detail::NativeTu15ArithmeticEnvironment environment(fp);if(!above(scalar(scale),scalar(zero)))return;}
    const auto pool=read(m,0x4ecbd0u),rows=read(m,pool+0x44u);
    const auto row_offset=12u*(object&65535u)+8u;
    const auto object_body=read(m,rows+row_offset),old_scale=read(m,object_body+0xa0u);
    store(m,writer,object_body+0xa0u,scale);
    // The source deliberately reloads the row after the scale publication.
    const auto reloaded_rows=read(m,pool+0x44u),reloaded_body=read(m,reloaded_rows+row_offset);
    if(read(m,reloaded_body+0x112u,2)!=0xffffu) {
        if(!services.interpolate)throw std::runtime_error("Native object scale requires its interpolation owner");
        services.interpolate(object,seconds,fp);
        native_tu15_object_scale_pose(m,object_body,old_scale,scale,fp,writer);
    }
    if(!services.refresh)throw std::runtime_error("Native object scale requires its model/bounds owner");
    services.refresh(object,fp);
#else
    (void)m;(void)scale;(void)seconds;(void)fp;(void)services;(void)writer;
    throw std::runtime_error("Exact native TU1.5 object scale backend unavailable");
#endif
}
void native_tu15_object_scale_hierarchy(SourcePhysicsContactMemory& m,std::uint32_t object,
    std::uint32_t scale,std::uint16_t ticks,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ObjectScaleServices& services,const NativeTu15ObjectScalePublisher& writer) {
    if(object==0xffffffffu)return;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto object_body=body(m,object);auto child=read(m,object_body+0x10u);
    // CVTSI2SS precedes the multiplier read in the source; the conversion is
    // exact for every signed SHORT, including -32768.
    const auto seconds=native_tu15_object_scale_seconds(m,ticks,fp);
    native_tu15_object_scale(m,object,scale,seconds,fp,services,writer);
    while(child!=0xffffffffu) {
        const auto child_body=body(m,child);
        native_tu15_object_scale_hierarchy(m,child,scale,ticks,fp,services,writer);
        child=read(m,child_body+0xcu);
    }
#else
    (void)m;(void)scale;(void)ticks;(void)fp;(void)services;(void)writer;
    throw std::runtime_error("Exact native TU1.5 object scale hierarchy backend unavailable");
#endif
}
}
