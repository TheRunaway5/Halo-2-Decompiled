#include "halo2/native_campaign_ai_cancel_support_tu15.hpp"
#include <array>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
}
std::uint32_t native_campaign_ai_actor_timer_tu15(const SourcePhysicsContactMemory &memory,std::uint32_t actor) {
    const auto records=read(memory,0x5020a4u);
    for(unsigned index=0;index<2;++index)if(read(memory,records+index*0x1cu)==actor)return records+index*0x1cu;
    return 0;
}
std::uint32_t native_campaign_ai_group_first_member_tu15(const SourcePhysicsContactMemory &memory,std::uint32_t group) {
    const auto pool=read(memory,0x50eef8u),rows=read(memory,pool+0x44u);
    return read(memory,rows+(group&0xffffu)*0x50u+0x18u);
}
void native_campaign_ai_group_members_begin_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t group,std::uint32_t iterator,const SourcePhysicsContactRecordObserver &observe) {
    const auto first=native_campaign_ai_group_first_member_tu15(memory,group);const auto address=iterator+4u;
    const std::array<std::byte,4> bytes{std::byte(first),std::byte(first>>8u),std::byte(first>>16u),std::byte(first>>24u)};
    if(!observe){memory.write(address,bytes);return;}
    SourcePhysicsContactRecordStore event{0x275acc,address,{}, {bytes.begin(),bytes.end()}};
    event.before.resize(4);memory.read(address,event.before);memory.write(address,bytes);observe(event);
}
}
