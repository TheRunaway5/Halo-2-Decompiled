#pragma once
#include "halo2/physics_contact_runtime.hpp"

namespace halo2 {
// Complete TU1.5 DC540. The caller owns its original model/object bank,
// request key and ABI; D9110 remains an actual required source owner.
void source_run_damage_model_children_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsContactRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint64_t source_damage_model_children_native_activity();
}
