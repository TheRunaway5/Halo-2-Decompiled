#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Pure action MOVs have no original hardware preimage load. A native owner
// must perform the real action before the observer receives unknown `before`.
class SourceSoundPioActionMemory:public SourcePhysicsContactMemory {
public:
    virtual void write_pio_action(std::uint32_t instruction,std::uint32_t address,
        std::span<const std::byte>)=0;
};
bool source_sound_pio_storage_tu15_owns_entry(std::uint32_t);
void source_run_sound_pio_storage_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
