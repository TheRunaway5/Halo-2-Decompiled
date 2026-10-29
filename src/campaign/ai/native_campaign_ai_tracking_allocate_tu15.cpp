#include "halo2/native_campaign_ai_tracking_allocate_tu15.hpp"
#include "halo2/native_campaign_ai_combat_reset_tu15.hpp"
#include "halo2/native_campaign_ai_perception_refs_tu15.hpp"
#include "halo2/native_tu15_data_array_allocate.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};
    for(unsigned k=0;k<n;++k)v|=std::to_integer<unsigned>(b[k])<<(k*8u);return v;
}
void write(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t v,unsigned n=4){
    std::array<std::byte,4>b{};for(unsigned k=0;k<n;++k)b[k]=std::byte(v>>(k*8u));
    m.write(p,std::span(b).first(n));
}
std::int16_t signed16(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
std::int32_t signed32(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
std::uint32_t row(std::uint32_t rows,std::uint32_t datum,std::uint32_t stride){return rows+(datum&0xffffu)*stride;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class TickEnvironment {
    alignas(16)std::array<std::byte,108> ambient_{};
    std::array<std::byte,28> source_{};
    unsigned ambient_mxcsr_;NativeTu15PhysicsFrameFp&fp_;
public:
    explicit TickEnvironment(NativeTu15PhysicsFrameFp&fp):ambient_mxcsr_(_mm_getcsr()),fp_(fp){
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u)||
           (fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u))
            throw std::runtime_error("Native tracking pressure requires its original FP trap owner");
        asm volatile("fnsave %0":"=m"(ambient_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(source_.data(),ambient_.data(),source_.size());const std::uint16_t empty=0xffffu;
        std::memcpy(source_.data(),&fp.control_word,2);std::memcpy(source_.data()+4,&fp.status,2);std::memcpy(source_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(source_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~TickEnvironment(){
        asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(ambient_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(ambient_mxcsr_);
    }
};
std::int32_t age_threshold(std::int16_t tick_rate,std::uint32_t scale,
        NativeTu15PhysicsFrameFp&fp,unsigned occupied_x87){
    TickEnvironment environment(fp);
    __m128 value=_mm_cvtsi32_ss(_mm_setzero_ps(),std::int32_t(tick_rate));
    const auto factor=_mm_castsi128_ps(_mm_cvtsi32_si128(signed32(scale)));
    asm volatile("mulss %1,%0":"+x"(value):"x"(factor):"memory");
    const float seconds=_mm_cvtss_f32(value);std::int32_t result;
    if(occupied_x87>7u)throw std::runtime_error("Native tracking pressure requires its original x87 stack-fault owner");
    // One balanced hardware conversion under the explicit source policy.
    asm volatile("flds %1; fistpl %0":"=m"(result):"m"(seconds):"memory","st");return result;
}
#endif
NativeCampaignAiActorIteratorTu15 iterator(SourcePhysicsContactMemory&m){
    NativeCampaignAiActorIteratorTu15 state{{0,0,none,none},false,none};
    const auto ai=read(m,0x5020a8u);
    if(read(m,ai+1u,1))state.storage.pool=read(m,0x5020c8u);
    return state;
}
}
std::uint32_t native_campaign_ai_actor_next_tu15(SourcePhysicsContactMemory&m,
        NativeCampaignAiActorIteratorTu15&state){
    const auto ai=read(m,0x5020a8u);if(!read(m,ai+1u,1))return 0;
    for(;;){
        native_tu15_data_array_advance(m,state.storage);
        if(!state.storage.current){state.previous=state.storage.datum;return 0;}
        if(state.active_only&&!read(m,state.storage.current+9u,1))continue;
        state.previous=state.storage.datum;return state.storage.current;
    }
}
std::uint16_t native_campaign_ai_actor_tracking_retire_tu15(SourcePhysicsContactMemory&m,std::uint32_t actor){
    const auto actors=read(m,0x5020c8u),rows=read(m,actors+0x44u);
    auto property=read(m,row(rows,actor,0x888u)+0x58u);std::uint16_t count=0;
    while(property!=none){
        const auto properties=read(m,0x50eef0u),property_rows=read(m,properties+0x44u);
        const auto member=row(property_rows,property,0x3cu),query=read(m,member+0x14u),current=property;
        property=read(m,member+0x2cu);if(query==none)continue;
        if(read(m,member+0x14u)!=none)native_campaign_ai_tracking_retire_tu15(m,actor,current);
        ++count;write(m,member+0x24u,0,2);write(m,member+0x10u,0);
    }
    return count;
}
void native_campaign_ai_tracking_extension_reset_tu15(SourcePhysicsContactMemory&m,std::uint32_t extension){
    const auto defaults=read(m,0x474d04u);
    write(m,extension+6u,4,2);write(m,extension+0x8cu,4,2);
    write(m,extension+0x10u,none);write(m,extension+0x14u,none);write(m,extension+0x50u,none);
    write(m,extension+0x8au,0,2);
    for(auto offset:{0x54u,0x58u,0x5cu,0x60u})write(m,extension+offset,0);
    write(m,extension+0xa2u,0,1);
    for(unsigned n=0;n<3;++n)write(m,extension+0x94u+n*4u,read(m,defaults+n*4u));
    write(m,extension+0xb0u,none);write(m,extension+0x64u,0,1);write(m,extension+0x66u,0,2);
    for(auto offset:{0x68u,0x69u,0x4cu})write(m,extension+offset,0,1);
    write(m,extension+0x70u,0,2);write(m,extension+0x6cu,1,1);write(m,extension+0x6du,1,1);
    write(m,extension+0x88u,0,1);write(m,extension+0x90u,0,2);write(m,extension+0x8au,0,2);
}
bool native_campaign_ai_tracking_pressure_tu15(SourcePhysicsContactMemory&m,NativeTu15PhysicsFrameFp&fp,unsigned occupied_x87){
    const auto ai=read(m,0x5020a8u),last=read(m,ai+0x370u),clock=read(m,0x51df54u),now=read(m,clock+8u);
    if(signed32(last)>=signed32(now))return false;
    // The first iterator initialization reuses the captured AI record; later
    // passes reload it. Each advance independently reads the real enable bit.
    NativeCampaignAiActorIteratorTu15 state{{0,0,none,none},false,none};
    if(read(m,ai+1u,1))state.storage.pool=read(m,0x5020c8u);
    auto actor=native_campaign_ai_actor_next_tu15(m,state);
    const auto scale=read(m,0x46a108u);std::uint16_t count=0;
    for(unsigned pass=0;pass<3;++pass){
        if(pass){state=iterator(m);actor=native_campaign_ai_actor_next_tu15(m,state);}
        while(actor){
            if(!read(m,actor+9u,1)){
                bool retire=pass==2;
                if(!retire){
                    const auto category=read(m,0x474c24u,2);
                    const bool equal=read(m,actor+0x3eu,2)==category;
                    retire=pass==1&&!equal;
                    if((pass==0&&!equal)||(pass==1&&equal)){
                        const auto rate=signed16(read(m,clock+2u,2));
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
                        const auto threshold=age_threshold(rate,scale,fp,occupied_x87);
#else
                        (void)rate;(void)scale;(void)fp;(void)occupied_x87;throw std::runtime_error("Exact native tracking pressure arithmetic backend unavailable");
                        const std::int32_t threshold=0;
#endif
                        const auto updated=read(m,actor+0x10u);retire=signed32(now-updated)>threshold;
                    }
                }
                if(retire)count=std::uint16_t(count+native_campaign_ai_actor_tracking_retire_tu15(m,state.previous));
            }
            actor=native_campaign_ai_actor_next_tu15(m,state);
        }
        if(signed16(count)>=5)break;
    }
    return signed16(count)>0;
}
std::uint32_t native_campaign_ai_tracking_allocate_tu15(SourcePhysicsContactMemory&m,
        std::uint32_t actor,std::uint32_t perception,std::int16_t priority,NativeTu15PhysicsFrameFp&fp,unsigned occupied_x87){
    const auto actors=read(m,0x5020c8u),actor_rows=read(m,actors+0x44u),record=row(actor_rows,actor,0x888u);
    std::int16_t chosen=-1,best_priority=-1;std::int32_t best_time=-1;std::uint32_t properties=0;
    for(unsigned slot=0;slot<8;++slot){
        const auto cached=read(m,record+0x5cu+slot*4u);properties=read(m,0x50eef0u);
        if(cached==none){chosen=std::int16_t(slot);break;}
        const auto extent=read(m,properties+0x38u),index=cached&0xffffu;
        bool valid=signed32(index)<signed32(extent);std::uint32_t member=0;
        if(valid){
            const auto stride=read(m,properties+0x24u),rows=read(m,properties+0x44u);member=rows+index*stride;
            const auto salt=signed16(read(m,member,2));valid=salt!=0&&std::int32_t(salt)==(signed32(cached)>>16);
        }
        if(!valid){write(m,record+0x5cu+slot*4u,none);chosen=std::int16_t(slot);break;}
        const auto candidate=signed16(read(m,member+0x1au,2));
        if(candidate<priority){
            bool better=chosen==-1||candidate<best_priority;
            if(!better&&candidate==best_priority){
                const auto position=native_campaign_ai_perception_position_tu15(m,member);better=signed32(read(m,position))<best_time;
            }
            if(better){
                chosen=std::int16_t(slot);best_priority=candidate;
                const auto position=native_campaign_ai_perception_position_tu15(m,member);best_time=signed32(read(m,position));
            }
        }
        if(slot==7)properties=read(m,0x50eef0u);
    }
    if(chosen==-1)return none;
    const auto cache=record+0x5cu+std::uint32_t(chosen)*4u,cached=read(m,cache);
    if(cached!=none){
        const auto rows=read(m,properties+0x44u),member=row(rows,cached,0x3cu);
        if(read(m,member+0x14u)!=none)native_campaign_ai_tracking_retire_tu15(m,read(m,member+4u),cached);
        write(m,member+0x24u,0,2);write(m,member+0x10u,0);
    }
    const auto queries=read(m,0x50eeecu);auto allocated=native_tu15_data_array_allocate(m,queries);
    if(allocated==none&&native_campaign_ai_tracking_pressure_tu15(m,fp,occupied_x87))allocated=native_tu15_data_array_allocate(m,queries);
    if(allocated==none){write(m,cache,none);return none;}
    const auto current_properties=read(m,0x50eef0u),property_rows=read(m,current_properties+0x44u);
    write(m,row(property_rows,perception,0x3cu)+0x14u,allocated);write(m,cache,perception);
    const auto rows=read(m,queries+0x44u),query=row(rows,allocated,0x124u),defaults=read(m,0x474ce8u);
    write(m,query+0x44u,none);
    for(auto offset:{0x6au,0x66u,0x62u,0x68u,0x65u,0x63u})write(m,query+offset,0,1);
    write(m,query+4u,none);write(m,query+0x64u,1,1);
    for(auto offset:{0x67u,0x6du,0x69u})write(m,query+offset,0,1);
    write(m,query+0x40u,none);write(m,query+0x6bu,0,1);
    write(m,query+0x4cu,read(m,defaults));write(m,query+0x50u,read(m,defaults+4u));
    const auto z=read(m,defaults+8u);write(m,query+0x58u,none,2);write(m,query+0x48u,none);write(m,query+0x5cu,0,1);write(m,query+0x54u,z);
    native_campaign_ai_tracking_extension_reset_tu15(m,query+0x70u);return allocated;
}
}
