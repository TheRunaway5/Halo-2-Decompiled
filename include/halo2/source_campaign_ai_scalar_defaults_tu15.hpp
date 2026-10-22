#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Whole original3826A0/3826C0/3826F0, in _cinit order46DD00..46DD0C.
// They copy authored operands into four retained AI geometry scalars. Storage
// and operands must exist; no completed value is supplied by this executor.
bool source_campaign_ai_scalar_defaults_tu15_owns_entry(std::uint32_t);
void source_run_campaign_ai_scalar_defaults_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
