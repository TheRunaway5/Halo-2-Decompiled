#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Seven whole original callbacks, in _cinit order46DD00..46DD1C.
// They copy authored operands into eight retained AI geometry/aiming scalars. Storage
// and operands must exist; no completed value is supplied by this executor.
bool source_campaign_ai_scalar_defaults_tu15_owns_entry(std::uint32_t);
void source_run_campaign_ai_scalar_defaults_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
