#pragma once
#include "halo2/physics_vehicle_constraint_cleanup.hpp"

namespace halo2 {
// Exact original first-person effect vector-pack call. The complete
// native packer retains the original SSE/x87 rounding, flags and packed EAX.
inline bool source_first_person_vector_pack_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsContactRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint fallback,const SourcePhysicsContactRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &stores={}) {
    if(call.instruction!=0x18f2feu || call.target!=0x18bbc0u || call.tail)return false;
    auto control=abi.floating_point.value_or(fallback);
    source_run_physics_vehicle_constraint_cleanup_runtime(memory,call.target,abi,control,services,stores);
    abi.floating_point=control;return true;
}
} // namespace halo2
