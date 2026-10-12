#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <optional>
#include <string>
namespace halo2 {
struct SlayerScoreRequest { std::uint32_t player; std::int32_t delta; };
struct SlayerScoreRequests {
  std::array<SlayerScoreRequest, 2> values{};
  unsigned count{};
};
// TU2C2350, ordinary scoring (bonus option bit0 disabled). Attribution and
// the supplied relationship/victim flags belong to the preceding death writer.
SlayerScoreRequests slayer_death_scores(std::uint32_t attacker,
    std::uint32_t victim, bool relationship, std::uint16_t victim_flags,
    bool suicide_loss, bool death_loss);
struct SlayerScoreResult { bool applied{}, end_match{}; };
// TU15CE00, active FFA. Store signed-i16 delta with the source clamp, but test
// the original signed-i32 delta for score-limit completion. Playback is separate.
SlayerScoreResult slayer_score_write(std::int16_t &score, std::int32_t delta,
    std::int32_t limit, bool engine_present = true, bool playing = true,
    bool active = true, bool player_present = true);
struct SlayerRankingPlayer {
  std::uint32_t player;
  std::int16_t score;
  std::uint16_t flags;
  std::uint8_t membership;
};
// TU15C970/241500, FFA winner only. Connected bitmap iteration is supplied by
// the span; suppressed players and membership NONE are excluded. Ties return NONE.
std::uint32_t slayer_ffa_winner(std::span<const SlayerRankingPlayer>);
struct SlayerTeamScoreResult { bool changed{}, end_match{}; };
// TU15CC90 ->1ED7B0. Player scores must already contain the preceding player
// write. Kind0 sums the full i32 delta; kind1/2 keep the minimum/maximum high
// water mark from original1A2BA0's unsuppressed, connected player iterator.
// Other nonzero kinds retain the first eligible player, as the source does.
// This is the score core; session membership and engine/statistic services
// remain caller-owned. Original team score entries have an 18-byte stride.
// The caller admits a valid source team index0..7 before invoking this leaf.
SlayerTeamScoreResult slayer_team_score_write(std::int16_t &score,
    std::int8_t team, std::int32_t delta, std::int32_t limit,
    std::int32_t kind, std::span<const SlayerRankingPlayer>);
// TU15C970/241500, with a nonempty original retained team mask. Ties produce
// NONE. A zero mask returns nullopt: the source reads unwritten ranking data,
// so that absent-team lifecycle has no recovered winner contract.
std::optional<std::uint32_t> slayer_team_winner(
    std::span<const std::int16_t, 8> scores, std::uint8_t retained_team_mask);
// TU163B10 ordinary Slayer remaining-tick branch. Options' seconds-to-ticks
// conversion and round-start ownership are caller inputs; zero means unlimited.
std::int32_t slayer_remaining_ticks(std::uint32_t clock,
    std::uint32_t round_start, std::uint32_t limit_ticks);
// TU15D490 stores this signed-short display clock before the world phase.
// Timed matches count down; unlimited matches count up and cap at 32767.
std::int16_t slayer_clock_seconds(std::uint32_t clock,
    std::uint32_t round_start, std::uint32_t limit_ticks);
// TU1600E0/163280: signed-short clamp, blank zero minutes and two-digit seconds.
std::string slayer_clock_text(std::int16_t seconds);
}
