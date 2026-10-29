#include "halo2/native_campaign_unit_action_support_tu15.hpp"
#include "halo2/native_campaign_unit_action_support_tu15_binding.hpp"
#include <array>
#include <atomic>
#include <stdexcept>
namespace halo2 {
namespace {std::array<std::atomic<std::uint64_t>,4> calls{};}
bool native_campaign_unit_action_support_tu15_owns_entry(std::uint32_t entry){return entry==0xb9390u||entry==0xe8f10u||entry==0x1efa70u||entry==0x1eec30u;}
bool native_campaign_unit_action_support_tu15_owns_call(std::uint32_t entry,const SourcePhysicsWorldRuntimeAbi&abi){
    if(!native_campaign_unit_action_support_tu15_owns_entry(entry))return false;
    if(entry!=0xe8f10u)return true;
    const auto type=abi.gp[7];return type==0x1cu||type==0x1du||type==0x1fu||type==0x20u;
}
void native_campaign_unit_action_support_tu15_run(SourcePhysicsContactMemory&m,std::uint32_t entry,
        SourcePhysicsWorldRuntimeAbi&abi,const SourcePhysicsWorldRuntimeServices&services){
    if(!native_campaign_unit_action_support_tu15_owns_entry(entry))throw std::invalid_argument("Unowned native unit action support");
    const auto activity=entry==0xb9390u?0u:entry==0xe8f10u?1u:entry==0x1efa70u?2u:3u;
    calls[activity].fetch_add(1,std::memory_order_relaxed);
    const auto read=[&](std::uint32_t p,unsigned n){std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8u);return v;};
    if(services.instruction)services.instruction(entry);
    if(entry==0xb9390u)native_campaign_unit_mark_update_tu15(m,abi.gp[0]);
    else if(entry==0xe8f10u)native_campaign_unit_interrupt_action_tu15(m,abi.gp[1],abi.gp[7]);
    else{
        NativeCampaignUnitActionArgumentsTu15 arguments{[&]{return std::uint16_t(read(abi.gp[4]+4u,2));},[&]{return std::uint8_t(read(abi.gp[4]+8u,1));}};
        if(entry==0x1efa70u)native_campaign_unit_notify_action_tu15(m,abi.gp[0],arguments);
        else native_campaign_unit_record_action_tu15(m,abi.gp[0],arguments);
    }
    read(abi.gp[4],4);abi.gp[4]+=(entry==0x1efa70u||entry==0x1eec30u)?12u:4u;
}
NativeCampaignUnitActionSupportActivityTu15 native_campaign_unit_action_support_activity_tu15(){
    return {calls[0].load(std::memory_order_relaxed),calls[1].load(std::memory_order_relaxed),calls[2].load(std::memory_order_relaxed),calls[3].load(std::memory_order_relaxed)};
}
}
