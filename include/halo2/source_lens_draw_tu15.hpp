#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_lens_draw_tu15_owns_entry(std::uint32_t);
// Complete original TU1.5 reflection, bitmap and primitive bodies. Reached
// missing resource/graphics owners retain their full ABI and fail explicitly.
void source_run_lens_draw_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
