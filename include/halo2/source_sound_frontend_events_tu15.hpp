#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete authenticated idle trap/interrupt/unlink and notifier CPU bodies.
// Native frontend commands, actual IRQ/DPC queues and exclusion are required.
// This is not the complete38E8C9 hardware configuration or DPC notification pass.
bool source_sound_frontend_events_tu15_owns_entry(std::uint32_t);
void source_run_sound_frontend_events_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
