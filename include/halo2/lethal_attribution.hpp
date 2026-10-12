#pragma once
#include "halo2/damage_history.hpp"
#include <optional>
#include <span>
namespace halo2 {
struct AttributionPlayer {
  std::uint32_t datum = 0xffffffffu;
  std::int8_t membership = -1;
};
struct LethalAttributionInput {
  std::uint32_t tick{}, incoming_player = 0xffffffffu,
      victim_player = 0xffffffffu, result_flags{};
  std::int16_t clock_rate = 30, incoming_designator = -1;
  std::int8_t victim_membership = -1;
  std::uint8_t cause{}, mode{}, unit_type{};
  bool engine_present{}, statistics_valid{};
};
struct LethalAttribution {
  std::uint32_t credited_player = 0xffffffffu;
  std::uint8_t cause{};
  bool relationship{}, credited_kill{}, nonenemy_credit{}, victim_death{}, suicide{};
  std::array<std::uint32_t, 4> assists{}, nonenemy_history{};
  unsigned assist_count{}, nonenemy_history_count{};
};
// TU DAC40 -> 1ED960: ordinary Slayer credit and assist selection from PREVIOUS
// history. The live-player span is a full-salt checked pool lookup boundary.
// Membership is the source player+C0 byte: FFA designator or session team.
// Vehicle/mount extra assists, writer services and stat mutation are separate.
std::optional<LethalAttribution> lethal_attribution(
    const LethalAttributionInput &, const DamageHistory &,
    std::span<const AttributionPlayer> live_players);
}
