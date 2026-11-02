#include "halo2/native_campaign_session_timers_tu15.hpp"

namespace halo2 {
NativeCampaignSessionTimerClockRequiredTu15::NativeCampaignSessionTimerClockRequiredTu15(
    std::uint32_t address):std::runtime_error("Native session timer requires its actual imported millisecond producer"),cell(address){}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t at,unsigned width=4){
    std::array<std::byte,4> bytes{};memory.read(at,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
}
void write(SourcePhysicsContactMemory &memory,std::uint32_t at,std::uint32_t value,
    std::uint32_t instruction,const SourcePhysicsContactRecordObserver &observe){
    std::array<std::byte,4> bytes{};for(unsigned n=0;n<4u;++n)bytes[n]=std::byte(value>>(8u*n));
    if(!observe){memory.write(at,bytes);return;}
    SourcePhysicsContactRecordStore event{instruction,at,std::vector<std::byte>(4),{bytes.begin(),bytes.end()}};
    memory.read(at,event.before);memory.write(at,bytes);observe(event);
}
}
std::uint32_t native_campaign_session_retire_timers_value_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t filter,std::uint32_t slot,const NativeCampaignTransitionClockTu15 &producer,
    const SourcePhysicsContactRecordObserver &observe){
    std::uint32_t result=filter;const auto slot_offset=slot*0x98u;
    for(std::uint32_t group=0;group<5u;++group){
        if(filter!=0xffffffffu&&filter!=group)continue;
        if(group==4u)result=slot_offset;
        const auto state=0x515f70u+slot_offset+group*0x99cu;
        if(read(memory,state)!=3u)continue;
        const auto cached=read(memory,0x51d048u,1);
        write(memory,state,4,0x77818u,observe);
        std::uint32_t tick;
        if(cached)tick=read(memory,0x51d04cu);
        else{
            const auto cell=read(memory,0x41b774u);
            if(!producer)throw NativeCampaignSessionTimerClockRequiredTu15(cell);
            producer(cell);
            tick=read(memory,cell);
        }
        if(group==4u)result=tick;
        write(memory,state+4u,tick,0x77830u,observe);
    }
    return result;
}
void native_campaign_session_retire_timers_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t filter,std::uint32_t slot,const NativeCampaignTransitionClockTu15 &producer){
    (void)native_campaign_session_retire_timers_value_tu15(memory,filter,slot,producer);
}
}
