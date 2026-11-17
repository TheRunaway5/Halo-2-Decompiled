#include "halo2/native_campaign_ai_movement_link_parameters_tu15.hpp"
#include "halo2/native_campaign_ai_character_property_tu15.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"
#include <bit>
#include <vector>

namespace halo2 {
namespace {
using U=std::uint32_t;
U read(const SourcePhysicsContactMemory &memory,U at,unsigned size=4) {
    std::array<std::byte,4> raw{};memory.read(at,std::span(raw).first(size));U value{};
    for(unsigned n=0;n<size;++n)value|=std::to_integer<U>(raw[n])<<(n*8u);
    return value;
}
void save(SourcePhysicsContactMemory &memory,U at,U value) {
    std::array<std::byte,4> raw{};for(unsigned n=0;n<4;++n)raw[n]=std::byte(value>>(n*8u));
    memory.write(at,raw);
}
void byte(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    U pc,U at,U value) {
    const bool observed=native_tu15_observes_retained_store(memory,observe,pc,at,1);
    SourcePhysicsContactRecordStore event{pc,at,{},std::vector<std::byte>{std::byte(value)}};
    if(observed){event.before.resize(1);memory.read(at,event.before);}
    memory.write(at,event.written);if(observed)observe(event);
}
std::int16_t signed_word(U value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
void scenario_speed(SourcePhysicsContactMemory &memory,SourcePhysicsWorldRuntimeAbi &abi) {
    auto &g=abi.gp;const auto kind=signed_word(g[1]);abi.xmm[0]={0,0,0,0};
    g[0]=read(memory,0x4ecc1cu);
    if(kind>=0 && kind<6 && g[0]) {
        g[2]=read(memory,g[0]+0xc8u);
        if(std::bit_cast<std::int32_t>(g[2])>0) {
            g[2]=read(memory,g[0]+0xccu);g[1]=U(std::int32_t(kind));
            abi.xmm[0][0]=read(memory,g[2]+g[1]*4u+0x80u);
        }
    }
    g[4]+=4u;
}
}
void native_campaign_ai_movement_link_parameters_call_tu15(SourcePhysicsContactMemory &memory,
    U entry,SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 &,
    const SourcePhysicsContactRecordObserver &observe) {
    if(entry==0x278fa0u){scenario_speed(memory,abi);return;}
    if(entry!=0x200310u)throw std::invalid_argument("Unknown native movement-link parameter entry");
    auto &g=abi.gp;const U sp=g[4];
    g[1]=read(memory,0x5020c8u);g[0]=(g[0]&0xffffu)*0x888u;
    save(memory,sp-4u,g[3]);save(memory,sp-8u,g[6]);
    g[6]=read(memory,g[1]+0x44u);g[1]=read(memory,g[0]+g[6]+0x18u);g[0]+=g[6];
    g[1]&=0xffffu;g[2]=g[1]*3u;g[1]=read(memory,0x4ecbd0u);g[1]=read(memory,g[1]+0x44u);
    g[3]=read(memory,g[1]+g[2]*4u+8u);g[1]=read(memory,g[0]+0x54u);
    g[4]=sp-12u;save(memory,g[4],0x20034au);
    native_campaign_ai_character_property_call_tu15(memory,0x1ecd00u,abi);
    abi.xmm[0]={read(memory,0x449fd4u),0,0,0};
    if(g[7])byte(memory,observe,0x200356u,g[7],0);
    if(g[0]) {
        g[6]=(g[6]&0xffff0000u)|read(memory,g[0]+0x1au,2);
        const auto count=signed_word(g[6]);
        if(count>0) {
            g[0]=count<6?U(std::int32_t(count)):6u;g[1]=g[0]-1u;
            g[4]=sp-12u;save(memory,g[4],0x20037cu);scenario_speed(memory,abi);
            if(g[7]) {
                g[2]=(g[2]&0xffffff00u)|U(count>=7);
                g[6]=read(memory,sp-8u);byte(memory,observe,0x200388u,g[7],g[2]&0xffu);
                g[3]=read(memory,sp-4u);g[4]=sp+4u;return;
            }
            g[6]=read(memory,sp-8u);g[3]=read(memory,sp-4u);g[4]=sp+4u;return;
        }
    }
    g[0]=(g[0]&0xffffff00u)|read(memory,g[3]+0xaau,1);
    if((g[0]&0xffu)==0) {
        g[0]=read(memory,g[3]);g[1]=read(memory,0x4f0414u);g[0]=(g[0]&0xffffu)*16u;
        g[2]=read(memory,g[0]+g[1]+8u);abi.xmm[0]={read(memory,g[2]+0x1f8u),0,0,0};
    }
    g[6]=read(memory,sp-8u);g[3]=read(memory,sp-4u);g[4]=sp+4u;
}
}
