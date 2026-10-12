#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace halo2 {
enum class SourceLocomotionPath {
  normal_request,
  forced_class_one,
  unavailable_physical_state,
};
struct SourceLocomotionStore {
  std::uint32_t instruction{};
  std::uint16_t offset{};
  std::uint8_t width{};
  std::uint32_t bits{};
};
struct SourceLocomotionUpdate {
  SourceLocomotionPath path{SourceLocomotionPath::normal_request};
  std::array<SourceLocomotionStore,2> stores{};
  std::uint8_t store_count{};
  // Populated only when the bounded wrapper cannot execute the source route.
  // Zero denotes an unverified/out-of-range original table index.
  std::uint32_t unavailable_dispatch_target{};
};

// Literal 1F52F0 packet consumer. Request+DC/E0/E4 are copied control axes;
// this does not produce those controls or derive them from world velocity.
// No-write preserves classification+4 and every other result byte.
// This API reproduces stored outputs, not CPU flags or FP exceptions.
SourceLocomotionUpdate classify_source_locomotion_request(
    std::span<const std::byte,0x154> request,
    std::span<std::byte,0x70> classification);

// Bounded 1E94E0: flag20 first, otherwise actual physical-state byte1.
// Other table routes return unavailable without executing a guessed result.
// The caller must retain that boundary and run the corresponding producer.
SourceLocomotionUpdate dispatch_source_locomotion(
    std::span<const std::byte,0x154> request,
    std::span<const std::byte> physical_state,
    std::span<std::byte,0x70> classification);
} // namespace halo2
