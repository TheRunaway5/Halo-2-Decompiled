#pragma once
#include "halo2/transform.hpp"
#include <cstdint>
#include <span>
#include <optional>

namespace halo2 {
// A read-only view of the same current WORLD nodes used by scene geometry.
// The scene producer retains the storage until its next pose/synchronization.
struct SourceSceneEffectOwner {
    std::size_t placement{};
    std::uint32_t object=0xffffffffu;
    std::uint64_t created_tick{};
    std::span<const Transform> nodes;
    std::span<const std::uint8_t> permutations;
    // BBE00's completed world origin, separately from animated node zero.
    std::optional<Vec3> origin;
};
} // namespace halo2
