#pragma once
#include "halo2/cache.hpp"
#include <functional>
#include <span>
namespace halo2 {
struct SpawnInfluence {
  Vec3 position;
  float inner{}, outer{}, upper{}, lower{}, weight{};
};
static_assert(sizeof(SpawnInfluence) == 0x20);
struct SpawnType { float inner{}, outer{}, weight{}; };
struct SourceSpawnZone {
  std::uint32_t teams{}, modes{};
  SpawnInfluence influence;
};
struct SourceTeamMapping {
  std::uint16_t assigned{}, teams{};
  std::array<std::int16_t, 9> designators{-1,-1,-1,-1,-1,-1,-1,-1,-1};
};
// Retail157790/TU158EE0, explicit valid session membership bytes. The empty
// roster stays empty; padding begins only after a real team is present.
std::uint16_t source_session_team_mask(std::uint16_t present,
    std::int32_t maximum, std::span<const std::int8_t> memberships);
// Retail157670/TU158DC0. Rotation is the original engine slot26 result;
// ordinary Slayer returns NONE. This does not allocate or balance players.
SourceTeamMapping source_team_mapping(std::uint16_t present,
    std::uint16_t session_teams, std::int32_t rotation = -1);
// Retail23B170/TU23D270. Objective-specific slot33 admission remains the
// explicit callback. Ordinary Slayer's unchanged slot33 always returns true.
std::vector<SpawnInfluence> source_spawn_zone_bank(std::span<const SourceSpawnZone>,
    unsigned source_engine, bool team_mode, std::int8_t membership,
    const SourceTeamMapping &, const std::function<bool(const SourceSpawnZone &)> &);
struct SpawnSettings {
  float upper{}, lower{}, random_bias{};
  std::array<SpawnType, 12> types{};
  std::vector<SpawnInfluence> initial_zones, respawn_zones;
  // All authored rows in source order, including non-Slayer modes/team masks.
  std::vector<SourceSpawnZone> source_initial_zones, source_respawn_zones;
};
float spawn_influence_score(Vec3, std::span<const SpawnInfluence>);
float spawn_candidate_score(float influences, float random_bias, std::uint32_t &seed);
std::uint32_t spawn_private_seed(std::uint32_t player_datum,
    std::uint32_t option_seed, std::uint32_t option_salt, std::int16_t spawn_count);
struct SpawnChoice { int index = -1; float score = -1000; };
struct SpawnHistory {
  std::uint8_t active{};
  Vec3 position;
  std::int16_t initial_gate{}, remaining{};
};
// Base162A30/TU1640D0. Call after the source player spawn/update phase.
// Capture, current-unit ownership and valid-player iteration are separate.
void tick_spawn_history(SpawnHistory &);
struct RespawnTimer {
  std::int32_t countdown{}, penalty{}, displayed_seconds{}, death_elapsed{};
};
// TU15E4D0's FFA countdown writer. Converted option ticks are caller inputs;
// history capture, unit release and identity-gated attacker penalties are separate.
void initialize_respawn_timer(RespawnTimer &, std::uint32_t clock,
    std::uint32_t engine_start, std::int32_t interval_ticks,
    std::int32_t one_second_ticks, std::int32_t suicide_ticks);
struct TeamRespawnPlayer {
  std::int8_t membership{-1};
  std::uint16_t flags{};
  std::int32_t countdown{};
};
struct SourceTeamPresence { std::uint16_t playing{}, retained{}; };
// Retail158140/TU1597E0: current unsuppressed teams and sticky presence of
// every connected player, including suppressed players. The span supplies
// the original connected bitmap; membership NONE contributes no team.
SourceTeamPresence source_team_presence(std::uint16_t previous_retained,
    std::span<const TeamRespawnPlayer>);
// Original15E4D0 ->15ED20 countdown core. Converted tick values and already
// selected credit/penalty are caller inputs; marker/event/life writers follow.
void initialize_team_respawn_timer(RespawnTimer &, std::int8_t membership,
    std::span<const TeamRespawnPlayer>, std::uint32_t clock, std::uint32_t engine_start,
    std::int32_t interval_ticks, std::int32_t one_second_ticks,
    std::int32_t suicide_ticks, std::int32_t modifier);
struct RespawnEligibility { bool eligible{}; unsigned notice{}; };
// TU15F3A0. Invoke only for source players with current unit NONE, before world
// updates. Notice is the original subtype 20/21 (zero means no notice).
RespawnEligibility tick_respawn_timer(RespawnTimer &, std::uint32_t datum,
    std::uint32_t clock, std::uint32_t engine_start, int tick_rate,
    bool engine_present = true, bool player_allowed = true,
    bool playing_phase = true);
// Every authored point consumes a draw. The supplied shape service is queried
// only for a strictly better score; the chooser invents no fallback location.
SpawnChoice choose_spawn(std::span<const Vec3>, std::span<const SpawnInfluence>,
    float random_bias, std::uint32_t &seed,
    const std::function<bool(std::size_t, Vec3)> &shape_allowed);
}
