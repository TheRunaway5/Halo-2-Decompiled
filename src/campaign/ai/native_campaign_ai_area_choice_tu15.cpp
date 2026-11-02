#include "halo2/native_campaign_ai_area_choice_tu15.hpp"
#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
#include "native_campaign_authored_area_iterator_tu15.hpp"
#include <bit>
#include <cstring>
#include <optional>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
NativeCampaignAiAreaScorePendingErrorTu15::NativeCampaignAiAreaScorePendingErrorTu15(
    NativeCampaignAiAreaScorePendingTu15 pending,std::exception_ptr cause):
    std::runtime_error("Authored area score retained pending mathematical quantities"),
    pending_(pending),cause_(std::move(cause)){}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> data{};memory.read(address,std::span(data).first(width));
    std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(data[n])<<(n*8u);
    return value;
}
std::uint64_t read_double(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,8> data{};memory.read(address,data);
    std::uint64_t value{};for(unsigned n=0;n<8;++n)value|=std::uint64_t(std::to_integer<unsigned>(data[n]))<<(n*8u);
    return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=4) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class ArithmeticEnvironment {
    alignas(16)std::array<std::byte,108> host_{};
    NativeTu15PhysicsFrameFp &fp_;unsigned mxcsr_;
public:
    explicit ArithmeticEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),mxcsr_(_mm_getcsr()) {
        if((fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u) ||
           (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Authored area scoring requires its masked actual FP policy");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::array<std::byte,28> native{};std::memcpy(native.data(),host_.data(),native.size());
        const std::uint16_t empty=0xffffu;
        std::memcpy(native.data(),&fp.control_word,2);std::memcpy(native.data()+4,&fp.status,2);
        std::memcpy(native.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native):"memory");
        _mm_setcsr(fp.mxcsr);
    }
    ~ArithmeticEnvironment(){
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(fp_.control_word),"=m"(fp_.status));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        _mm_setcsr(mxcsr_);
    }
};
using Scalar=__m128;
Scalar scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
Scalar subtract(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar multiply(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar divide(Scalar a,Scalar b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
NativeCampaignAiAreaScorePendingTu15 pending_quantity(NativeCampaignAiAreaScorePendingTu15::Stage stage,unsigned count) {
    alignas(16)std::array<std::byte,108> saved{};
    NativeCampaignAiAreaScorePendingTu15 value{stage,count,{}};
    asm volatile("fnsave %0\n\tfrstor %0":"=m"(saved)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    for(unsigned n=0;n<count;++n)asm volatile("fstpt %0":"=m"(value.quantities[n])::"memory","st");
    asm volatile("frstor %0"::"m"(saved):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    return value;
}
std::optional<std::uint32_t> better_score(const SourcePhysicsContactMemory &memory,
    std::uint32_t actor,const std::array<std::uint32_t,3> &point,std::uint32_t best,
    NativeTu15PhysicsFrameFp &fp,unsigned occupied) {
    ArithmeticEnvironment environment(fp);
    using Stage=NativeCampaignAiAreaScorePendingTu15::Stage;
    std::optional<Stage> pending;unsigned quantity_count{};
    try {
        auto z=bits(subtract(scalar(point[2]),scalar(read(memory,actor+0x240u))));
        auto y=bits(subtract(scalar(point[1]),scalar(read(memory,actor+0x23cu))));
        auto x=bits(subtract(scalar(point[0]),scalar(read(memory,actor+0x238u))));
        if(occupied>6u)throw std::invalid_argument("Authored area scoring requires two free x87 temporaries");
        std::uint32_t magnitude;
        asm volatile("flds %1\n\tfmuls %1\n\tflds %2\n\tfmuls %2\n\tfaddp %%st,%%st(1)\n\tflds %3\n\tfmuls %3\n\tfaddp %%st,%%st(1)\n\tfsqrt\n\tfsts %0\n\tfabs"
            :"=m"(magnitude):"m"(z),"m"(y),"m"(x):"memory","st","st(1)");
        pending=Stage::Magnitude;quantity_count=1;
        const auto epsilon=read(memory,0x46a0ecu);unsigned char small{};
        asm volatile("flds %1\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0":"=qm"(small):"m"(epsilon):"memory","cc","st","st(1)");
        pending.reset();
        if(small)magnitude=0;
        else {
            const auto reciprocal=divide(scalar(read(memory,0x46a0d0u)),scalar(magnitude));
            x=bits(multiply(reciprocal,scalar(x)));
            y=bits(multiply(scalar(y),reciprocal));z=bits(multiply(scalar(z),reciprocal));
        }
        asm volatile("flds %0"::"m"(z):"memory","st");pending=Stage::DirectionZ;quantity_count=1;
        auto facing=read(memory,actor+0x298u);
        asm volatile("fmuls %0\n\tflds %1"::"m"(facing),"m"(y):"memory","st","st(1)");
        pending=Stage::DirectionYAndZScore;quantity_count=2;facing=read(memory,actor+0x294u);
        asm volatile("fmuls %0\n\tfaddp %%st,%%st(1)\n\tflds %1"::"m"(facing),"m"(x):"memory","st","st(1)");
        pending=Stage::DirectionXAndYZScore;facing=read(memory,actor+0x290u);
        asm volatile("fmuls %0\n\tfaddp %%st,%%st(1)\n\tflds %1"::"m"(facing),"m"(magnitude):"memory","st","st(1)");
        pending=Stage::DistanceAndFacingScore;const auto penalty=read_double(memory,0x46a328u);
        std::uint32_t score;
        asm volatile("fmull %1\n\tfsubrp %%st,%%st(1)\n\tfstps %0":"=m"(score):"m"(penalty):"memory","st","st(1)");
        pending.reset();unsigned char better{};
        asm volatile("flds %1\n\tflds %2\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0"
            :"=qm"(better):"m"(best),"m"(score):"memory","cc","st","st(1)");
        if(better)return score;
        return {};
    }catch(...){if(pending)throw NativeCampaignAiAreaScorePendingErrorTu15(pending_quantity(*pending,quantity_count),std::current_exception());throw;}
}
#endif
}
bool native_campaign_ai_choose_area_tu15(SourcePhysicsContactMemory &memory,
    const NativeCampaignAiAreaChoiceInputsTu15 &inputs,NativeTu15PhysicsFrameFp &fp,unsigned occupied,
    const SourcePhysicsContactRecordObserver &observe) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto pool=read(memory,0x5020c8u);const auto actor=inputs.actor();
    auto best=read(memory,0x44aac8u);
    const auto actor_offset=(actor&0xffffu)*0x888u,row=read(memory,pool+0x44u)+actor_offset;
    const auto mode=read(memory,row+0x26cu)!=0xffffffffu;
    const auto squad=std::uint16_t(read(memory,row+0x30u,2));
    detail::CampaignAuthoredAreaIteratorTu15 areas(memory,squad,mode);
    unsigned count{};std::uint16_t best_ordinal=0xffffu,best_zone=0xffffu,best_area=0xffffu;
    unsigned old_ordinal=0xffffffffu;
    for(auto area=areas.next();area!=0xffffu;area=areas.next()) {
        if(count>=32u)break;
        const auto mask=1u<<count;
        const auto selection=inputs.selection();
        if(!(read(memory,selection)&mask)) {
            const auto zone=areas.zone();const auto scenario=read(memory,0x4ecc20u);
            const auto zones=read(memory,scenario+0x16cu),entries=read(memory,zones+std::uint32_t(zone)*0x38u+0x34u);
            const auto point=entries+std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(area)))*0x88u+0x24u;
            const auto position=native_campaign_ai_project_point_value_tu15(memory,point,fp);
            if(const auto score=better_score(memory,row,position,best,fp,occupied)) {
                best=*score;best_ordinal=std::uint16_t(count);best_zone=zone;best_area=area;
            }
        }else if(area==std::uint16_t(read(memory,selection+6u,2)) &&
                 areas.zone()==std::uint16_t(read(memory,selection+4u,2)))old_ordinal=count;
        ++count;
    }
    if(count<2u)return false;
    if(best_ordinal!=0xffffu) {
        const auto selection=inputs.selection();
        publish(memory,observe,0x1bbe36u,selection+4u,best_zone,2);
        publish(memory,observe,0x1bbe3fu,selection+6u,best_area,2);
        const auto mask=read(memory,selection);
        publish(memory,observe,0x1bbe51u,selection,mask|(1u<<best_ordinal));
    }else {
        const auto selection=inputs.selection();
        const auto mask=1u<<(old_ordinal&31u),current_pool=read(memory,0x5020c8u);
        publish(memory,observe,0x1bbe71u,selection,mask);
        const auto current_row=read(memory,current_pool+0x44u)+actor_offset;
        publish(memory,observe,0x1bbe78u,current_row+0x3feu,3,2);
        for(unsigned n=0;n<4;++n) {
            const auto absent=read(memory,0x47d6c0u);
            publish(memory,observe,0x1bbe96u,current_row+0x402u+n*6u,absent);
        }
        (void)native_campaign_ai_choose_area_tu15(memory,inputs.actor(),selection,fp,occupied,observe);
    }
    return true;
#else
    (void)memory;(void)inputs;(void)fp;(void)occupied;(void)observe;
    throw std::runtime_error("Exact authored area-choice backend unavailable");
#endif
}
bool native_campaign_ai_choose_area_tu15(SourcePhysicsContactMemory &memory,std::uint32_t actor,
    std::uint32_t selection,NativeTu15PhysicsFrameFp &fp,unsigned occupied,
    const SourcePhysicsContactRecordObserver &observe) {
    struct Values final:NativeCampaignAiAreaChoiceInputsTu15 {
        std::uint32_t datum,record;
        Values(std::uint32_t a,std::uint32_t s):datum(a),record(s){}
        std::uint32_t actor()const override{return datum;}
        std::uint32_t selection()const override{return record;}
    } values(actor,selection);
    return native_campaign_ai_choose_area_tu15(memory,values,fp,occupied,observe);
}
std::uint32_t native_campaign_ai_prepare_area_choice_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t request,const SourcePhysicsContactRecordObserver &observe) {
    const auto selection=request+0xcu;
    publish(memory,observe,0x1bbec7u,selection,0);
    return selection;
}
bool native_campaign_ai_begin_area_choice_tu15(SourcePhysicsContactMemory &memory,std::uint32_t actor,
    std::uint32_t request,NativeTu15PhysicsFrameFp &fp,unsigned occupied,
    const SourcePhysicsContactRecordObserver &observe) {
    const auto selection=native_campaign_ai_prepare_area_choice_tu15(memory,request,observe);
    return native_campaign_ai_choose_area_tu15(memory,actor,selection,fp,occupied,observe);
}
}
