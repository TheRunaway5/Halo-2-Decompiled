#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Complete TU1.5 2F5600 collector lifetime and query-call adapter. The nested
// shape-pair query retains its explicit original owner; no contact is supplied.
void native_tu15_scoped_contact_query(SourcePhysicsContactMemory &,
    SourcePhysicsWorldRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
