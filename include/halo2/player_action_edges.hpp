#pragma once
#include <cstdint>

namespace halo2 {
// Player+38/+3C/+2, retained across input frames by original150DD0.
struct SourcePlayerActionEdges {
  std::uint32_t previous_gameplay{};
  std::uint16_t previous_buttons{}, flags{};
};
void source_player_action_edges(SourcePlayerActionEdges &, std::uint32_t &gameplay,
                                std::uint16_t &actions);
// Original18841A..188433: scalar binary32 multiply, CVTTSS2SI.
std::int32_t source_pickup_hold_threshold(float authored_seconds);
std::uint16_t source_pickup_buttons(bool primary, std::uint16_t primary_ms,
                                   bool secondary, std::uint16_t secondary_ms,
                                   std::int32_t threshold_ms);
} // namespace halo2
