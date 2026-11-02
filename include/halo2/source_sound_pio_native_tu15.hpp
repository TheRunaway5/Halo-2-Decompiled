#pragma once
#include "halo2/source_sound_pio_storage_tu15.hpp"
namespace halo2 {
// Semantic native integer owners for complete38E0CE and its38E159 suffix.
// Instruction callbacks remain observation/cancellation boundaries. Actual
// source storage and action owners supply every read/write; no FIFO capacity,
// tracking method, device completion or startup result is manufactured.
bool source_sound_pio_native_tu15_owns_entry(std::uint32_t);
void source_run_sound_pio_native_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
