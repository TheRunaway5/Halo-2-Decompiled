#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Retained literal reference for complete2216D0 and its closed CPU gain
// dependencies. Normal execution uses the registered semantic native kernel.
// No source voice/admission, device state or callback result is supplied.
bool source_loop_combined_publication_tu15_owns_entry(std::uint32_t);
void source_run_loop_combined_publication_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
