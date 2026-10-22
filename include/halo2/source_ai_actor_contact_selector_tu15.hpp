#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Original272200 reads the actor's selected contact through the existing
// complete267410 lookup. The real banks and full register/stack ABI are required.
bool source_ai_actor_contact_selector_tu15_owns_entry(std::uint32_t);
void source_run_ai_actor_contact_selector_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
