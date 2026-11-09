#pragma once

#include "halo2/physics_world_runtime.hpp"

namespace halo2 {

// C1270, invoked by the original C13B0 recovery table. Builds the list of
// retained root objects for C12F0. This step never retires an object.
void native_tu15_collect_object_recovery_candidates(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRuntimeCall &,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});

// C12F0 with no candidate. A positive count needs the original object
// eligibility and deletion owners before any list element can be removed.
void native_tu15_finish_empty_object_recovery(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRuntimeCall &,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});

}
