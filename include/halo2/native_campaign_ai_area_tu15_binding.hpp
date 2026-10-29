#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool native_campaign_ai_area_tu15_owns_entry(std::uint32_t);
//1B6F20 ordinary callback transport. Decode actor, return owedAX16 behavior and
// consumeRET8. Dead registers/flags and the private iterator stack are not a VM.
void native_campaign_ai_area_tu15_run(SourcePhysicsContactMemory &,
    std::uint32_t,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint64_t native_campaign_ai_area_activity_tu15();
}
