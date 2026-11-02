#include "halo2/native_campaign_session_timers_tu15_binding.hpp"
#include <atomic>

namespace halo2 {
namespace {
std::atomic<std::uint64_t> calls{};
std::uint32_t returned(const SourcePhysicsContactMemory &memory,std::uint32_t address){
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4u;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
    return value;
}
}
NativeCampaignSessionTimerReturnRequiredTu15::NativeCampaignSessionTimerReturnRequiredTu15(
    std::uint32_t target):std::runtime_error("Native session timer requires its actual caller continuation"),target_(target){}
bool native_campaign_session_timers_tu15_owns_entry(std::uint32_t entry){return entry==0x777e0u;}
bool native_campaign_session_timers_tu15_owns_call(const SourcePhysicsContactRuntimeCall &call){
    return !call.tail&&call.target==0x777e0u&&
        (call.instruction==0x77ad3u||call.instruction==0x77bd3u||
         call.instruction==0x77c1au||call.instruction==0x78204u);
}
void native_campaign_session_timers_tu15_run(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &abi,
    const NativeCampaignTransitionClockTu15 &producer,const SourcePhysicsContactRecordObserver &observe){
    if(!native_campaign_session_timers_tu15_owns_call(call))
        throw std::invalid_argument("Unqualified native session timer caller");
    calls.fetch_add(1,std::memory_order_relaxed);
    const auto filter=call.instruction==0x78204u?abi.gp[5]:0xffffffffu;
    const auto slot=call.instruction==0x77bd3u?abi.gp[0]:
        (call.instruction==0x78204u?abi.gp[7]:abi.gp[6]);
    abi.gp[0]=native_campaign_session_retire_timers_value_tu15(memory,filter,slot,producer,observe);
    const auto target=returned(memory,abi.gp[4]);
    if(target!=call.instruction+5u)throw NativeCampaignSessionTimerReturnRequiredTu15(target);
    abi.gp[4]+=12u;
}
std::uint64_t native_campaign_session_timers_calls_tu15(){return calls.load(std::memory_order_relaxed);}
}
