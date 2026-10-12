#include "halo2/slayer.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
namespace halo2 {
SlayerScoreRequests slayer_death_scores(std::uint32_t attacker,
    std::uint32_t victim, bool relationship, std::uint16_t victim_flags,
    bool suicide_loss, bool death_loss) {
  SlayerScoreRequests result;
  if (attacker != 0xffffffffu && !(victim_flags & 2u)) {
    if (!relationship)
      result.values[result.count++] = {attacker, 1};
    else if (attacker != victim || suicide_loss)
      result.values[result.count++] = {attacker, -1};
  }
  if (death_loss)
    result.values[result.count++] = {victim, -1};
  return result;
}
SlayerScoreResult slayer_score_write(std::int16_t &score, std::int32_t delta,
    std::int32_t limit, bool engine_present, bool playing, bool active,
    bool player_present) {
  if (!engine_present || !playing || !active || !player_present) return {};
  const auto short_delta = std::bit_cast<std::int16_t>(std::uint16_t(delta));
  score = std::int16_t(std::clamp(int(score) + int(short_delta), -30000, 30000));
  return {true, delta > 0 && limit != 0 && int(score) >= limit};
}
std::uint32_t slayer_ffa_winner(std::span<const SlayerRankingPlayer> players) {
  std::uint32_t winner = 0xffffffffu;
  int best = -32769;
  bool tied = false;
  for (const auto &p : players) {
    if (p.player == 0xffffffffu || (p.flags & 2u) || p.membership == 0xffu) continue;
    if (p.score > best) { best = p.score; winner = p.player; tied = false; }
    else if (p.score == best) tied = true;
  }
  return tied ? 0xffffffffu : winner;
}
SlayerTeamScoreResult slayer_team_score_write(std::int16_t &score,
    std::int8_t team, std::int32_t delta, std::int32_t limit,
    std::int32_t kind, std::span<const SlayerRankingPlayer> players) {
  const auto previous = std::int32_t(score);
  auto requested = std::numeric_limits<std::int32_t>::min();
  if (!kind) {
    requested = std::bit_cast<std::int32_t>(std::uint32_t(previous) +
        std::uint32_t(delta));
  } else {
    for (const auto &player : players) {
      if (player.player == 0xffffffffu || (player.flags & 2u) ||
          std::bit_cast<std::int8_t>(player.membership) != team) continue;
      if (requested == std::numeric_limits<std::int32_t>::min() ||
          (kind == 1 && requested > player.score) ||
          (kind == 2 && requested <= player.score)) requested = player.score;
    }
    if (requested <= previous) return {};
  }
  if (requested == previous) return {};
  // Original subtract wraps before its signed-short stat writer. The end
  // condition uses the prospective i32 total, rather than this stored value.
  const auto short_delta = std::bit_cast<std::int16_t>(
      std::uint16_t(std::uint32_t(requested) - std::uint32_t(previous)));
  score = std::int16_t(std::clamp(int(score) + int(short_delta), -30000, 30000));
  return {true, requested > previous && limit != 0 && requested >= limit};
}
std::optional<std::uint32_t> slayer_team_winner(
    std::span<const std::int16_t, 8> scores, std::uint8_t mask) {
  if (!mask) return {};
  std::uint32_t winner = 0xffffffffu;
  int best = -32769;
  bool tied = false;
  for (unsigned team = 0; team < 8; ++team) {
    if (!(mask & (1u << team))) continue;
    if (scores[team] > best) { best = scores[team]; winner = team; tied = false; }
    else if (scores[team] == best) tied = true;
  }
  return tied ? 0xffffffffu : winner;
}
std::int32_t slayer_remaining_ticks(std::uint32_t clock,
    std::uint32_t round_start, std::uint32_t limit_ticks) {
  const auto elapsed = std::max(std::bit_cast<std::int32_t>(clock - round_start), 0);
  return std::max(std::bit_cast<std::int32_t>(limit_ticks - std::uint32_t(elapsed)), 0);
}
std::int16_t slayer_clock_seconds(std::uint32_t clock,
    std::uint32_t round_start, std::uint32_t limit_ticks) {
  const auto ticks = limit_ticks ? slayer_remaining_ticks(clock, round_start, limit_ticks)
      : std::max(std::bit_cast<std::int32_t>(clock - round_start), 0);
  // Original CVTSI2SS, MULSS and CVTTSS2SI, with the verified 30 Hz clock.
  const float seconds = float(ticks) * std::bit_cast<float>(0x3d088889u);
  auto integer = !std::isfinite(seconds) || double(seconds) >= 2147483648. ||
          double(seconds) < -2147483648.
      ? std::numeric_limits<std::int32_t>::min() : std::int32_t(seconds);
  if (!limit_ticks) integer = std::min(integer, 32767);
  return std::bit_cast<std::int16_t>(std::uint16_t(integer));
}
std::string slayer_clock_text(std::int16_t value) {
  const int seconds = std::max(int(value), 0);
  const int minutes = seconds / 60, remainder = seconds - minutes * 60;
  return (minutes ? std::to_string(minutes) : " ") + ":" +
      (remainder <= 9 ? "0" : "") + std::to_string(remainder);
}
}
