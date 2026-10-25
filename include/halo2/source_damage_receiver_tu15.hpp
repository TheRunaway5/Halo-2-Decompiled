#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_damage_receiver_tu15_owns_entry(std::uint32_t);
// Complete original D91AF receiver callback and its declared dependent owner.
// Original guards, arguments, stores and external callees remain required.
void source_run_damage_receiver_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
