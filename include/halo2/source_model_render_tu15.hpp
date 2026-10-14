#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Native CPU texture view at the sole authenticated1D2EA call site. Complete
// original D3D helpers derive format/type/dimensions/pitches from the actual
// texture header. Original1D2D0 never reads the GPU multisample descriptor;
// that unowned field is left untouched, not supplied as a guessed device state.
void source_native_cache_bitmap_view_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete original TU1.5 combat/action bodies over the live world's bank.
// Missing downstream services preserve the actual caller ABI and fail.
bool source_model_render_tu15_owns_entry(std::uint32_t);
void source_run_model_render_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
