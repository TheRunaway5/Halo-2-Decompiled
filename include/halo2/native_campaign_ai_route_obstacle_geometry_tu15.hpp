#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Authored obstacle marker selection (279640/279900/279B30): EAX capacity,
// stack object, output records, flags; AX count and RET C. Uses the native
// BAD60 marker closure with the actual caller frame and authored marker names.
// 279600 tests an obstacle's world orientation (ECX record, AL, RET).
// 2CAB80 appends an obstacle (EDX list, ESI point, BX flags, stack object and
// radius), retaining the original 64-row capacity and AL/RET8 contract.
void native_campaign_ai_route_obstacle_geometry_call_tu15(
    SourcePhysicsContactMemory &,std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
}
