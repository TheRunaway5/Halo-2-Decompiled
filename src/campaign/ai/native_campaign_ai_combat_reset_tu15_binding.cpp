#include "halo2/native_campaign_ai_combat_reset_tu15.hpp"
#include "halo2/native_campaign_ai_combat_reset_tu15_binding.hpp"
#include <array>
#include <atomic>
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {std::atomic<std::uint64_t> entry_calls{};}
bool native_campaign_ai_combat_reset_tu15_owns_entry(std::uint32_t entry){return entry==0x1ec7b0u;}
void native_campaign_ai_combat_reset_tu15_run(SourcePhysicsContactMemory&m,std::uint32_t entry,
        SourcePhysicsWorldRuntimeAbi&abi,const SourcePhysicsWorldRuntimeServices&services){
    if(!native_campaign_ai_combat_reset_tu15_owns_entry(entry))throw std::invalid_argument("Unowned native combat reset");
    entry_calls.fetch_add(1,std::memory_order_relaxed);
    if(abi.eflags&0x400u)throw std::invalid_argument("Native combat reset needs engine ABI forward string direction");
    if(services.instruction)services.instruction(entry);
    native_campaign_ai_combat_reset_tu15(m,abi.gp[0],std::bit_cast<std::int16_t>(std::uint16_t(abi.gp[7])));
    std::array<std::byte,4>return_word{};m.read(abi.gp[4],return_word);abi.gp[4]+=4u;
}
std::uint64_t native_campaign_ai_combat_reset_activity_tu15(){return entry_calls.load(std::memory_order_relaxed);}
}
