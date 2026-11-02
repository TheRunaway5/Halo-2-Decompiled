#pragma once
#include "halo2/native_tu15_physics_frame.hpp"

namespace halo2 {
enum class NativeTu15LightUpStage {perpendicular, normalized};
using NativeTu15LightUpStore = std::function<void(NativeTu15LightUpStage,unsigned,std::uint32_t)>;
struct NativeTu15LightUpResult {
    std::array<std::uint32_t,3> up;
    std::uint32_t magnitude_bits{};
};
// Complete31B30 and its30BD0 tail: x87 cross/norm, SSE perpendicular,
// then the shared native normalizer. Raw REAL bits preserve NaN payloads and
// signed zeros. Private scalar intermediates have no retained stack address.
// Stage observers run after each arithmetic environment has been restored.
NativeTu15LightUpResult native_tu15_light_orthonormal_up(
    const SourcePhysicsContactMemory& constants, std::array<std::uint32_t,3> up,
    const std::array<std::uint32_t,3>& forward, NativeTu15PhysicsFrameFp&,
    const NativeTu15LightUpStore& = {});
}
