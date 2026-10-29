#include "halo2/native_campaign_ai_behavior_reset_tu15.hpp"
#include <array>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory &memory,std::uint32_t address){
    std::array<std::byte,4> data;memory.read(address,data);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(data[n])<<(n*8u);
    return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width){
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
}
bool native_campaign_ai_behavior_reset_tu15(SourcePhysicsContactMemory &memory,
    const NativeCampaignAiBehaviorResetArgumentsTu15 &arguments,const SourcePhysicsContactRecordObserver &observe){
    const auto actor=arguments.actor();
    const auto pool=word(memory,0x5020c8u);
    const auto rows=word(memory,pool+0x44u);
    const auto action=arguments.action();
    const auto record=rows+(actor&0xffffu)*0x888u;
    publish(memory,observe,0x1c9d00,record+0x50c,0,1);
    publish(memory,observe,0x1c9d06,record+0x5ac,0xffffffffu,4);
    publish(memory,observe,0x1c9d10,record+0x5b0,0xffffu,2);
    publish(memory,observe,0x1c9d19,record+0x5b4,0,2);
    publish(memory,observe,0x1c9d20,record+0x5b6,0,2);
    publish(memory,observe,0x1c9d27,record+0x4ac,0,2);
    publish(memory,observe,0x1c9d2e,record+0x504,0,2);
    publish(memory,observe,0x1c9d35,action+0xc,0,1);
    publish(memory,observe,0x1c9d38,action+0x10,0xffffffffu,4);
    publish(memory,observe,0x1c9d3f,action+0x38,0,2);
    publish(memory,observe,0x1c9d43,action+0x20,0,1);
    return true;
}
}
