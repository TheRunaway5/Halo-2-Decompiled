#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
bool native_campaign_ai_area_choice_tu15_owns_call(const SourcePhysicsContactRuntimeCall &);
void native_campaign_ai_area_choice_tu15_run(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRuntimeCall &,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint64_t native_campaign_ai_area_choice_calls_tu15();
}
