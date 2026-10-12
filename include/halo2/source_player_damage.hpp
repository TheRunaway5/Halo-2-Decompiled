#pragma once
#include <cstdint>
#include <functional>

namespace halo2 {
// The raw fields consumed by whole Xbox15F520. Their initialization,
// configuration, spawn reset and pre-world countdown have separate owners.
struct SourcePlayerDamageState {
  std::uint8_t strength{}; // player+C1
  std::int8_t membership=-1; // player+C0
  std::int32_t field_194{};
};
struct SourcePlayerDamageServices {
  std::function<SourcePlayerDamageState(std::uint32_t)> player;
  std::function<bool(std::uint32_t,std::int32_t)> trait; // selected engine+8C
  std::function<bool(std::int16_t,std::int16_t)> enemies; // selected engine+6C
};
// Whole15DEC0/15F520/160900, after the real target-player resolver and
// before vitality/material damage. Preserves early gates and SSE operation
// order; flags are the actual variant+48 word, including friendly fire bit7.
float source_player_damage_scale(bool selected_engine,std::uint32_t flags,
    std::uint32_t attacker,std::uint32_t victim,const SourcePlayerDamageServices&);
}
