#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace halo2 {
struct SourceItemMotionStorage {
  std::uint32_t datum;
  std::uint8_t object_type;
  std::span<std::byte> body;
  std::span<const std::byte> definition;
};
struct SourceItemMotionInputs {
  float seconds_per_tick, gravity, floor_normal_limit, settling_speed_limit,
      contact_point_bias;
  std::array<float, 3> resting_linear;
  std::uint16_t contact_bank;
  // Literal original pre-existing cast scratch. Only a needed cast consumes
  // this backing; output producers retain their actual partial-write contract.
  std::span<std::byte, 0x5c> collision_scratch;
};
struct SourceItemMotionServices {
  std::function<void(std::uint32_t)> make_moving;
  std::function<bool(std::uint32_t, std::span<const std::byte, 12>,
      std::span<const std::byte, 12>, std::uint32_t, std::uint32_t,
      std::span<std::byte, 0x5c>)> cast;
  std::function<void(std::uint32_t, std::span<const std::byte, 12>,
      std::span<const std::byte, 0x5c>, std::span<const std::byte, 12>)> contact_effect;
  // Actual FBBE0 ABI: optional contact point input, normal input and adjusted
  // point output. In this measured branch the first two use cast+8/+28h.
  std::function<void(std::uint32_t, std::span<const std::byte, 12>,
      std::span<const std::byte, 12>, std::span<std::byte, 12>)> contact_transform;
  std::function<void(std::uint32_t, std::span<const std::byte, 12>,
      std::span<const std::byte, 8>)> root_transform;
  std::function<void(std::uint32_t, std::uint32_t)> network_dirty;
};
struct SourceItemMotionStore {
  std::uint32_t instruction;
  std::size_t offset;
  std::vector<std::byte> before, after;
  friend bool operator==(const SourceItemMotionStore &,
                         const SourceItemMotionStore &) = default;
};
using SourceItemMotionObserver = std::function<void(const SourceItemMotionStore &)>;

// Measured unparented first-item F9B10/F9EA0 resting/miss/floor-contact paths.
// Transform, terrain/world and contact-effect producers remain explicit.
// Unsupported impulses, queue/physical motion, bounce, rotation and timer
// branches fail when reached. F9B10 defines AL; F9EA0 has no promoted return.
bool source_item_update(SourceItemMotionStorage, const SourceItemMotionInputs &,
    const SourceItemMotionServices &, const SourceItemMotionObserver & = {});
void source_item_motion(SourceItemMotionStorage, const SourceItemMotionInputs &,
    const SourceItemMotionServices &, const SourceItemMotionObserver & = {});
} // namespace halo2
