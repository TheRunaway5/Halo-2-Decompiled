#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Actual1BBB50: require two results from the authored flags5 iterator. No
// navigation geometry, synthetic area or persistent query storage is used.
bool native_campaign_ai_has_multiple_areas_tu15(const SourcePhysicsContactMemory &,
    std::uint32_t actor);
// Only the real indirect descriptor callback at1B0195 is admitted. The caller
// stores AX before killing EAX/EDX; actual retained FP state is preserved.
bool native_campaign_ai_area_count_tu15_owns_call(const SourcePhysicsContactRuntimeCall &);
void native_campaign_ai_area_count_tu15_run(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRuntimeCall &,SourcePhysicsWorldRuntimeAbi &);
// Process-wide diagnostic activity, outside saved gameplay state.
std::uint64_t native_campaign_ai_area_count_calls_tu15();
}
