#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete TU1.5 AI behavior-mask C++ constructors, storage/new-map, cold actor, binding and birth-state
// owners. They borrow the actual source bank, arena, tags and object owners.
// Shared1164D0 queries actual retained unit-extension flags;116440/116400
// and1E14F0 own its original four-byte transition and duration input.
// Shared2039B0 queries unfinished recorded animations in the existing
// startup-owned5021FC pool; its original AL result and object argument remain.
// The public Hunter's114232 caller reaches complete11DDF0 and its observed
// original unit/animation callees over the same retained object bank.
// Unprovided squad, perception, lifecycle or frame owners fail explicitly.
// This module does not supply AI decisions or replace missing firing input.
bool source_ai_actor_tu15_owns_entry(std::uint32_t);
void source_run_ai_actor_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
