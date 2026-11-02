#include "halo2/native_tu15_constraint_auxiliary_math.hpp"
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
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth,unsigned temporaries,NativeTu15ConstraintAuxiliaryMathStats*stats):fp_(fp),previous_(_mm_getcsr()){
        if(temporaries>8u||depth>8u-temporaries||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
           (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            {if(stats)++stats->unsupported_profiles;throw std::runtime_error("Native TU1.5 constraint auxiliary math requires its unsupported numerical-fault owner");}
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::array<std::byte,28>native{};std::memcpy(native.data(),host_.data(),28);const std::uint16_t empty=0xffffu;
        std::memcpy(native.data(),&fp.control_word,2);std::memcpy(native.data()+4,&fp.status,2);
        std::memcpy(native.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));
        fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(previous_);}
};
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));
    std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;}
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
std::uint32_t native_tu15_normalize_constraint_axis(SourcePhysicsContactMemory&m,std::uint32_t vector,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observer,NativeTu15ConstraintAuxiliaryMathStats*stats){
    if(stats)++stats->calls;Environment environment(fp,depth,5u,stats);
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
    if(tiny){if(stats)++stats->small_vectors;return 0;}
    const auto one=scalar(read(m,0x46a0d0u));const auto first=scalar(read(m,vector));
    const auto reciprocal=divide(one,scalar(length));
    store(m,observer,0x30c1bu,vector,bits(mul(first,reciprocal)));
    const auto second=mul(reciprocal,scalar(read(m,vector+4u)));
    const auto third=mul(reciprocal,scalar(read(m,vector+8u)));
    store(m,observer,0x30c2cu,vector+4u,bits(second));store(m,observer,0x30c31u,vector+8u,bits(third));
    if(stats)++stats->normalized;return length;
}

namespace {
__m128 subtract(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool greater(__m128 a,__m128 b){unsigned char result;asm volatile("comiss %2,%1; seta %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result;}
std::uint32_t rounded_integer(__m128 value){const auto input=bits(value);std::int32_t output;
    asm volatile("flds %1; fistpl %0":"=m"(output):"m"(input):"memory","st");return std::bit_cast<std::uint32_t>(output);}
}
std::uint32_t native_tu15_pack_constraint_direction(const SourcePhysicsContactMemory&m,
    std::uint32_t vector,NativeTu15PhysicsFrameFp&fp,unsigned depth,NativeTu15ConstraintAuxiliaryMathStats*stats){
    if(stats)++stats->pack_calls;Environment environment(fp,depth,1u,stats);
    const auto minimum=scalar(read(m,0x46a0dcu)),maximum=scalar(read(m,0x46a0d0u));
    const auto clamp=[&](std::uint32_t p,bool reload){
        if(greater(minimum,scalar(read(m,p))))return minimum;
        const auto value=scalar(read(m,p));if(greater(value,maximum))return maximum;
        return reload?scalar(read(m,p)):value;
    };
    const auto z=clamp(vector+8u,true),y=clamp(vector+4u,true),x=clamp(vector,false);
    const auto packed_z=rounded_integer(mul(subtract(z,minimum),scalar(read(m,0x46a338u))));
    const auto scale=scalar(read(m,0x46a33cu));
    const auto packed_y=rounded_integer(mul(subtract(y,minimum),scale));
    const auto packed_x=rounded_integer(mul(subtract(x,minimum),scale));
    return (((packed_z<<11u)|packed_y)<<11u)|packed_x;
}
std::array<std::uint32_t,3> native_tu15_constraint_body_linear_velocity(const SourcePhysicsContactMemory&m,
    NativeTu15PhysicsComponent component,std::uint32_t slot,NativeTu15ConstraintAuxiliaryMathStats*stats){
    if(stats)++stats->body_velocity_calls;const auto motions=read(m,component.identity+0x70u),body=read(m,motions+slot*0x60u+0x40u);
    if(read(m,body+0x40u,1)==0){const auto motion=read(m,body+0x3cu),z=read(m,motion+0x48u),y=read(m,motion+0x44u),x=read(m,motion+0x40u);
        return{x,y,z};}
    if(stats)++stats->default_velocities;const auto fallback=read(m,0x474d04u);
    const auto x=read(m,fallback),y=read(m,fallback+4u),z=read(m,fallback+8u);return{x,y,z};
}
} // namespace halo2
