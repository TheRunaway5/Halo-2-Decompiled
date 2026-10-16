#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Whole authenticated deferred notification/base PCM voice CPU bodies. Actual
// synchronization, DMA fence, atomic pending bits and virtual/timer owners are
// mandatory; unsupported branches retain their required callee boundaries.
// Exact three-instruction inlined metadata header constructor387073..387080.
void source_construct_sound_pcm_metadata_header_tu15(SourcePhysicsContactMemory&,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
bool source_sound_frontend_dpc_tu15_owns_entry(std::uint32_t);
void source_run_sound_frontend_dpc_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
