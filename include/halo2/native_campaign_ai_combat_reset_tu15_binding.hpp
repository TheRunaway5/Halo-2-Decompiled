#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool native_campaign_ai_combat_reset_tu15_owns_entry(std::uint32_t);
void native_campaign_ai_combat_reset_tu15_run(SourcePhysicsContactMemory &,
    std::uint32_t,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsWorldRuntimeServices & = {});
std::uint64_t native_campaign_ai_combat_reset_activity_tu15();
}
