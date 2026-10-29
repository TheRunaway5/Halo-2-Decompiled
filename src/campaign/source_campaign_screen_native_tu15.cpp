#include "halo2/source_campaign_screen_tu15.hpp"
#include "halo2/native_campaign_screen_lifecycle_tu15_binding.hpp"
#include "halo2/native_campaign_screen_timed_tu15_binding.hpp"
#include <stdexcept>

namespace halo2 {
bool source_campaign_screen_tu15_owns_entry(std::uint32_t entry) {
    return native_campaign_screen_lifecycle_tu15_owns_entry(entry) ||
        native_campaign_screen_timed_tu15_owns_entry(entry);
}
void source_run_campaign_screen_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint policy,const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver &observe) {
    if(!source_campaign_screen_tu15_owns_entry(entry))
        throw std::invalid_argument("Unowned native campaign screen entry");
    if(!abi.floating_point)abi.floating_point=policy;
    if(native_campaign_screen_lifecycle_tu15_owns_entry(entry))
        native_campaign_screen_lifecycle_tu15_run(memory,entry,abi,observe);
    else native_campaign_screen_timed_tu15_run(memory,entry,abi,observe);
}
}
