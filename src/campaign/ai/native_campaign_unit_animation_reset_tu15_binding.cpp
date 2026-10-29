#include "halo2/native_campaign_unit_animation_reset_tu15.hpp"
#include "halo2/native_campaign_unit_animation_reset_tu15_binding.hpp"
#include <array>
#include <atomic>
#include <stdexcept>
namespace halo2 {
namespace {std::atomic<std::uint64_t> calls{};}
bool native_campaign_unit_animation_reset_tu15_owns_entry(std::uint32_t entry){return entry==0x116920u;}
void native_campaign_unit_animation_reset_tu15_run(SourcePhysicsContactMemory&m,std::uint32_t entry,
        SourcePhysicsWorldRuntimeAbi&abi,const SourcePhysicsWorldRuntimeServices&services){
    if(!native_campaign_unit_animation_reset_tu15_owns_entry(entry))throw std::invalid_argument("Unowned native unit animation reset");
    calls.fetch_add(1,std::memory_order_relaxed);if(services.instruction)services.instruction(entry);
    native_campaign_unit_animation_reset_tu15(m,abi.gp[0]);
    std::array<std::byte,4>return_word{};m.read(abi.gp[4],return_word);abi.gp[4]+=4u;
}
std::uint64_t native_campaign_unit_animation_reset_activity_tu15(){return calls.load(std::memory_order_relaxed);}
}
