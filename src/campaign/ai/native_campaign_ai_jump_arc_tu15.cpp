#include "halo2/native_campaign_ai_jump_arc_tu15.hpp"
#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include <bit>
#include <cstring>
#include <optional>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
NativeCampaignAiJumpArcPendingErrorTu15::NativeCampaignAiJumpArcPendingErrorTu15(
    NativeCampaignAiJumpArcPendingTu15 pending,std::exception_ptr cause):
    std::runtime_error("Native jump arc input rejected with pending mathematical quantities"),
    pending_(std::move(pending)),cause_(std::move(cause)){}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> raw{};memory.read(address,raw);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(raw[n])<<(n*8u);
    return value;
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
            throw std::invalid_argument("Native jump arc requires masked original FP exceptions");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),native_.size());const std::uint16_t empty=0xffffu;
        std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);
        std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~ArithmeticEnvironment() {
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(fp_.control_word),"=m"(fp_.status));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);
    }
};
class SimdEnvironment {
    NativeTu15PhysicsFrameFp &fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),previous_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native jump arc requires masked SIMD exceptions");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
enum class Pending {HorizontalSquared,HorizontalDistance,DiscriminantRoot,FlightTimeSquared,HorizontalVelocitySquares,LaunchMagnitude};
NativeCampaignAiJumpArcPendingTu15 pending_quantities(Pending kind) {
    alignas(16) std::array<std::byte,108> before{};
    NativeCampaignAiJumpExtendedRealTu15 first{},second{};
    asm volatile("fnsave %0\n\tfrstor %0":"=m"(before)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    asm volatile("fstpt %0":"=m"(first.bytes)::"memory","st");
    if(kind==Pending::HorizontalVelocitySquares)asm volatile("fstpt %0":"=m"(second.bytes)::"memory","st");
    asm volatile("frstor %0"::"m"(before):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    switch(kind) {
    case Pending::HorizontalSquared:return NativeCampaignAiJumpHorizontalSquaredTu15{first};
    case Pending::HorizontalDistance:return NativeCampaignAiJumpHorizontalDistanceTu15{first};
    case Pending::DiscriminantRoot:return NativeCampaignAiJumpDiscriminantRootTu15{first};
    case Pending::FlightTimeSquared:return NativeCampaignAiJumpFlightTimeSquaredTu15{first};
    case Pending::HorizontalVelocitySquares:return NativeCampaignAiJumpHorizontalVelocitySquaresTu15{first,second};
    case Pending::LaunchMagnitude:return NativeCampaignAiJumpLaunchMagnitudeTu15{first};
    }
    throw std::logic_error("Unknown native jump quantity");
}
using Scalar=__m128;
Scalar scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
Scalar subtract(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar multiply(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar divide(Scalar a,Scalar b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool greater(Scalar a,Scalar b) {
    unsigned char result{};asm volatile("comiss %2,%1\n\tseta %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result;
}
bool below_or_unordered(Scalar a,Scalar b) {
    unsigned char result{};asm volatile("comiss %2,%1\n\tsetb %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result;
}
bool ordered_equal(Scalar a,Scalar b) {
    unsigned char equal{},ordered{};
    asm volatile("ucomiss %3,%2\n\tsete %0\n\tsetnp %1":"=qm"(equal),"=qm"(ordered):"x"(a),"x"(b):"cc","memory");return equal&&ordered;
}
void load(std::uint32_t value){asm volatile("flds %0"::"m"(value):"memory","st");}
std::uint32_t spill_single(){std::uint32_t value;asm volatile("fstps %0":"=m"(value)::"memory","st");return value;}
std::uint32_t square_root_single(std::uint32_t value) {
    load(value);asm volatile("fsqrt":::"memory","st");return spill_single();
}
struct Prepared {
    bool straight_fallback{},feasible=true;
    std::array<std::uint32_t,3> difference{},direction{};
};
Prepared prepare(const SourcePhysicsContactMemory &memory,const NativeCampaignAiJumpPointTu15 &first,
    const NativeCampaignAiJumpPointTu15 &second,std::uint32_t maximum_speed) {
    std::optional<Pending> pending;
    try {
        Prepared result;
        auto x=scalar(first.coordinate(0));x=subtract(x,scalar(second.coordinate(0)));
        auto y=scalar(first.coordinate(1));y=subtract(y,scalar(second.coordinate(1)));
        const auto horizontal_squared=add(multiply(y,y),multiply(x,x));
        load(bits(horizontal_squared));pending=Pending::HorizontalSquared;
        const auto gravity_address=read(memory,0x52be68u);
        asm volatile("fsqrt":::"memory","st");pending=Pending::HorizontalDistance;
        auto gravity=scalar(read(memory,gravity_address));
        auto z=scalar(first.coordinate(2));gravity=multiply(gravity,scalar(0x3f800000u));
        const auto zero=_mm_setzero_ps();const bool negative_gravity=greater(zero,gravity);
        z=subtract(z,scalar(second.coordinate(2)));
        const auto horizontal_distance=scalar(spill_single());pending.reset();
        result.difference={bits(x),bits(y),bits(z)};
        if(negative_gravity){result.straight_fallback=true;return result;}
        if(!greater(gravity,zero)){result.straight_fallback=true;return result;}
        const auto epsilon=scalar(read(memory,0x46a0ecu));
        if(!greater(horizontal_distance,epsilon)&&!greater(z,epsilon)) {
            result.straight_fallback=true;return result;
        }
        const auto squared_distance=add(multiply(z,z),horizontal_squared);
        auto quarter_gravity_squared=multiply(gravity,gravity);
        quarter_gravity_squared=multiply(quarter_gravity_squared,scalar(read(memory,0x46a130u)));
        auto discriminant_product=multiply(squared_distance,quarter_gravity_squared);
        discriminant_product=multiply(discriminant_product,scalar(read(memory,0x46a120u)));
        load(bits(discriminant_product));asm volatile("fsqrt":::"memory","st");pending=Pending::DiscriminantRoot;
        const auto denominator=multiply(quarter_gravity_squared,scalar(read(memory,0x46a11cu)));
        const auto gravity_height=multiply(gravity,z);
        asm volatile("fchs":::"memory","st");const auto negative_root=spill_single();pending.reset();
        load(read(memory,0x46a0dcu));
        const auto minimum_speed_squared=subtract(gravity_height,scalar(negative_root));
        const bool negative_minimum=greater(zero,minimum_speed_squared);
        const auto denominator_bits=bits(denominator);std::uint32_t fallback_time;
        asm volatile("fdivs %1\n\tfmuls %2\n\tfsqrt\n\tfstps %0":"=m"(fallback_time):"m"(denominator_bits),"m"(negative_root):"memory","st");
        const auto minimum_speed=negative_minimum?0u:square_root_single(bits(minimum_speed_squared));
        auto chosen_speed=maximum_speed,time=fallback_time;
        bool solved{};
        if(!below_or_unordered(scalar(maximum_speed),scalar(minimum_speed))) {
            const auto energy=subtract(gravity_height,multiply(scalar(maximum_speed),scalar(maximum_speed)));
            const bool negative_energy=greater(zero,energy);
            const auto discriminant=subtract(multiply(energy,energy),discriminant_product);
            if(negative_energy&&!below_or_unordered(discriminant,zero)) {
                load(bits(discriminant));const auto energy_bits=bits(energy);const std::int32_t high_arc_sign=1;
                asm volatile("fsqrt\n\tfimull %0\n\tfsubs %1\n\tfdivs %2"::"m"(high_arc_sign),"m"(energy_bits),"m"(denominator_bits):"memory","st");
                pending=Pending::FlightTimeSquared;const auto zero_bits=read(memory,0x46a0e8u);unsigned char positive{};
                asm volatile("flds %1\n\tfld %%st(1)\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0":"=qm"(positive):"m"(zero_bits):"memory","cc","st","st(1)","st(2)");
                if(positive){asm volatile("fsqrt":::"memory","st");time=spill_single();solved=true;}
                else asm volatile("fstp %%st(0)":::"memory","st");
                pending.reset();
            }
        }
        if(!solved){chosen_speed=minimum_speed;result.feasible=false;}
        const auto reciprocal=divide(scalar(read(memory,0x46a0d0u)),scalar(time));
        const auto vx=multiply(reciprocal,x),vy=multiply(reciprocal,y);
        const auto vx_bits=bits(vx),vy_bits=bits(vy);
        load(vy_bits);asm volatile("fmuls %0"::"m"(vy_bits):"memory","st");
        auto vz=multiply(reciprocal,z);
        load(vx_bits);asm volatile("fmuls %0"::"m"(vx_bits):"memory","st");
        pending=Pending::HorizontalVelocitySquares;
        auto gravity_time=multiply(scalar(time),gravity);
        gravity_time=multiply(gravity_time,scalar(read(memory,0x46a0ccu)));
        vz=add(vz,gravity_time);
        asm volatile("faddp %%st,%%st(1)":::"memory","st","st(1)");
        (void)multiply(scalar(time),scalar(chosen_speed));
        asm volatile("fsqrt":::"memory","st");(void)spill_single();pending.reset();
        result.direction={vx_bits,vy_bits,bits(vz)};return result;
    }catch(...) {
        if(pending)throw NativeCampaignAiJumpArcPendingErrorTu15(pending_quantities(*pending),std::current_exception());
        throw;
    }
}
// This adapter forwards only actual constant storage. It captures the one
// mathematical norm owed when that source read fails inside native30BD0;
// there is no private retained vector, CPU state or supplied result here.
class NormalizationConstants final:public SourcePhysicsContactMemory {
    const SourcePhysicsContactMemory &memory_;
public:
    explicit NormalizationConstants(const SourcePhysicsContactMemory &memory):memory_(memory){}
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        try{memory_.read(address,out);}catch(...) {
            if(address==0x46a0ecu)throw NativeCampaignAiJumpArcPendingErrorTu15(
                pending_quantities(Pending::LaunchMagnitude),std::current_exception());
            throw;
        }
    }
    void write(std::uint32_t,std::span<const std::byte>)override {
        throw std::logic_error("Native private arc normalization cannot publish through constant storage");
    }
};
#endif
}
NativeCampaignAiJumpArcValueTu15 native_campaign_ai_jump_arc_profile_tu15(
    const SourcePhysicsContactMemory &memory,const NativeCampaignAiJumpPointTu15 &first,
    const NativeCampaignAiJumpPointTu15 &second,std::uint32_t maximum_speed,NativeTu15PhysicsFrameFp &fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Prepared prepared;
    {ArithmeticEnvironment environment(fp);prepared=prepare(memory,first,second,maximum_speed);}
    if(prepared.straight_fallback) {
        const auto fallback=native_campaign_ai_jump_fallback_value_tu15(memory,first,second,maximum_speed,fp);
        return {true,fallback.direction};
    }
    NormalizationConstants constants(memory);
    auto magnitude=native_campaign_ai_normalize_vector_value_tu15(constants,prepared.direction,fp);
    bool zero_magnitude;
    {SimdEnvironment environment(fp);zero_magnitude=ordered_equal(scalar(magnitude),scalar(read(memory,0x46a0e8u)));}
    if(zero_magnitude) {
        prepared.feasible=false;prepared.direction=prepared.difference;
        magnitude=native_campaign_ai_normalize_vector_value_tu15(constants,prepared.direction,fp);
        SimdEnvironment environment(fp);
        if(ordered_equal(scalar(magnitude),scalar(read(memory,0x46a0e8u)))) {
            const auto fallback=read(memory,0x474d10u);
            prepared.direction[0]=read(memory,fallback);prepared.direction[1]=read(memory,fallback+4u);prepared.direction[2]=read(memory,fallback+8u);
        }
    }
    return {prepared.feasible,prepared.direction};
#else
    (void)memory;(void)first;(void)second;(void)maximum_speed;(void)fp;
    throw std::runtime_error("Exact native TU1.5 jump arc arithmetic unavailable");
#endif
}
NativeCampaignAiJumpArcValueTu15 native_campaign_ai_jump_arc_profile_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t first,std::uint32_t second,
    std::uint32_t maximum_speed,NativeTu15PhysicsFrameFp &fp) {
    NativeCampaignAiJumpPointMemoryTu15 first_point(memory,first),second_point(memory,second);
    return native_campaign_ai_jump_arc_profile_tu15(memory,first_point,second_point,maximum_speed,fp);
}

bool native_campaign_ai_jump_link_tu15(const SourcePhysicsContactMemory &memory,
    std::int16_t kind,const NativeCampaignAiJumpPointTu15 &from,
    const NativeCampaignAiJumpPointTu15 &to,const NativeCampaignAiJumpPointTu15 *secondary_from,
    const NativeCampaignAiJumpPointTu15 *secondary_to,NativeTu15PhysicsFrameFp &fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint32_t jump_speed{};bool refused{};
    {
        SimdEnvironment environment(fp);const auto zero=_mm_setzero_ps();
        const auto scenario=read(memory,0x4ecc1cu);
        const bool supported_kind=kind>=0&&kind<6;
        if(supported_kind&&scenario&&std::bit_cast<std::int32_t>(read(memory,scenario+0xc8u))>0) {
            const auto block=read(memory,scenario+0xccu);
            jump_speed=read(memory,block+std::uint32_t(kind)*4u+0x80u);
        }
        const auto target_height=scalar(to.coordinate(2));
        bool rises=greater(target_height,scalar(from.coordinate(2)));
        if(!rises&&secondary_to&&secondary_from) {
            const auto secondary_height=scalar(secondary_to->coordinate(2));
            rises=greater(secondary_height,scalar(secondary_from->coordinate(2)));
        }
        if(rises) {
            Scalar maximum_rise=zero;
            if(supported_kind&&scenario&&std::bit_cast<std::int32_t>(read(memory,scenario+0xc8u))>0) {
                const auto block=read(memory,scenario+0xccu);
                maximum_rise=scalar(read(memory,block+std::uint32_t(kind)*4u+0x98u));
            }
            auto rise=scalar(to.coordinate(2));rise=subtract(rise,scalar(from.coordinate(2)));
            if(secondary_to&&secondary_from) {
                auto alternative=scalar(secondary_to->coordinate(2));
                alternative=subtract(alternative,scalar(secondary_from->coordinate(2)));
                if(!greater(rise,alternative))rise=alternative;
            }
            if(!ordered_equal(maximum_rise,zero)&&!greater(maximum_rise,rise))refused=true;
        }
    }
    if(refused)return false;
    const auto primary=native_campaign_ai_jump_arc_profile_tu15(memory,from,to,jump_speed,fp);
    if(!primary.feasible||!secondary_from||!secondary_to)return primary.feasible;
    return native_campaign_ai_jump_arc_profile_tu15(memory,*secondary_from,*secondary_to,jump_speed,fp).feasible;
#else
    (void)memory;(void)kind;(void)from;(void)to;(void)secondary_from;(void)secondary_to;(void)fp;
    throw std::runtime_error("Exact native TU1.5 jump link arithmetic unavailable");
#endif
}

}
