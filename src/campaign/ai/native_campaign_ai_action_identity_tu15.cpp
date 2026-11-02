#include "halo2/native_campaign_ai_action_identity_tu15.hpp"
#include <array>
#include <bit>
namespace halo2 { namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
std::int32_t signed_word(std::uint32_t word){return std::bit_cast<std::int16_t>(std::uint16_t(word));}
}
void native_campaign_ai_action_history_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t actor,std::uint32_t goal,std::uint8_t flag,const SourcePhysicsContactRecordObserver &observe) {
    if(goal==read(memory,0x47d6c0u))return;
    const auto pool=read(memory,0x5020c8u),rows=read(memory,pool+0x44u);
    const auto record=rows+(actor&0xffffu)*0x888u;
    bool found{};
    for(unsigned n=0;n<4;++n) {
        const auto entry=record+0x400u+n*6u;
        if(read(memory,entry+2u)==goal) {
            found=true;
            if(!flag)publish(memory,observe,0x26c553u,entry,flag,1);
        }
    }
    if(found)return;
    const auto next=(signed_word(read(memory,record+0x3feu,2))+1)%4;
    publish(memory,observe,0x26c573u,record+0x3feu,std::uint16_t(next),2);
    auto index=next;
    publish(memory,observe,0x26c580u,record+0x400u+std::uint32_t(index)*6u,flag,1);
    index=signed_word(read(memory,record+0x3feu,2));
    publish(memory,observe,0x26c58du,record+0x402u+std::uint32_t(index)*6u,goal,4);
}
std::uint32_t native_campaign_ai_dynamic_goal_definition_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t goal) {
    const auto owner=(goal>>16u)&0x7fffu,pool=read(memory,0x52d87cu);
    const auto rows=read(memory,pool+0x44u),record=rows+owner*0x484u;
    const auto index=signed_word(goal);
    if(index<0)return 0;
    const auto count=signed_word(read(memory,record+8u,2));
    if(index>=count)return 0;
    const auto mask=read(memory,record+0x40u+std::uint32_t(index>>5)*4u);
    if(!(mask&(1u<<(std::uint32_t(index)&31u))))return 0;
    return record+0x44u+std::uint32_t(index)*32u;
}
std::uint32_t native_campaign_ai_goal_definition_tu15(const SourcePhysicsContactMemory &memory,
    std::uint32_t goal) {
    if(goal==read(memory,0x47d6c0u))return 0;
    if(goal&0x80000000u)return native_campaign_ai_dynamic_goal_definition_tu15(memory,goal);
    const auto index=signed_word(goal);const auto zone=goal>>16u;
    const auto scenario=read(memory,0x4ecc20u),zones=read(memory,scenario+0x16cu);
    const auto record=zones+zone*0x38u;
    if(index<0)return 0;
    if(index>=std::bit_cast<std::int32_t>(read(memory,record+0x28u)))return 0;
    return read(memory,record+0x2cu)+std::uint32_t(index)*32u;
}
}
