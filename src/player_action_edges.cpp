#include "halo2/player_action_edges.hpp"
#include <cmath>
#include <stdexcept>

namespace halo2 {
void source_player_action_edges(SourcePlayerActionEdges &p, std::uint32_t &gameplay,
                                std::uint16_t &actions) {
  const auto incoming=gameplay;
  gameplay&=~p.previous_gameplay;
  p.previous_gameplay=incoming&0x16100074u;
  const bool primary=actions&1u, secondary=actions&0x10u;
  const bool old_primary=p.previous_buttons&1u, old_secondary=p.previous_buttons&0x10u;
  actions=std::uint16_t((actions&~0xccu) |
      (primary&&!old_primary?4u:0u) | (!primary&&old_primary?8u:0u) |
      (secondary&&!old_secondary?0x40u:0u) | (!secondary&&old_secondary?0x80u:0u));
  p.previous_buttons=std::uint16_t((p.previous_buttons&~0x11u) |
      (primary?1u:0u) | (secondary?0x10u:0u));
  if(p.flags&0x10u) {
    actions&=0xfff0u;
    if(!primary)p.flags&=0xffefu;
  }
  if(p.flags&0x20u) {
    actions&=0xff0fu;
    if(!secondary)p.flags&=0xffdfu;
  }
}
std::int32_t source_pickup_hold_threshold(float seconds) {
  const float milliseconds=seconds*1000.f;
  if(!std::isfinite(milliseconds) || milliseconds<0 || milliseconds>=2147483648.f)
    throw std::runtime_error("Invalid authored pickup hold threshold");
  return std::int32_t(milliseconds);
}
std::uint16_t source_pickup_buttons(bool primary,std::uint16_t primary_ms,
    bool secondary,std::uint16_t secondary_ms,std::int32_t threshold) {
  // Source threshold tests do not also test the down bit. Device polling
  // supplies zero duration on release; preserve that separate ownership.
  return std::uint16_t((primary?1u:0u) | (primary_ms>=threshold?2u:0u) |
      (secondary?0x10u:0u) | (secondary_ms>=threshold?0x20u:0u));
}
} // namespace halo2
