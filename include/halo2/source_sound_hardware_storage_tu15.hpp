#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Actual unchanged CPU requirements,16 descriptor allocations and tail pool
// construction. Native DMA/pool and scoped exclusion services are required.
// No frontend/voice/DSP configuration is supplied by this storage component.
bool source_sound_hardware_storage_tu15_owns_entry(std::uint32_t);
void source_run_sound_hardware_storage_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
