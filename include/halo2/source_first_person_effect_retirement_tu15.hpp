#pragma once
#include "halo2/physics_vehicle_environment_state.hpp"

namespace halo2 {
// Exact original first-person effect/part and resource-retirement calls. The
// existing complete native owner retains the caller ABI and retires real pool
// records. Resource1262F0 replaces a tracking row,126400 releases a metadata
// row, and1264E0 retires each of its two completed asynchronous queues.
inline bool source_first_person_effect_retirement_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsContactRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint fallback,const SourcePhysicsContactRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &stores={}) {
    const bool effect=call.instruction==0x17b262u && call.target==0x17a230u;
    const bool part=call.instruction==0x17a6a0u && call.target==0x16e8a0u;
    const bool resource=call.target==0x16e8a0u &&
        (call.instruction==0x126307u || call.instruction==0x126466u ||
         call.instruction==0x12657au || call.instruction==0x1265f5u);
    if(call.tail || (!effect && !part && !resource))return false;
    auto control=abi.floating_point.value_or(fallback);
    source_run_physics_vehicle_environment_state_runtime(memory,call.target,abi,control,services,stores);
    abi.floating_point=control;return true;
}
} // namespace halo2
