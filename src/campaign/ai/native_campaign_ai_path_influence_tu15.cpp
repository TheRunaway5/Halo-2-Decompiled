#include "halo2/native_campaign_ai_path_influence_tu15.hpp"
#include "halo2/native_campaign_ai_frame_geometry_tu15.hpp"
#include <bit>
#include <cstring>
#include <optional>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
NativeCampaignAiPathInfluencePendingErrorTu15::NativeCampaignAiPathInfluencePendingErrorTu15(
    NativeCampaignAiPathInfluenceQuantityTu15 quantity,std::exception_ptr cause):
    std::runtime_error("Native path influence retained a pending mathematical quantity"),
    quantity_(quantity),cause_(std::move(cause)){}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));
    std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class ArithmeticEnvironment {
    alignas(16) std::array<std::byte,108> host_{};
    std::array<std::byte,28> native_{};
    NativeTu15PhysicsFrameFp &fp_;unsigned host_mxcsr_;
public:
    explicit ArithmeticEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),host_mxcsr_(_mm_getcsr()) {
        if((fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u) ||
           (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native path influence requires its masked actual FP policy");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),native_.size());const std::uint16_t empty=0xffffu;
        std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);
        std::memcpy(native_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~ArithmeticEnvironment() {
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(fp_.control_word),"=m"(fp_.status));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        _mm_setcsr(host_mxcsr_);
    }
};
using Scalar=__m128;
Scalar scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
Scalar sub(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar mul(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar divide(Scalar a,Scalar b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool above(Scalar a,Scalar b) {
    unsigned char value{};asm volatile("comiss %2,%1\n\tseta %0":"=qm"(value):"x"(a),"x"(b):"cc","memory");return value;
}
NativeCampaignAiPathInfluenceQuantityTu15 quantity(NativeCampaignAiPathInfluenceQuantityTu15::Kind kind) {
    alignas(16)std::array<std::byte,108> saved{};NativeCampaignAiPathInfluenceQuantityTu15 result{kind,{}};
    asm volatile("fnsave %0\n\tfrstor %0":"=m"(saved)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    asm volatile("fstpt %0":"=m"(result.extended)::"memory","st");
    asm volatile("frstor %0"::"m"(saved):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    return result;
}
template<class Sink>std::uint32_t calculate(const SourcePhysicsContactMemory &memory,
    std::uint32_t origin,std::uint32_t endpoint,std::uint32_t influence,
    NativeTu15PhysicsFrameFp &fp,unsigned occupied_x87,const Sink &publish_distance) {
    const auto first_frame=std::uint16_t(read(memory,origin+0xcu,2));
    auto distance=read(memory,0x45695cu);
    const auto end=native_campaign_ai_inverse_frame_point_value_tu15(memory,first_frame,endpoint,fp).point;
    const auto second_frame=std::uint16_t(read(memory,origin+0xcu,2));
    const auto center=native_campaign_ai_inverse_frame_point_value_tu15(memory,second_frame,influence+0x2cu,fp).point;
    ArithmeticEnvironment environment(fp);
    std::optional<NativeCampaignAiPathInfluenceQuantityTu15::Kind> pending;
    try {
        auto x=sub(scalar(end[0]),scalar(read(memory,origin)));
        auto z=sub(scalar(end[2]),scalar(read(memory,origin+8u)));
        auto y=sub(scalar(end[1]),scalar(read(memory,origin+4u)));
        const auto center_z=scalar(center[2]);
        auto length=add(mul(z,z),mul(x,x));length=add(length,mul(y,y));
        Scalar chosen_x,chosen_y,chosen_z;
        if(above(length,scalar(read(memory,0x46a0e8u)))) {
            auto toward_y=scalar(read(memory,origin+4u)),toward_z=scalar(read(memory,origin+8u));
            auto toward_x=sub(scalar(read(memory,origin)),scalar(center[0]));
            toward_y=sub(toward_y,scalar(center[1]));toward_z=sub(toward_z,center_z);
            auto fraction=add(mul(toward_z,z),mul(toward_y,y));fraction=add(fraction,mul(toward_x,x));
            fraction=divide(fraction,length);
            if(above(scalar(0),fraction) || above(fraction,scalar(read(memory,0x46a0d0u)))) {
                chosen_x=scalar(end[0]);chosen_y=scalar(end[1]);chosen_z=scalar(end[2]);
            }else {
                x=mul(x,fraction);chosen_x=add(x,scalar(read(memory,origin)));
                y=mul(y,fraction);chosen_y=add(y,scalar(read(memory,origin+4u)));
                z=mul(z,fraction);chosen_z=add(z,scalar(read(memory,origin+8u)));
            }
        }else {
            chosen_x=scalar(read(memory,origin));chosen_y=scalar(read(memory,origin+4u));chosen_z=scalar(read(memory,origin+8u));
        }
        y=sub(chosen_y,scalar(center[1]));x=sub(chosen_x,scalar(center[0]));z=sub(chosen_z,center_z);
        const auto y_squared=mul(y,y),z_squared=mul(z,z),x_squared=mul(x,x);
        const auto radius=scalar(read(memory,influence+0x3cu));
        // The retained radius read precedes the original squared-distance
        // additions, including their sticky FP effects on a rejected read.
        auto squared=add(z_squared,y_squared);squared=add(squared,x_squared);
        std::uint32_t response=0;
        if(above(mul(radius,radius),squared)) {
            if(occupied_x87>=8)throw std::runtime_error("Native path influence requires its actual free extended-real temporary");
            const auto squared_bits=bits(squared);
            asm volatile("flds %1\n\tfsqrt\n\tfsts %0":"=m"(distance):"m"(squared_bits):"memory","st");
            pending=NativeCampaignAiPathInfluenceQuantityTu15::Kind::DistanceRoot;
            const auto current_radius=read(memory,influence+0x3cu);
            asm volatile("fdivs %0"::"m"(current_radius):"memory","st");
            pending=NativeCampaignAiPathInfluenceQuantityTu15::Kind::DistanceRatio;
            const auto one=read(memory,0x46a0d0u);
            asm volatile("fsubrs %0"::"m"(one):"memory","st");
            pending=NativeCampaignAiPathInfluenceQuantityTu15::Kind::NormalizedInfluence;
            const auto weight=read(memory,influence+0x40u);
            asm volatile("fmuls %1\n\tfstps %0":"=m"(response):"m"(weight):"memory","st");
            pending.reset();
        }
        publish_distance(distance);
        return response;
    }catch(...) {
        if(pending)throw NativeCampaignAiPathInfluencePendingErrorTu15(quantity(*pending),std::current_exception());
        throw;
    }
}
#endif
}
std::uint32_t native_campaign_ai_path_influence_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t origin,std::uint32_t endpoint,std::uint32_t influence,
    std::uint32_t output,NativeTu15PhysicsFrameFp &fp,unsigned occupied_x87,
    const SourcePhysicsContactRecordObserver &observe) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return calculate(memory,origin,endpoint,influence,fp,occupied_x87,[&](std::uint32_t value){
        std::array<std::byte,4> bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));
        SourcePhysicsContactRecordStore event{0x27c6a2u,output,{},std::vector<std::byte>(bytes.begin(),bytes.end())};
        if(observe){event.before.resize(4);memory.read(output,event.before);}
        memory.write(output,bytes);if(observe)observe(event);
    });
#else
#error Native path influence requires its exact x87 and SIMD backend
#endif
}
NativeCampaignAiPathInfluenceValueTu15 native_campaign_ai_path_influence_value_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t origin,std::uint32_t endpoint,
    std::uint32_t influence,NativeTu15PhysicsFrameFp &fp,unsigned occupied_x87) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    NativeCampaignAiPathInfluenceValueTu15 result{};
    result.influence=calculate(memory,origin,endpoint,influence,fp,occupied_x87,[&](std::uint32_t value){result.distance=value;});
    return result;
#endif
}
}
