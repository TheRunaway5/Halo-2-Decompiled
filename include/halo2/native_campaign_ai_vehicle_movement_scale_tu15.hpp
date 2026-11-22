#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
class SourceRuntimeMemory;

// The live 2A46BA -> 202130 movement-scale call accepts EAX vehicle, ECX
// actor, and the already-pushed 2A46BF return word. The caller owns its
// in-tick execution and topology and must exclude instruction observation.
// Returns false before publishing any store or register on an unsupported
// input. A successful call publishes the original stores and RET 1C ABI.
bool native_campaign_ai_vehicle_movement_scale_tu15_try(SourceRuntimeMemory &,
    const SourcePhysicsContactRuntimeCall &, SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
