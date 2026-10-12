#pragma once
#include <cstdint>
#include <optional>
#include <span>
namespace halo2 {
struct OwnerCounterState {
  std::int16_t count{};
  std::uint32_t last_tick = 0xffffffffu;
};
struct OwnerCounterPlayer {
  std::uint32_t datum = 0xffffffffu, current_unit = 0xffffffffu;
};
struct OwnerCounterObject {
  // Actual salted HEADER identity, not a retained stale reference. A current
  // or forwarding link is resolved by index; incoming fallback is full checked.
  std::uint32_t datum = 0xffffffffu, player = 0xffffffffu,
      forward_9 = 0xffffffffu, forward_other = 0xffffffffu;
  std::uint8_t type{};
  bool body_present{}, dead{};
  OwnerCounterState counter;
};
struct OwnerCounterInput {
  std::uint32_t tick{}, incoming_player = 0xffffffffu, incoming_object = 0xffffffffu;
  float seconds_per_tick{};
  std::int16_t damage_class{}, incoming_designator = -1, victim_membership = -1;
  std::uint8_t lethal{}, enabled_cap{};
};
// TU CBA60 tail only, after the victim history write and before owner detach.
// Caller supplies actual allocated headers/bodies and valid binding invariants.
// The returned index identifies the mutated object; source return EAX is unused.
std::optional<std::size_t> incoming_owner_counter(const OwnerCounterInput &,
    std::span<const OwnerCounterPlayer>, std::span<OwnerCounterObject>);
}
