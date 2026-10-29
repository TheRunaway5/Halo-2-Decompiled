#include "halo2/native_campaign_screen_lifecycle_tu15_binding.hpp"
#include "halo2/native_campaign_screen_lifecycle_tu15.hpp"
#include <atomic>
#include <stdexcept>

namespace halo2 {
namespace {
std::atomic<std::uint64_t> stop_calls{}, capture_calls{};
}
bool native_campaign_screen_lifecycle_tu15_owns_entry(std::uint32_t entry) {
    return entry==0x1fcb0u || entry==0x1fce0u;
}
void native_campaign_screen_lifecycle_tu15_run(SourcePhysicsContactMemory &memory,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &observe) {
    if(!native_campaign_screen_lifecycle_tu15_owns_entry(entry))
        throw std::invalid_argument("Unowned native cinematic lifecycle entry");
    if(entry==0x1fcb0u) {
        if(abi.eflags&0x400u)
            throw std::invalid_argument("Native cinematic reset requires the ordinary clear-direction caller ABI");
        stop_calls.fetch_add(1,std::memory_order_relaxed);
        native_campaign_screen_stop_tu15(memory,observe);
    } else {
        capture_calls.fetch_add(1,std::memory_order_relaxed);
        native_campaign_screen_capture_complete_tu15(memory,observe);
    }
    // Reading at completion preserves a real unavailable-return boundary
    // after preceding persistent publications; no return address is supplied.
    std::array<std::byte,4> return_word;
    memory.read(abi.gp[4],return_word);
    abi.gp[4]+=4u;
}
NativeCampaignScreenLifecycleActivityTu15 native_campaign_screen_lifecycle_activity_tu15() {
    return {stop_calls.load(std::memory_order_relaxed),capture_calls.load(std::memory_order_relaxed)};
}
}
