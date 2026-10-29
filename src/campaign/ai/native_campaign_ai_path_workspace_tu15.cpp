#include "halo2/native_campaign_ai_path_workspace_tu15.hpp"
#include <array>

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=4) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
}
std::uint32_t NativeCampaignAiPathMemoryWordsTu15::word(unsigned index)const {
    return read(memory_,address_+std::uint32_t(index)*4u);
}
void native_campaign_ai_path_workspace_initialize_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t workspace,const NativeCampaignAiPathWordsTu15 &main,
    const NativeCampaignAiPathWordsTu15 *policy,const NativeCampaignAiPathWordsTu15 *transport,
    std::uint32_t application_context,const SourcePhysicsContactRecordObserver &observe) {
    publish(memory,observe,0x27b018,workspace+0x54,0,1);
    publish(memory,observe,0x27b01b,workspace+0x90,0xffffu,2);
    publish(memory,observe,0x27b024,workspace+0xae,0,2);
    publish(memory,observe,0x27b02b,workspace+0x110b0,0,2);
    if(policy) {
        for(unsigned n=0;n<7;++n)publish(memory,observe,0x27b03c,workspace+0x70+n*4u,policy->word(n));
    }else {
        publish(memory,observe,0x27b040,workspace+0x70,0,2);
        publish(memory,observe,0x27b044,workspace+0x72,0,2);
    }
    const auto bsp=read(memory,0x4ecc18u);
    const auto count=std::int32_t(read(memory,bsp+0xc4u));
    const auto navigation=count>0 ? read(memory,bsp+0xc8u) : 0u;
    publish(memory,observe,0x27b066,workspace+0x8c,navigation);
    for(unsigned n=0;n<20;++n)publish(memory,observe,0x27b073,workspace+n*4u,main.word(n));
    publish(memory,observe,0x27b079,workspace+0x50,application_context);
    publish(memory,observe,0x27b07c,workspace+0xac,0,1);
    if(transport) {
        for(unsigned n=0;n<7;++n)publish(memory,observe,0x27b091,workspace+0x140b8+n*4u,transport->word(n));
        publish(memory,observe,0x27b094,workspace+0x14188,0,2);
    }else {
        constexpr std::array<std::uint32_t,7> sites{0x27b0a7,0x27b0a9,0x27b0ac,0x27b0af,0x27b0b2,0x27b0b5,0x27b0b8};
        for(unsigned n=0;n<7;++n)publish(memory,observe,sites[n],workspace+0x140b8+n*4u,0);
        publish(memory,observe,0x27b0bc,workspace+0x14188,0,2);
    }
}
}
