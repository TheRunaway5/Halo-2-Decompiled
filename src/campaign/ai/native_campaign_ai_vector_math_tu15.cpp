#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include "halo2/native_tu15_normalize_vector.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <array>
#include <bit>
#include <cstring>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
NativeCampaignAiPendingDistanceErrorTu15::NativeCampaignAiPendingDistanceErrorTu15(
    NativeCampaignAiPendingDistanceTu15 pending,std::exception_ptr cause):
    std::runtime_error("Native TU1.5 distance publication rejected with pending extended result"),
    pending_(pending),cause_(std::move(cause)){}
void NativeCampaignAiDistancePublicationTu15::rethrow_if_failed()const {
    if(!failure)return;
    if(pending)throw NativeCampaignAiPendingDistanceErrorTu15(*pending,failure);
    std::rethrow_exception(failure);
}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> bytes{};
    if(const auto *page=memory.direct_page(address,4))std::memcpy(bytes.data(),page->bytes+(address&4095u),4);
    else memory.read(address,bytes);
    std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value) {
    if(!observe) {
        std::array<std::byte,4> bytes{};
        for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));
        memory.write(address,bytes);return;
    }
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(4)};
    if(observe){event.before.resize(4);memory.read(address,event.before);}
    for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using ArithmeticEnvironment=detail::NativeTu15ArithmeticEnvironment;
void load(std::uint32_t value){asm volatile("flds %0"::"m"(value):"memory","st");}
void subtract(std::uint32_t value){asm volatile("fsubs %0"::"m"(value):"memory","st");}
NativeCampaignAiPendingDistanceTu15 materialize_extended() {
    NativeCampaignAiPendingDistanceTu15 value;
    // Materialize the native scalar at the typed boundary, then retain that
    // same quantity for its owed publication. This is exact extended storage,
    // not a prematurely rounded single/double or a guest stack reconstruction.
    asm volatile("fstpt %0\n\tfldt %0":"=m"(value.extended)::"memory","st");
    return value;
}
void distance(const SourcePhysicsContactMemory &memory,std::uint32_t first,std::uint32_t second) {
    load(read(memory,first));subtract(read(memory,second));
    load(read(memory,first+4));subtract(read(memory,second+4));
    load(read(memory,first+8));subtract(read(memory,second+8));
    // Three real differences -> sqrt((dx*dx + dy*dy) + dz*dz).
    // Operand priority and every hardware precision boundary are retained;
    // unused arithmetic copies are not reproduced as virtual FP registers.
    asm volatile("fxch %%st(2)\n\tfmul %%st(0),%%st\n\tfxch %%st(1)\n\tfmul %%st(0),%%st\n\tfaddp %%st,%%st(1)\n\tfxch %%st(1)\n\tfmul %%st(0),%%st\n\tfaddp %%st,%%st(1)\n\tfsqrt"
        :::"memory","st","st(1)","st(2)");
}
NativeCampaignAiDistancePublicationTu15 publish_distance(SourcePhysicsContactMemory &memory,
    std::uint32_t destination,const NativeCampaignAiPendingDistanceTu15 &pending,
    const SourcePhysicsContactRecordObserver &observe) {
    NativeCampaignAiDistancePublicationTu15 result;
    SourcePhysicsContactRecordStore event{0x266ef9,destination,{},std::vector<std::byte>(4)};
    try {if(observe){event.before.resize(4);memory.read(destination,event.before);}}
    catch(...){result.pending=pending;result.failure=std::current_exception();return result;}
    alignas(16) std::array<std::byte,108> before{};
    asm volatile("fnsave %0\n\tfrstor %0":"=m"(before)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    std::uint32_t value;asm volatile("fstps %0":"=m"(value)::"memory","st");
    for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(n*8u));
    try {memory.write(destination,event.written);}
    catch(...) {
        // Only a speculative native spill is undone. The source rejected its
        // store before completing FSTP, so its actual pending quantity and
        // pre-publication status/TOP remain owed and are carried explicitly.
        asm volatile("frstor %0"::"m"(before):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        result.pending=pending;result.failure=std::current_exception();return result;
    }
    result.committed=true;result.written_bits=value;
    try {if(observe)observe(event);}catch(...){result.failure=std::current_exception();}
    return result;
}
using Scalar=__m128;
Scalar scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
Scalar divide(Scalar a,Scalar b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar multiply(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
// Shared norm instruction sequence: x*x + y*y, then z*z. Keep the destination
// operand order for NaN payload priority and the guest's x87 precision control.
#define HALO2_NORMALIZE_LENGTH "fmul %%st(0),%%st\n\tfxch %%st(1)\n\tfmul %%st(0),%%st\n\tfaddp %%st,%%st(1)\n\tfxch %%st(1)\n\tfmul %%st(0),%%st\n\tfaddp %%st,%%st(1)\n\tfsqrt"
template<class Input,class Sink> NativeTu15NormalizeOutcome normalize(const SourcePhysicsContactMemory &memory,
    const Input &input,const Sink &sink,std::optional<std::uint32_t> prepared_epsilon={},
    NativeTu15NormalizeConstants constants=native_tu15_normalize_tu15,bool resident=false) {
    NativeTu15NormalizeOutcome result;
    unsigned char too_small{},carry{},zero{},parity{};
    if(resident) {
        // No virtual memory/observer calls while a real x87 temporary is live.
        // An input exception therefore cannot leak a native stack value into
        // the translated caller. Private source stack scratch is elided here.
        const auto z=input(2),y=input(1),x=input(0);
        const auto epsilon=prepared_epsilon?*prepared_epsilon:read(memory,constants.epsilon);
        asm volatile("flds %5\n\tflds %6\n\tflds %7\n\t" HALO2_NORMALIZE_LENGTH
            "\n\tfsts %0\n\tfabs\n\tflds %8\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %1\n\tsetb %2\n\tsete %3\n\tsetp %4"
            :"=&m"(result.magnitude_bits),"=qm"(too_small),"=qm"(carry),"=qm"(zero),"=qm"(parity)
            :"m"(z),"m"(y),"m"(x),"m"(epsilon):"memory","cc","st","st(1)","st(2)");
    } else {
    load(input(2));load(input(1));load(input(0));
    // The source computes its norm entirely in x87 before the single-precision
    // reciprocal/vector phase. Do not replace this with an SSE squared norm.
    asm volatile(HALO2_NORMALIZE_LENGTH
        :::"memory","st","st(1)","st(2)");
    (void)materialize_extended();
    asm volatile("fsts %0\n\tfabs":"=m"(result.magnitude_bits)::"memory","st");
    const auto epsilon=prepared_epsilon?*prepared_epsilon:read(memory,constants.epsilon);
    asm volatile("flds %4\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0\n\tsetb %1\n\tsete %2\n\tsetp %3"
        :"=qm"(too_small),"=qm"(carry),"=qm"(zero),"=qm"(parity):"m"(epsilon):"memory","cc","st","st(1)");
    }
    result.compare=too_small?NativeTu15NormalizeCompare::above:
        parity?NativeTu15NormalizeCompare::unordered:zero?NativeTu15NormalizeCompare::equal:
        carry?NativeTu15NormalizeCompare::below:NativeTu15NormalizeCompare::above;
    if(too_small)return result;
    const auto one=scalar(read(memory,constants.one));
    const auto x=scalar(input(0));
    const auto reciprocal=divide(one,scalar(result.magnitude_bits));
    result.published[0]=bits(multiply(x,reciprocal));sink(0,result.published[0],0x30c1b);
    const auto y=multiply(reciprocal,scalar(input(1)));
    const auto z=multiply(reciprocal,scalar(input(2)));
    result.published[1]=bits(y);result.published[2]=bits(z);
    sink(1,result.published[1],0x30c2c);
    sink(2,result.published[2],0x30c31);
    return result;
}
#undef HALO2_NORMALIZE_LENGTH
#endif
}
NativeCampaignAiDistancePublicationTu15 native_campaign_ai_publish_perception_distance_tu15(
    SourcePhysicsContactMemory &memory,std::uint32_t first,std::uint32_t second,std::uint32_t destination,
    NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    ArithmeticEnvironment environment(fp);distance(memory,first,second);
    const auto pending=materialize_extended();return publish_distance(memory,destination,pending,observe);
#else
    (void)memory;(void)first;(void)second;(void)destination;(void)fp;(void)observe;
    throw std::runtime_error("Exact native TU1.5 vector arithmetic backend unavailable");
#endif
}
std::uint32_t native_campaign_ai_normalize_vector_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t vector,NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    return native_tu15_normalize_vector_30bd0(memory,vector,fp,observe).xmm0_bits();
}
NativeTu15NormalizeOutcome native_tu15_normalize_vector_30bd0(SourcePhysicsContactMemory &memory,
    std::uint32_t vector,NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe,
    NativeTu15NormalizeConstants constants) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    ArithmeticEnvironment environment(fp);
    return normalize(memory,[&](unsigned axis){return read(memory,vector+axis*4u);},
        [&](unsigned axis,std::uint32_t value,std::uint32_t site){publish(memory,observe,site,vector+axis*4u,value);},{},constants);
#else
    (void)memory;(void)vector;(void)fp;(void)observe;(void)constants;
    throw std::runtime_error("Exact native TU1.5 vector arithmetic backend unavailable");
#endif
}
[[gnu::noinline]] NativeTu15NormalizeOutcome native_tu15_normalize_vector_30bd0_resident(
    SourcePhysicsContactMemory &memory,std::uint32_t vector,const SourcePhysicsContactRecordObserver &observe,
    NativeTu15NormalizeConstants constants) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return normalize(memory,[&](unsigned axis){return read(memory,vector+axis*4u);},
        [&](unsigned axis,std::uint32_t value,std::uint32_t site){publish(memory,observe,site,vector+axis*4u,value);},
        {},constants,true);
#else
    (void)memory;(void)vector;(void)observe;(void)constants;
    throw std::runtime_error("Exact native TU1.5 vector arithmetic backend unavailable");
#endif
}
std::uint32_t native_campaign_ai_normalize_vector_value_tu15(const SourcePhysicsContactMemory &memory,
    std::array<std::uint32_t,3> &value,NativeTu15PhysicsFrameFp &fp) {
    return native_campaign_ai_normalize_vector_value_tu15(memory,value,fp,{});
}
std::uint32_t native_campaign_ai_normalize_vector_value_tu15(const SourcePhysicsContactMemory &memory,
    std::array<std::uint32_t,3> &value,NativeTu15PhysicsFrameFp &fp,
    const std::function<void(unsigned,std::uint32_t)> &publish) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto epsilon=read(memory,0x46a0ecu);
    std::array<std::pair<unsigned,std::uint32_t>,3> publications{};unsigned count{};
    std::uint32_t magnitude;
    {
        ArithmeticEnvironment environment(fp);
        magnitude=normalize(memory,[&](unsigned axis){return value[axis];},
            [&](unsigned axis,std::uint32_t bits,std::uint32_t){value[axis]=bits;if(publish)publications[count++]={axis,bits};},epsilon).xmm0_bits();
    }
    // Observers run under their host environment and cannot alter the source
    // arithmetic's controls or sticky flags.
    for(unsigned n=0;n<count;++n)publish(publications[n].first,publications[n].second);
    return magnitude;
#else
    (void)memory;(void)value;(void)fp;(void)publish;
    throw std::runtime_error("Exact native TU1.5 vector arithmetic backend unavailable");
#endif
}

}
