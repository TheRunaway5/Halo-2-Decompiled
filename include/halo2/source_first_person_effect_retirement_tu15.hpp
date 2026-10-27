#pragma once
#include "halo2/physics_vehicle_environment_state.hpp"

namespace halo2 {
// Exact original first-person effect/part-retirement calls. The existing complete
// native owner retains the caller ABI and retires real linked effect records.
inline bool source_first_person_effect_retirement_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsContactRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint fallback,const SourcePhysicsContactRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &stores={}) {
    const bool effect=call.instruction==0x17b262u && call.target==0x17a230u;
    const bool part=call.instruction==0x17a6a0u && call.target==0x16e8a0u;
    if(call.tail || (!effect && !part))return false;
    auto control=abi.floating_point.value_or(fallback);
    source_run_physics_vehicle_environment_state_runtime(memory,call.target,abi,control,services,stores);
    abi.floating_point=control;return true;
}
} // namespace halo2
