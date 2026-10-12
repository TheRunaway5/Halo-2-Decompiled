#include "halo2/spawn.hpp"
#include <bit>
#include <cmath>
#include <algorithm>
namespace halo2 {
std::uint16_t source_session_team_mask(std::uint16_t present,
    std::int32_t maximum, std::span<const std::int8_t> memberships) {
  std::uint16_t teams = 0;
  unsigned count = 0;
  for (auto team : memberships) {
    if (team == -1) continue;
    const auto bit = std::uint16_t(1u << (std::uint8_t(team) & 31u));
    if (!(teams & bit)) { teams |= bit; ++count; }
  }
  if (!maximum) maximum = 8;
  int available = 0;
  for (unsigned team = 0; team < 8; ++team) available += bool(present & (1u << team));
  maximum = std::min(maximum, available);
  if (count > 0 && int(count) < maximum)
    for (unsigned team = 0; team < 8; ++team)
      if (!(teams & (1u << team))) {
        teams |= std::uint16_t(1u << team);
        if (int(++count) == maximum) break;
      }
  return teams;
}
SourceTeamMapping source_team_mapping(std::uint16_t present,
    std::uint16_t session, std::int32_t rotation) {
  if (rotation < -1 || rotation > 8) throw FormatError("Unrecovered engine team rotation");
  SourceTeamMapping result;
  unsigned team = rotation == -1 ? 0u : unsigned(rotation);
  for (unsigned count = 0; count < 8; ++count, ++team) {
    if (team == 8) team = 0;
    if (!(session & (1u << team))) continue;
    for (unsigned designator = 0; designator < 9; ++designator)
      if ((present & (1u << designator)) && !(result.assigned & (1u << designator))) {
        result.assigned |= std::uint16_t(1u << designator);
        result.teams |= std::uint16_t(1u << team);
        result.designators[designator] = std::int16_t(team);
        break;
      }
  }
  return result;
}
std::vector<SpawnInfluence> source_spawn_zone_bank(std::span<const SourceSpawnZone> zones,
    unsigned engine, bool team_mode, std::int8_t membership,
    const SourceTeamMapping &mapping, const std::function<bool(const SourceSpawnZone &)> &accepts) {
  unsigned mode_bit;
  switch (engine) {
    case 1: case 9: mode_bit = 3; break;
    case 2: mode_bit = 0; break;
    case 3: mode_bit = 1; break;
    case 4: mode_bit = 2; break;
    case 7: mode_bit = 6; break;
    case 8: mode_bit = 7; break;
    default: throw FormatError("Spawn zone filtering requires a recovered source game engine");
  }
  std::vector<SpawnInfluence> result;
  for (const auto &zone : zones) {
    if (team_mode && membership != -1) {
      bool admitted = false;
      for (unsigned d = 0; d < 9; ++d)
        admitted |= (mapping.assigned & (1u << d)) && (zone.teams & (1u << d)) &&
                    mapping.designators[d] == membership;
      if (!admitted) continue;
    }
    if (zone.modes && !(zone.modes & (1u << mode_bit))) continue;
    if (accepts(zone) && result.size() < 128) result.push_back(zone.influence);
  }
  return result;
}
void initialize_respawn_timer(RespawnTimer &state, std::uint32_t clock,
    std::uint32_t engine_start, std::int32_t interval_ticks,
    std::int32_t one_second_ticks, std::int32_t suicide_ticks) {
  state.death_elapsed = std::max(std::bit_cast<std::int32_t>(clock-engine_start), 0);
  const auto penalty = std::uint32_t(state.penalty) + std::uint32_t(suicide_ticks);
  const auto total = std::bit_cast<std::int32_t>(penalty + std::uint32_t(interval_ticks));
  state.countdown = std::max(one_second_ticks, total);
  state.penalty = 0;
}
SourceTeamPresence source_team_presence(std::uint16_t previous,
    std::span<const TeamRespawnPlayer> players) {
  SourceTeamPresence result{0,previous};
  for(const auto &player:players) {
    if(player.membership == -1) continue;
    const auto bit=std::uint16_t(1u << (std::uint8_t(player.membership)&31u));
    result.retained |= bit;
    if(!(player.flags&2u)) result.playing |= bit;
  }
  return result;
}
void initialize_team_respawn_timer(RespawnTimer &state, std::int8_t member,
    std::span<const TeamRespawnPlayer> players, std::uint32_t clock, std::uint32_t start,
    std::int32_t interval, std::int32_t second, std::int32_t suicide, std::int32_t modifier) {
  const auto penalty = std::bit_cast<std::int32_t>(std::uint32_t(state.penalty) + std::uint32_t(suicide));
  const auto minimum = std::max(second, penalty);
  initialize_respawn_timer(state, clock, start, interval, second, suicide);
  if (modifier == 0) {
    for (const auto &player : players)
      if (!(player.flags & 2u) && player.membership == member &&
          player.countdown > 0 && player.countdown >= minimum && player.countdown < state.countdown)
        state.countdown = player.countdown;
  } else if (modifier == 1) {
    if (interval <= 0) throw FormatError("Source cycling respawn requires a positive interval");
    const auto elapsed = std::max(std::bit_cast<std::int32_t>(clock - start), 0);
    // Source ordinary session times and supported option intervals fit i32.
    // Reject an overflowed cycle instead of introducing a guessed rollover.
    std::int64_t next = (std::int64_t(elapsed) / interval + 1) * interval;
    const auto earliest = std::int64_t(elapsed) + minimum;
    if (next < earliest) next += ((earliest - next + interval - 1) / interval) * interval;
    if (next > 2147483647) throw FormatError("Source cycling respawn overflow is unrecovered");
    state.countdown = std::int32_t(next - elapsed);
  } else if (modifier != 2) throw FormatError("Unknown source team respawn modifier");
}
RespawnEligibility tick_respawn_timer(RespawnTimer &state, std::uint32_t datum,
    std::uint32_t clock, std::uint32_t engine_start, int rate,
    bool engine_present, bool player_allowed, bool playing_phase) {
  if (!engine_present) return {true, 0};
  if (!player_allowed || !playing_phase) return {};
  if (rate <= 0) throw FormatError("Invalid original respawn tick rate");
  unsigned notice = 0;
  if (state.countdown > 0) {
    if (state.countdown == rate*3 || state.countdown == rate*2 || state.countdown == rate)
      notice = 20;
    else if (state.countdown == 1) notice = 21;
    --state.countdown;
    state.displayed_seconds = std::clamp(state.countdown/rate, 0, 1023);
  }
  if (state.countdown != 0) return {false, notice};
  const auto elapsed = std::bit_cast<std::int32_t>(clock-engine_start);
  return {elapsed < 4 || (std::uint32_t(elapsed)&31u) == ((datum&0xffffu)&31u), notice};
}
void tick_spawn_history(SpawnHistory &history) {
  if (!history.initial_gate) return;
  const int remaining = int(history.remaining) - 1;
  history.remaining = std::int16_t(remaining > 0 ? remaining : 0);
  if (!history.remaining) {
    history.active = 0;
    history.initial_gate = 0;
  }
}
float spawn_influence_score(Vec3 point, std::span<const SpawnInfluence> list) {
  float total = 0;
  for (const auto &influence : list) {
    const float dx = point.x - influence.position.x;
    const float dy = point.y - influence.position.y;
    const float dz = point.z - influence.position.z;
    const float xx = dx * dx, yy = dy * dy;
    const float distance_squared = xx + yy;
    const float outer_squared = influence.outer * influence.outer;
    if (!(outer_squared > distance_squared) || !(influence.upper >= dz) ||
        !(dz >= -influence.lower)) continue;
    const float inner_squared = influence.inner * influence.inner;
    float value = influence.weight;
    if (!(inner_squared > distance_squared) && !(influence.inner >= influence.outer)) {
      const double distance = std::sqrt(double(distance_squared));
      const double from_inner = distance - double(influence.inner);
      const double width = double(influence.outer) - double(influence.inner);
      const double fraction = from_inner / width;
      const double weight = (1.0 - fraction) * double(influence.weight);
      value = float(weight);
    }
    total += value;
  }
  return total;
}
float spawn_candidate_score(float score, float bias, std::uint32_t &seed) {
  seed = seed * 0x19660du + 0x3c6ef35fu;
  const auto reciprocal = std::bit_cast<float>(std::uint32_t(0x37800080));
  const double random = double(seed >> 16) * double(reciprocal);
  const double weighted = random * double(bias);
  return float(double(score) + weighted);
}
std::uint32_t spawn_private_seed(std::uint32_t datum, std::uint32_t seed,
    std::uint32_t salt, std::int16_t count) {
  return ((1u << ((datum & 0xffff) & 31)) ^ seed ^ salt) + std::uint32_t(std::int32_t(count));
}
SpawnChoice choose_spawn(std::span<const Vec3> starts,
    std::span<const SpawnInfluence> influences, float bias, std::uint32_t &seed,
    const std::function<bool(std::size_t, Vec3)> &shape_allowed) {
  SpawnChoice choice;
  for (std::size_t i = 0; i < starts.size(); ++i) {
    const float score = spawn_candidate_score(spawn_influence_score(starts[i], influences), bias, seed);
    if (score > choice.score && shape_allowed(i, starts[i])) {
      choice = {int(i), score};
    }
  }
  return choice;
}
}
extern "C" {
void halo2_initialize_respawn_timer(halo2::RespawnTimer *state, std::uint32_t clock,
    std::uint32_t engine_start, std::int32_t interval, std::int32_t second,
    std::int32_t suicide) {
  halo2::initialize_respawn_timer(*state, clock, engine_start, interval, second, suicide);
}
int halo2_tick_respawn_timer(halo2::RespawnTimer *state, std::uint32_t datum,
    std::uint32_t clock, std::uint32_t start, int rate, int engine,
    int player_allowed, int phase, unsigned *notice) {
  const auto result = halo2::tick_respawn_timer(*state, datum, clock, start,
      rate, engine, player_allowed, phase);
  *notice = result.notice;
  return result.eligible;
}
void halo2_tick_spawn_history(halo2::SpawnHistory *history) {
  halo2::tick_spawn_history(*history);
}
float halo2_spawn_influence_score(const halo2::Vec3 *point,
    const halo2::SpawnInfluence *list, unsigned count) {
  return halo2::spawn_influence_score(*point, {list, count});
}
float halo2_spawn_candidate_score(float sum, float bias, std::uint32_t *seed) {
  return halo2::spawn_candidate_score(sum, bias, *seed);
}
std::uint32_t halo2_spawn_private_seed(std::uint32_t datum, std::uint32_t seed,
    std::uint32_t salt, std::int16_t count) {
  return halo2::spawn_private_seed(datum, seed, salt, count);
}
int halo2_choose_spawn(const halo2::Vec3 *starts, unsigned count,
    const halo2::SpawnInfluence *influences, unsigned influence_count, float bias,
    std::uint32_t *seed, const std::uint8_t *allowed, float *score) {
  const auto choice = halo2::choose_spawn({starts, count}, {influences, influence_count}, bias,
      *seed, [&](std::size_t i, halo2::Vec3) { return allowed[i] != 0; });
  *score = choice.score;
  return choice.index;
}
}
