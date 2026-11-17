#include "halo2/native_campaign_ai_behavior_129_tu15.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"
#include <array>
#include <stdexcept>
namespace halo2 {
namespace {
using U=std::uint32_t;
U read(const SourcePhysicsContactMemory &memory,U at,unsigned width=4) {
    std::array<std::byte,4> raw{};memory.read(at,std::span(raw).first(width));U value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<U>(raw[n])<<(n*8u);return value;
}
void store(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    U pc,U at,U value,unsigned width) {
    const bool watched=native_tu15_observes_retained_store(memory,observe,pc,at,width);
    SourcePhysicsContactRecordStore event{pc,at,{},std::vector<std::byte>(width)};
    if(watched){event.before.resize(width);memory.read(at,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(at,event.written);if(watched)observe(event);
}
}
void native_campaign_ai_behavior_129_call_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &observe) {
    auto &g=abi.gp;
    if(entry==0x1cacb0u) {
        g[1]=read(memory,g[4]+8u);g[2]=(g[2]&0xffffff00u)|read(memory,g[1]+0x10u,1);
        g[0]=read(memory,0x47c1a8u);
        if(g[2]&255u)g[0]=(g[0]&0xffff0000u)|read(memory,0x47c1a4u,2);
        g[4]+=16u;return;
    }
    if(entry==0x1cae70u) {
        g[1]=read(memory,g[4]+4u);g[0]=read(memory,0x5020c8u);g[2]=read(memory,g[0]+0x44u);
        g[1]=(g[1]&0xffffu)*0x888u;g[0]=read(memory,g[1]+g[2]+0x338u);g[1]+=g[2];
        if(g[0]!=0xffffffffu) {
            g[2]=read(memory,0x50eef0u);g[0]=(g[0]&0xffffu)*0x3cu;g[0]+=read(memory,g[2]+0x44u);
            g[0]=(g[0]&0xffff0000u)|read(memory,g[0]+0x24u,2);
            const auto kind=std::int16_t(std::uint16_t(g[0]));
            if(kind>=1 && kind<=2) {
                store(memory,observe,0x1caeb7u,g[1]+0x41cu,4,2);
                store(memory,observe,0x1caec0u,g[1]+0x420u,2,2);
                store(memory,observe,0x1caec9u,g[1]+0x488u,1,1);
            }
        }
        g[4]+=12u;return;
    }
    if(entry!=0x1cacd0u)throw std::invalid_argument("Unknown native behavior129 entry");
    std::array<std::byte,4> raw{};memory.read(abi.gp[4]+8u,raw);std::uint32_t state{};
    for(unsigned n=0;n<4;++n)state|=std::to_integer<std::uint32_t>(raw[n])<<(n*8u);
    abi.gp[0]=state;
    const bool watched=native_tu15_observes_retained_store(memory,observe,0x1cacd4u,state+0xcu,4);
    SourcePhysicsContactRecordStore event{0x1cacd4u,state+0xcu,{},std::vector<std::byte>(4)};
    if(watched){event.before.resize(4);memory.read(state+0xcu,event.before);}
    memory.write(state+0xcu,event.written);if(watched)observe(event);
    abi.gp[0]=(abi.gp[0]&0xffffff00u)|1u;abi.gp[4]+=12u;
}
}
