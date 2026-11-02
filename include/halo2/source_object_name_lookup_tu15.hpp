#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original BD790. The signed AX index addresses the native name
// table behind 4EABA0; invalid indices return NONE without reading that bank.
bool source_object_name_lookup_tu15_owns_entry(std::uint32_t);
void source_run_object_name_lookup_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
}
