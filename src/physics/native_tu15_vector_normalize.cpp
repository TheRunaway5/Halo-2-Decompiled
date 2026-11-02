#include "halo2/native_tu15_vector_normalize.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
class Environment {
    alignas(16)std::array<std::byte,108>host_{};NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),previous_(_mm_getcsr()){
        if(depth>3||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
           (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native TU1.5 vector normalization requires its unsupported numerical-fault owner");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::array<std::byte,28>native{};std::memcpy(native.data(),host_.data(),28);const std::uint16_t empty=0xffffu;
        std::memcpy(native.data(),&fp.control_word,2);std::memcpy(native.data()+4,&fp.status,2);
        std::memcpy(native.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));
        fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(previous_);}
};
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);
    std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t pc,std::uint32_t p,std::uint32_t value){
    std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(4);m.read(p,event.before);event.written.assign(b.begin(),b.end());}m.write(p,b);if(o)o(event);
}
__m128 scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(__m128 value){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 divide(__m128 a,__m128 b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
}
std::uint32_t native_tu15_normalize_vector(SourcePhysicsContactMemory&m,std::uint32_t vector,
    std::uint32_t fallback,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observer,NativeTu15VectorNormalizeStats*stats,bool*normalized_path){
    if(stats)++stats->calls;Environment environment(fp,depth);
    const auto z=read(m,vector+8u),y=read(m,vector+4u),x=read(m,vector),epsilon=read(m,0x46a0ecu);
    std::uint32_t length;unsigned char tiny;
    // The original association and extended precision are one native hardware
    // operation. No source instruction decoder or recorded result participates.
    asm volatile("flds %[z]; flds %[y]; flds %[x]; fld %%st(0); fmul %%st(1),%%st; "
        "fld %%st(2); fmul %%st(3),%%st; faddp %%st,%%st(1); "
        "fld %%st(3); fmul %%st(4),%%st; faddp %%st,%%st(1); fsqrt; "
        "fstp %%st(3); fstp %%st(0); fstp %%st(0); fsts %[length]; fabs; "
        "flds %[epsilon]; fcomip %%st(1),%%st; fstp %%st(0); seta %[tiny]"
        :[length]"=m"(length),[tiny]"=qm"(tiny)
        :[x]"m"(x),[y]"m"(y),[z]"m"(z),[epsilon]"m"(epsilon)
        :"cc","memory","st","st(1)","st(2)","st(3)","st(4)");
    if(tiny){
        store(m,observer,0x122bcdu,vector,read(m,fallback));
        store(m,observer,0x122bd2u,vector+4u,read(m,fallback+4u));
        store(m,observer,0x122bd8u,vector+8u,read(m,fallback+8u));
        if(normalized_path)*normalized_path=false;
        if(stats)++stats->fallbacks;return 0;
    }
    const auto one=scalar(read(m,0x46a0d0u));const auto first=scalar(read(m,vector));
    const auto reciprocal=divide(one,scalar(length));
    store(m,observer,0x122babu,vector,bits(mul(first,reciprocal)));
    const auto second=mul(reciprocal,scalar(read(m,vector+4u)));
    const auto third=mul(reciprocal,scalar(read(m,vector+8u)));
    store(m,observer,0x122bbcu,vector+4u,bits(second));store(m,observer,0x122bc1u,vector+8u,bits(third));
    if(normalized_path)*normalized_path=true;
    if(stats)++stats->normalized;return length;
}
} // namespace halo2
