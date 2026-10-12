#pragma once
#include "halo2/vista_announcer_events.hpp"
#include <functional>

namespace halo2 {
struct SourceScoreEventOptions {
  std::uint32_t score_limit{},player_flags{};
  std::uint8_t raw_mode{};
};
struct SourceScoreEventServices {
  // Actual options pointer is reloaded after the first/second dispatched
  // event. Referenced fields may change during the supplied real consumers.
  std::function<const SourceScoreEventOptions&()> options;
  std::function<bool()> engine_present;
  std::function<std::int8_t(std::uint32_t player)> membership;
  std::function<void(const VistaAnnouncerEvent&)> network,local;
};
// Whole19F470/1A2DD0/1A9080. The two source publication owners remain
// explicit consumers; no missing network/local delivery is successful.
void source_score_changed_event(std::uint32_t player,std::int32_t updated_score,
    const SourceScoreEventServices&);
} // namespace halo2
