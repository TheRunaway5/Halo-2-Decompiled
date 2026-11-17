#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Actual 202660: EAX instance, ECX object, stack [sector, encoded, point],
// RET 12. Actual 2185E0: EAX encoded, ECX object, RET; returns frame in AX.
// Owns static BSP projection, instance/object sector mapping and moving-frame
// mapping, with the original caller frame and floating-point state retained.
// Actual218960: EAX frame, ECX source, EBX destination, RET, AL resolved.
void native_campaign_ai_navigation_lookup_call_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry, SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
}
