#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original TU1.5 model resource/render bodies over the live bank.
// Missing downstream services preserve the actual caller ABI and fail.
bool source_model_render_tu15_owns_entry(std::uint32_t);
void source_run_model_render_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
