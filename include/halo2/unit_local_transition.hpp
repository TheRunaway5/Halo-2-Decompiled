#pragma once
#include "halo2/unit_constructor_tail.hpp"
#include <array>
#include <functional>

namespace halo2 {
struct SourceLocalTransitionClock {
  // Literal clock12 bytes at518444. +2 is signed16 frequency; +4 dt bits.
  // Floating controls are caller-owned context, not executable defaults.
  std::array<std::byte, 12> record;
  std::uint16_t fpcw;
  std::uint32_t mxcsr;
};
using SourceLocalTransitionClockProvider =
    std::function<SourceLocalTransitionClock()>;
enum class UnitLocalTransitionBuffer { Body, Headers, Manager, Saved, Counter };
struct UnitLocalTransitionStore {
  std::uint32_t instruction;
  UnitLocalTransitionBuffer buffer;
  std::size_t offset;
  unsigned bytes;
  std::uint32_t before, after;
  friend bool operator==(const UnitLocalTransitionStore &,
                         const UnitLocalTransitionStore &) = default;
};
using UnitLocalTransitionObserver =
    std::function<void(const UnitLocalTransitionStore &)>;
struct SourceUnitLocalTransitionServices {
  UnitConstructorObjectPool objects;
  UnitConstructorTailMetadata tags;
  std::uint32_t one_bits, zero_bits;
  SourceLocalTransitionClockProvider clock;
};
// Complete1DCE20/1DCC30. Counter is current8/duration8/curve8/flags8.
// No guessed clock or integer-conversion mode is supplied.
std::uint32_t
source_transition_remaining_seconds(std::span<const std::byte> counter,
                                    const SourceLocalTransitionClockProvider &);
void start_source_transition_counter(
    std::span<std::byte> counter, std::uint32_t seconds_bits,
    std::uint8_t direction, std::uint32_t one_bits,
    const SourceLocalTransitionClockProvider &,
    const UnitLocalTransitionObserver &observer = {});
// Complete1CEFE0. Copies actual current32-byte locals after admission, then
// starts the actual manager+64 counter. Preserves curve and every other byte.
bool save_source_unit_local_transition(
    std::span<std::byte> manager, std::int32_t node_count,
    std::uint32_t seconds_bits, std::span<std::byte> saved_locals,
    std::span<const std::byte> current_locals,
    const SourceUnitLocalTransitionServices &,
    const UnitLocalTransitionObserver &observer = {});
// CompleteBB240, using source signed +10E/+110/+112/+12A fields. An admitted
// object request marks B8250 even if1CEFE0 returns false. Body allocation,
// node/pose construction, clock scheduling and graph admission remain owners.
void capture_source_object_transition(
    std::uint32_t unit, std::uint32_t seconds_bits,
    const SourceUnitLocalTransitionServices &,
    const UnitLocalTransitionObserver &observer = {});
} // namespace halo2
