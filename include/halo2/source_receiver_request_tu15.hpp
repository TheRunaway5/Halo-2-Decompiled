#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Complete original receiver request and queued callbacks. The retained
// shadow-frame owner supplies the actual queue; no request result is invented.
bool source_receiver_request_tu15_owns_entry(std::uint32_t);
void source_run_receiver_request_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
