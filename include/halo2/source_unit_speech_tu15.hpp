#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_unit_speech_tu15_owns_entry(std::uint32_t);
void source_run_unit_speech_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
class PhysicsLiveWorldTu15;
// Original18F510 consumes the actual object/tag/permutation and elapsed PCM
// seconds. Resource admission, facial-channel binding and weights stay with
// the original owners. The caller supplies no Xbox voice or readiness flag.
void source_unit_speech_tu15(PhysicsLiveWorldTu15 &,std::uint32_t object,
    std::uint32_t sound,std::int32_t pitch_range,std::int32_t permutation,
    float elapsed_seconds);
// Actual BCDF0 unit validation and D3300 matching-tag retirement. The sound
// tag is resolved by the native voice owner rather than a fabricated Xbox id.
void source_unit_speech_stop_tu15(PhysicsLiveWorldTu15 &,std::uint32_t object,
    std::uint32_t sound);
}
