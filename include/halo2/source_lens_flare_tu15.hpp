#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Complete original effect lens-request branch, admission, packed request and
// retained request-pool helpers. Callers own actual view/tag/pool lifetimes.
// Original frame/view gates own admission; display stamps and GPU query
// completion still require their actual native renderer owners.
bool source_lens_flare_tu15_owns_entry(std::uint32_t);
void source_run_lens_flare_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
