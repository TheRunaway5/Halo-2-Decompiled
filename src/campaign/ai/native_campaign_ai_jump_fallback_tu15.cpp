#include "halo2/native_campaign_ai_jump_fallback_tu15.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <optional>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
NativeCampaignAiJumpPendingErrorTu15::NativeCampaignAiJumpPendingErrorTu15(
    NativeCampaignAiJumpPendingTu15 pending,std::exception_ptr cause):
    std::runtime_error("Native jump trajectory input rejected with pending mathematical quantity"),
    pending_(pending),cause_(std::move(cause)){}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> raw{};memory.read(address,raw);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(raw[n])<<(n*8u);
    return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(4)};
    if(observe){event.before.resize(4);memory.read(address,event.before);}
    for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class ArithmeticEnvironment {
    alignas(16) std::array<std::byte,108> host_{};
    std::array<std::byte,28> native_{};
    NativeTu15PhysicsFrameFp &fp_;unsigned host_mxcsr_;
public:
    explicit ArithmeticEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),host_mxcsr_(_mm_getcsr()) {
        if((fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
           (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native jump math requires masked original FP exceptions");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),native_.size());const std::uint16_t empty=0xffffu;
        std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);
        std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");
        _mm_setcsr(fp.mxcsr);
    }
    ~ArithmeticEnvironment() {
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(fp_.control_word),"=m"(fp_.status));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        _mm_setcsr(host_mxcsr_);
    }
};
NativeCampaignAiJumpPendingTu15 pending_quantity(NativeCampaignAiJumpPendingKindTu15 kind) {
    NativeCampaignAiJumpPendingTu15 pending{kind,{}};
    alignas(16) std::array<std::byte,108> before{};
    asm volatile("fnsave %0\n\tfrstor %0":"=m"(before)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    asm volatile("fstpt %0":"=m"(pending.quantity)::"memory","st");
    asm volatile("frstor %0"::"m"(before):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    return pending;
}
void load(std::uint32_t value){asm volatile("flds %0"::"m"(value):"memory","st");}
void subtract(std::uint32_t value){asm volatile("fsubs %0"::"m"(value):"memory","st");}
std::uint32_t spill_single(){std::uint32_t value;asm volatile("fstps %0":"=m"(value)::"memory","st");return value;}
using Scalar=__m128;
Scalar scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
Scalar multiply(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar divide(Scalar a,Scalar b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool equal_zero(Scalar value) {
    unsigned char equal{},ordered{};const auto zero=_mm_setzero_ps();
    asm volatile("ucomiss %3,%2\n\tsete %0\n\tsetnp %1":"=qm"(equal),"=qm"(ordered):"x"(value),"x"(zero):"cc","memory");
    return equal&&ordered;
}
bool greater(Scalar value,Scalar bound) {
    unsigned char result{};
    asm volatile("comiss %2,%1\n\tseta %0":"=qm"(result):"x"(value),"x"(bound):"cc","memory");return result;
}
template<class Sink> void calculate(const SourcePhysicsContactMemory &memory,
    const NativeCampaignAiJumpPointTu15 &first,const NativeCampaignAiJumpPointTu15 &second,
    std::uint32_t speed_bits,const Sink &sink) {
    std::optional<NativeCampaignAiJumpPendingKindTu15> pending;
    try {
        load(first.coordinate(0));pending=NativeCampaignAiJumpPendingKindTu15::FirstX;
        subtract(second.coordinate(0));const auto dx=spill_single();pending.reset();
        load(first.coordinate(1));pending=NativeCampaignAiJumpPendingKindTu15::FirstY;
        subtract(second.coordinate(1));const auto dy=spill_single();pending.reset();
        load(first.coordinate(2));pending=NativeCampaignAiJumpPendingKindTu15::FirstZ;
        subtract(second.coordinate(2));std::uint32_t dz{},magnitude;
        // The source retains extended dz while spilling its REAL multiplier:
        // sqrt(((dz_extended*dz_REAL)+(dy_REAL*dy_REAL))+(dx_REAL*dx_REAL)).
        asm volatile("fsts %0":"=m"(dz)::"memory","st");
        asm volatile("fmuls %1\n\tflds %2\n\tfmuls %2\n\tfaddp %%st,%%st(1)\n\tflds %3\n\tfmuls %3\n\tfaddp %%st,%%st(1)\n\tfsqrt\n\tfsts %0\n\tfabs"
            :"=m"(magnitude):"m"(dz),"m"(dy),"m"(dx):"memory","st","st(1)");
        pending=NativeCampaignAiJumpPendingKindTu15::AbsoluteDistance;
        const auto epsilon=read(memory,0x46a0ecu);unsigned char too_small{};
        asm volatile("flds %1\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0"
            :"=qm"(too_small):"m"(epsilon):"memory","cc","st","st(1)");
        pending.reset();std::array<std::uint32_t,3> direction{};
        if(too_small)magnitude=0;
        else {
            const auto one=scalar(read(memory,0x46a0d0u));
            const auto reciprocal=divide(one,scalar(magnitude));
            direction[0]=bits(multiply(reciprocal,scalar(dx)));
            direction[1]=bits(multiply(scalar(dy),reciprocal));
            direction[2]=bits(multiply(scalar(dz),reciprocal));
        }
        if(too_small||equal_zero(scalar(magnitude))) {
            const auto fallback=read(memory,0x474d10u);
            direction[0]=read(memory,fallback);direction[1]=read(memory,fallback+4u);direction[2]=read(memory,fallback+8u);
        }
        std::uint32_t duration{};
        if(greater(scalar(speed_bits),scalar(read(memory,0x46a0ecu))))
            duration=bits(divide(scalar(magnitude),scalar(speed_bits)));
        sink(NativeCampaignAiJumpFallbackValueTu15{direction,magnitude,speed_bits,duration});
    }catch(...) {
        if(pending)throw NativeCampaignAiJumpPendingErrorTu15(pending_quantity(*pending),std::current_exception());
        throw;
    }
}
#endif
}
void native_campaign_ai_jump_fallback_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t first,std::uint32_t second,std::uint32_t speed_bits,
    NativeCampaignAiJumpFallbackOutputsTu15 outputs,NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    ArithmeticEnvironment environment(fp);
    NativeCampaignAiJumpPointMemoryTu15 first_point(memory,first),second_point(memory,second);
    calculate(memory,first_point,second_point,speed_bits,[&](const NativeCampaignAiJumpFallbackValueTu15 &value) {
        const auto &direction=value.direction;const auto magnitude=value.distance_bits,duration=value.duration_bits;
        publish(memory,observe,0xff66c,outputs.direction,direction[0]);
        publish(memory,observe,0xff66e,outputs.direction+4u,direction[1]);
        publish(memory,observe,0xff671,outputs.direction+8u,direction[2]);
        if(outputs.distance)publish(memory,observe,0xff676,outputs.distance,magnitude);
        if(outputs.speed)publish(memory,observe,0xff67e,outputs.speed,speed_bits);
        if(outputs.duration)publish(memory,observe,0xff688,outputs.duration,duration);
    });
#else
    (void)memory;(void)first;(void)second;(void)speed_bits;(void)outputs;(void)fp;(void)observe;
    throw std::runtime_error("Exact native TU1.5 jump trajectory arithmetic unavailable");
#endif
}
NativeCampaignAiJumpFallbackValueTu15 native_campaign_ai_jump_fallback_value_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t first,std::uint32_t second,
    std::uint32_t speed_bits,NativeTu15PhysicsFrameFp &fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    NativeCampaignAiJumpPointMemoryTu15 first_point(memory,first),second_point(memory,second);
    return native_campaign_ai_jump_fallback_value_tu15(memory,first_point,second_point,speed_bits,fp);
#else
    (void)memory;(void)first;(void)second;(void)speed_bits;(void)fp;
    throw std::runtime_error("Exact native TU1.5 jump trajectory arithmetic unavailable");
#endif
}

NativeCampaignAiJumpFallbackValueTu15 native_campaign_ai_jump_fallback_value_tu15(
    const SourcePhysicsContactMemory &memory,const NativeCampaignAiJumpPointTu15 &first,
    const NativeCampaignAiJumpPointTu15 &second,std::uint32_t speed_bits,NativeTu15PhysicsFrameFp &fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    ArithmeticEnvironment environment(fp);NativeCampaignAiJumpFallbackValueTu15 value{};
    calculate(memory,first,second,speed_bits,[&](const NativeCampaignAiJumpFallbackValueTu15 &result){value=result;});
    return value;
#else
    (void)memory;(void)first;(void)second;(void)speed_bits;(void)fp;
    throw std::runtime_error("Exact native TU1.5 jump trajectory arithmetic unavailable");
#endif
}

}
