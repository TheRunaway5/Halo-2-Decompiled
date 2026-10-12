#pragma once
#include <cstdint>
#include <span>

namespace halo2 {
// Original player+1AC signed life count. Zero configured lives initializes
// NONE; other32-bit inputs narrow only at the source's signed-short store.
void source_initialize_player_lives(std::int16_t&,std::int32_t configured,
    bool engine_present,std::uint8_t raw_mode);
// Original15CE70/15E4D0/15F8B0 death prefix. This does not own death
// attribution, respawn timers, statistics, replication notifications or rounds.
void source_player_life_death(std::int16_t&,std::int32_t game_type,
    std::uint8_t raw_mode,bool engine_present,std::int16_t lifecycle,std::int32_t status);
bool source_player_lives_exhausted(std::int32_t configured,std::uint32_t current_unit,
    std::int16_t remaining);
bool source_player_respawn_allowed(std::int32_t configured,std::uint16_t flags,
    std::int8_t membership,std::uint32_t current_unit,std::int16_t remaining);
struct SourcePlayerLifeParticipant {
  std::uint16_t flags;
  std::int8_t membership;
  std::uint32_t current_unit;
  std::int16_t remaining;
};
// Supplied span is the occupied player bitmap; original iterator excludes
// flag2, including when its remaining count or unit would otherwise qualify.
bool source_team_lives_exhausted(std::int32_t configured,bool engine_present,
    bool team_mode,std::int32_t team,std::span<const SourcePlayerLifeParticipant>);
} // namespace halo2
