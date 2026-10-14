#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_map_world_support_tu15_owns_entry(std::uint32_t);
//Complete actual callbacks reached by ordinary authored map construction.
//The original retained memory/caller ABI and all external owners are required.
void source_run_map_world_support_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
