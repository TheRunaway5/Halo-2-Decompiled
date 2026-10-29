#include "halo2/native_campaign_ai_query_start_tu15.hpp"
#include "halo2/native_campaign_ai_query_scheduler_tu15.hpp"
#include <array>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
}
NativeCampaignAiQueryStartResult native_campaign_ai_query_start_tu15(
    SourcePhysicsContactMemory &memory,std::uint32_t actor,std::uint32_t query,
    const SourcePhysicsContactRecordObserver &observe) {
    const auto pool=read(memory,0x5020c8u),initial_rows=read(memory,pool+0x44u);
    const auto self=initial_rows+(actor&0xffffu)*0x888u,group=read(memory,self+0x7cu);
    if(group==0xffffffffu)return {0,false};
    const auto groups=read(memory,0x50eef8u),group_rows=read(memory,groups+0x44u);
    auto member=read(memory,group_rows+(group&0xffffu)*0x50u+0x18u);
    std::uint32_t reservations{};bool did_reserve=false;
    while(member!=0xffffffffu) {
        const auto actor_rows=read(memory,pool+0x44u),row=actor_rows+(member&0xffffu)*0x888u;
        const auto next=read(memory,row+0x80u);
        if(row!=self&&native_campaign_ai_query_reserve_member_tu15(memory,query,member,3,0x3f800000u,observe)) {
            ++reservations;did_reserve=true;
        }
        member=next;
    }
    return {std::uint16_t(reservations),did_reserve};
}
}
