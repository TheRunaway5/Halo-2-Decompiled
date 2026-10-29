#include "halo2/native_campaign_ai_perception_score_tu15.hpp"
#include "halo2/native_campaign_ai_perception_policy_tu15.hpp"
#include "halo2/native_campaign_ai_perception_refs_tu15.hpp"
#include "halo2/native_campaign_ai_character_tu15.hpp"
#include "halo2/native_tu15_data_array_iteration.hpp"
#include <array>
#include <bit>
#include <cstring>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint64_t read64(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n){
    std::array<std::byte,8>b{};m.read(p,std::span(b).first(n));std::uint64_t v{};
    for(unsigned k=0;k<n;++k)v|=std::uint64_t(std::to_integer<unsigned>(b[k]))<<(k*8u);return v;
}
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){return std::uint32_t(read64(m,p,n));}
std::int16_t signed16(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
std::int8_t signed8(std::uint32_t v){return std::bit_cast<std::int8_t>(std::uint8_t(v));}
std::uint32_t row(std::uint32_t rows,std::uint32_t datum,std::uint32_t stride){return rows+(datum&0xffffu)*stride;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class SimdEnvironment {
    NativeTu15PhysicsFrameFp&fp_;unsigned ambient_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp&fp):fp_(fp),ambient_(_mm_getcsr()){
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native perception score requires its original SIMD trap owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(ambient_);}
};
class ConfidenceEnvironment {
    alignas(16)std::array<std::byte,108>ambient_{};std::array<std::byte,28>source_{};NativeTu15PhysicsFrameFp&fp_;
public:
    explicit ConfidenceEnvironment(NativeTu15PhysicsFrameFp&fp):fp_(fp){
        if((fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u))throw std::runtime_error("Native perception confidence requires its original FP trap owner");
        asm volatile("fnsave %0":"=m"(ambient_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(source_.data(),ambient_.data(),source_.size());const std::uint16_t empty=0xffffu;
        std::memcpy(source_.data(),&fp.control_word,2);std::memcpy(source_.data()+4,&fp.status,2);std::memcpy(source_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(source_):"memory");
    }
    ~ConfidenceEnvironment(){
        asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));
        asm volatile("frstor %0"::"m"(ambient_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    }
};
__m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
std::uint32_t bits(__m128 value){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 divide(__m128 a,__m128 b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 integer(std::int32_t value){return _mm_cvtsi32_ss(_mm_setzero_ps(),value);}
bool above(__m128 a,__m128 b){unsigned char result;asm volatile("comiss %2,%1; seta %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result!=0;}
bool above_equal(__m128 a,__m128 b){unsigned char result;asm volatile("comiss %2,%1; setae %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result!=0;}
std::pair<std::uint32_t,bool> confidence_product(std::uint32_t value,std::uint64_t factor,std::uint32_t lower,NativeTu15PhysicsFrameFp&fp){
    ConfidenceEnvironment environment(fp);const auto input=std::bit_cast<float>(value),bound=std::bit_cast<float>(lower);const auto scale=std::bit_cast<double>(factor);float output;unsigned char below;
    // Balanced native extended product and comparison. The lower comparison
    // uses the extended product; the published local is rounded to float.
    asm volatile("flds %2; fmull %3; fsts %0; flds %4; fcomip %%st(1),%%st; fstp %%st(0); seta %1"
        :"=m"(output),"=qm"(below):"m"(input),"m"(scale),"m"(bound):"memory","cc","st","st(1)");
    return {std::bit_cast<std::uint32_t>(output),below!=0};
}
#endif
}
std::uint32_t native_campaign_ai_perception_score_tu15(SourcePhysicsContactMemory&m,
        std::uint32_t actor,std::uint32_t perception,NativeTu15PhysicsFrameFp&fp,unsigned occupied_x87){
    const auto properties=read(m,0x50eef0u),property_rows=read(m,properties+0x44u),actors=read(m,0x5020c8u),actor_rows=read(m,actors+0x44u);
    const auto property=row(property_rows,perception,0x3cu),group=read(m,property+8u),groups=read(m,0x50eef4u),group_rows=read(m,groups+0x44u);
    const auto record=row(actor_rows,actor,0x888u),group_record=row(group_rows,group,0xc4u);
    const auto position=native_campaign_ai_perception_position_tu15(m,property),query=read(m,property+0x14u);
    std::uint32_t extension=0;
    if(query!=none){const auto queries=read(m,0x50eeecu),query_rows=read(m,queries+0x44u),query_record=row(query_rows,query,0x124u);if(query_record)extension=query_record+0x70u;}
    if(read(m,group_record+0x26u,1)||!read(m,group_record+0x23u,1)||signed16(read(m,property+0x24u,2))<=0)return 0;
    const auto stale=read(m,position+0x5eu,1),one_bits=read(m,0x46a100u);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    SimdEnvironment environment(fp);const auto one=scalar(one_bits);
    if(stale){const auto ticks=signed16(read(m,position+0x5cu,2));const auto clock=read(m,0x51df54u);const auto seconds=mul(integer(ticks),scalar(read(m,clock+4u)));if(above_equal(seconds,one))return 0;}
    const auto object=read(m,property+0x20u),objects=read(m,0x4ecbd0u),object_rows=read(m,objects+0x44u),body=read(m,row(object_rows,object,12u)+8u);
    if(read(m,body+0xaau,1)==15u)return 0;
    const auto kind=read(m,property+0x18u,2);if(kind!=1u&&kind!=6u)return 0;
    const auto inactive=read(m,record+7u,1);const auto unit=scalar(read(m,0x46a0d0u));
    std::int16_t range_rank=0,state_rank=0,recognition=0;
    auto attention=scalar(0),confidence=scalar(0),alignment=scalar(0);
    if(!inactive&&!native_campaign_ai_perception_has_target_tu15(m,perception)){
        if(read(m,record+0x269u,1)){
            const auto watched=read(m,record+0x26cu),tracked=read(m,position+0x3cu);range_rank=watched==tracked?10:0;
        }else if(!native_campaign_ai_actor_vehicle_policy_tu15(m,actor)){
            auto range=scalar(0);const auto policy=native_campaign_ai_character_tracking_policy_tu15(m,actor);if(policy)range=scalar(read(m,policy+4u));
            const bool close=above(unit,scalar(read(m,property+0x28u)));
            bool remembered=false;if(close&&read(m,property+0x24u,2)==3u&&extension)remembered=read(m,extension+0x88u,1)!=0;
            if(close&&!remembered)range_rank=10;
            else if(read(m,position+0x3cu)==none){
                const auto target_kind=read(m,position+0x69u,1),actor_kind=read(m,record+0x265u,1);
                if(target_kind!=actor_kind||above(range,scalar(read(m,property+0x28u))))range_rank=10;
            }
        }else{
            const auto policy=native_campaign_ai_actor_vehicle_perception_tu15(m,actor);
            const bool close=above(unit,scalar(read(m,property+0x28u)));
            bool remembered=false;if(close&&read(m,property+0x24u,2)==3u&&extension)remembered=read(m,extension+0x88u,1)!=0;
            if(close&&!remembered)range_rank=7;
            else if(policy){
                auto range=scalar(read(m,policy+0x18u));const auto authored=native_campaign_ai_actor_order_character_tu15(m,actor);
                if(authored){const auto variant=native_campaign_ai_order_perception_variant_tu15(m,authored);if(variant==1)range=scalar(read(m,policy+0x28u));else if(variant==2)range=scalar(read(m,policy+0x30u));}
                if(above(range,scalar(read(m,property+0x28u))))range_rank=5;
                else{const auto near=scalar(read(m,policy+0xcu)),distance=scalar(read(m,property+0x28u));if(above(near,distance))range_rank=3;}
            }
        }
    }
    if(read(m,position+0x5eu,1))state_rank=1;
    else{
        bool tracking=false;
        if(extension&&!read(m,record+7u,1))tracking=read(m,extension+0x2au,1)!=0;
        if(tracking)state_rank=6;
        else{
            const auto state=signed16(read(m,property+0x24u,2));
            if(state>=1){
                if(read(m,record+7u,1))state_rank=2;
                else{
                    bool recently_seen=false;if(extension){const auto mode=read(m,extension+6u,2);recently_seen=mode==0u||mode==1u;}
                    if(recently_seen){
                        const auto visible=read(m,position+0x63u,1);state_rank=visible&&signed8(read(m,extension+0x39u,1))<=1?5:4;
                    }else if(state>=3)state_rank=!extension||read(m,extension+0x88u,1)?1:2;
                    else state_rank=3;
                }
            }
        }
    }
    if(perception==read(m,record+0x338u)&&signed16(read(m,record+0x86u,2))>=5&&!read(m,position+0x5eu,1)){
        const auto engaged=read(m,record+0x354u,1);attention=scalar(read(m,0x46a16cu));if(!engaged)attention=scalar(read(m,0x46a11cu));
    }
    if(read(m,position+0x67u,1))recognition=10;
    if(extension){
        const auto value=read(m,extension+0x60u);if(occupied_x87>6u)throw std::runtime_error("Native perception confidence requires its original x87 stack-fault owner");
        const auto factor=read64(m,0x46a6d0u,8);const auto lower=read(m,0x46a0e8u);
        const auto product=confidence_product(value,factor,lower,fp);
        if(!product.second){confidence=scalar(product.first);const auto upper=scalar(read(m,0x46a0e4u));if(above(confidence,upper))confidence=upper;}
    }
    const auto distance=scalar(read(m,property+0x28u)),distance_scale=scalar(read(m,0x46a178u));auto denominator=mul(distance,distance_scale);
    const auto watched=read(m,record+0x26cu);denominator=add(denominator,unit);const auto proximity=divide(one,denominator);
    if(watched!=none){
        const auto team=read(m,record+0x24u,2);
        if(team!=0xffffu){
            const auto game=read(m,0x4f3218u),mode=read(m,game+8u);bool admitted=true,hostile=false;
            if(mode==1u){
                const auto signed_team=signed16(team);if(signed_team<0||signed_team>=16)admitted=false;
                else{const auto ai=read(m,0x5020c4u);hostile=!native_tu15_bit_array_test(m,ai+0xc4u,std::int32_t(signed_team)*16+1);}
            }else if(mode==2u)throw NativeCampaignAiPerceptionScoreRequiredOwnerTu15(0x26fe94u,0x160a60u);
            else hostile=team!=1u;
            if(admitted&&!hostile){
                const auto child=native_campaign_ai_child_player_tu15(m,read(m,record+0x26cu));
                if(child.unit!=none){
                    const auto current_objects=read(m,0x4ecbd0u),rows=read(m,current_objects+0x44u),unit_body=read(m,row(rows,child.unit,12u)+8u);
                    __m128 x,y,z;
                    if(extension){const auto xb=read(m,extension+0x2cu),yb=read(m,extension+0x30u),zb=read(m,extension+0x34u);x=scalar(xb);y=scalar(yb);z=scalar(zb);}
                    else{
                        x=scalar(read(m,position+4u));y=scalar(read(m,position+8u));z=scalar(read(m,position+0xcu));
                        x=sub(x,scalar(read(m,record+0x238u)));y=sub(y,scalar(read(m,record+0x23cu)));z=sub(z,scalar(read(m,record+0x240u)));
                    }
                    auto vz=scalar(read(m,unit_body+0x158u)),vy=scalar(read(m,unit_body+0x154u));vy=mul(vy,y);vz=mul(vz,z);auto dot=add(vz,vy);
                    auto vx=mul(scalar(read(m,unit_body+0x150u)),x);dot=add(dot,vx);const auto scale=scalar(read(m,0x46a10cu));dot=mul(dot,scale);
                    if(!above(scalar(0),dot)){
                        vz=mul(scalar(read(m,unit_body+0x158u)),z);vy=mul(scalar(read(m,unit_body+0x154u)),y);vx=scalar(read(m,unit_body+0x150u));
                        auto total=add(vz,vy);vx=mul(vx,x);total=add(total,vx);alignment=mul(total,scale);
                    }
                }
            }
        }
    }
    auto score=add(integer(std::int32_t(range_rank)+std::int32_t(state_rank)),integer(recognition));
    score=add(score,alignment);score=add(score,proximity);score=add(score,confidence);score=add(score,attention);return bits(score);
#else
    (void)fp;(void)occupied_x87;(void)stale;(void)one_bits;throw std::runtime_error("Exact native perception score arithmetic backend unavailable");
#endif
}
}
