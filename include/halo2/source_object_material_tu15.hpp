#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Original18B4C0 resolves a living/dead unit material, falling back to the
// model's first damage-info indirect material. Its SHORT output belongs to
// the actual caller, including callers that provide a stack-local result.
bool source_object_material_tu15_owns_entry(std::uint32_t);
void source_run_object_material_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
