#include "halo2/native_campaign_ai_area_tu15_binding.hpp"
#include "halo2/native_campaign_ai_area_tu15.hpp"
#include <atomic>
#include <stdexcept>
namespace halo2 {
namespace {
std::atomic<std::uint64_t> calls{};
std::uint32_t word(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
}
bool native_campaign_ai_area_tu15_owns_entry(std::uint32_t entry){return entry==0x1b6f20u;}
void native_campaign_ai_area_tu15_run(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &) {
    if(!native_campaign_ai_area_tu15_owns_entry(entry))throw std::invalid_argument("Unowned native campaign area callback");
    calls.fetch_add(1,std::memory_order_relaxed);
    const auto actor=word(memory,abi.gp[4]+4u);
    const auto result=native_campaign_ai_area_behavior_tu15(memory,actor);
    word(memory,abi.gp[4]);
    abi.gp[0]=(abi.gp[0]&0xffff0000u)|result;
    abi.gp[4]+=12u;
}
std::uint64_t native_campaign_ai_area_activity_tu15(){return calls.load(std::memory_order_relaxed);}
}
