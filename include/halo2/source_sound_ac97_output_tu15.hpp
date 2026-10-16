#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Unchanged output-DMA channel/EP construction and restart CPU bodies.
// Codec queries, native interrupt/timer lifetime, channel reset/control and
// processor bytes remain required external resource boundaries. Merely
// lowering these bodies does not admit their MMIO reads/writes or playback.
bool source_sound_ac97_output_tu15_owns_entry(std::uint32_t);
void source_run_sound_ac97_output_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
