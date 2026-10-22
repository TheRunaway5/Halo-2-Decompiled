#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Whole original TU1.5 teleporter update, occupied-exit/deepest-contact
// selection and blocked notices. Existing live world services keep their ABI.
bool source_teleporter_tu15_owns_entry(std::uint32_t actual_entry);
void source_run_teleporter_tu15(SourcePhysicsContactMemory&,std::uint32_t actual_entry,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
