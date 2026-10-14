#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete authenticated owners, plus explicitly bounded blocks: DBB90
// physical delivery DBD64..DBFCC and D9A30 radius geometry D9AB2..D9CA4.
// They require original caller frames and stop before attribution/cone owners.
// D9ACC is the diagnostic geometry entry with declared closest-point inputs.
bool source_death_cycle_runtime_tu15_owns_entry(std::uint32_t);
void source_run_death_cycle_runtime_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actual_entry,SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
