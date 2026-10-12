#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace halo2 {
using SourceHudScoreColor = std::array<std::uint32_t, 3>;
struct SourceHudScoreColors {
  bool scenario_present{};
  std::span<const SourceHudScoreColor> appearance; // matg+160/+164, RGB12
  std::span<const SourceHudScoreColor> teams; // first mulg universal+10/+14
};
struct SourceHudScorePlayer {
  std::uint32_t datum = 0xffffffffu;
  std::int8_t membership = -1, primary_color{}; // player+C0/+84
  std::uint16_t flags{};
  std::int16_t score{};
  std::optional<std::int16_t> wins; // statborg player+12, not individual score
};
struct SourceHudScoreInput {
  bool engine_present = true, teams{};
  std::int32_t source_type = 2, limit = 25, round_mode{}, time_limit = 480;
  std::int16_t clock_seconds{};
  std::uint32_t local_player = 0xffffffffu;
  std::uint16_t connected_players{}, playing_teams{};
  std::array<SourceHudScorePlayer, 16> players;
  std::array<std::int16_t, 8> team_scores{};
  std::array<std::optional<std::int16_t>, 8> team_wins;
  std::u16string_view variant_name;
};
enum class SourceHudScoreStatus { Hidden, Ready, UnavailableRoundStatistics, InvalidInput };
struct SourceHudScoreDisplay {
  SourceHudScoreStatus status = SourceHudScoreStatus::Hidden;
  std::int32_t type{}, limit{}, score{}, rival_score{};
  bool teams{}, timer{}, leading{}, rival{};
  std::uint32_t player{}, rival_player{};
  SourceHudScoreColor color{}, rival_color{};
  std::u16string name, timer_text, score_text, rival_text;
  float score_fraction{}, rival_fraction{};
};
// Complete score/rival/text/color portion of retail15F3A0/TU160A40 and its
// owning13992A/TU13B09A HUD conversion. Pool/stat/session inputs are explicit.
// Unit charge and Territories zone progress have separate, unfinished owners.
// A limit of1 selects signed-short wins; absent wins never become fake scores.
SourceHudScoreDisplay source_hud_score_display(const SourceHudScoreInput&,
    const SourceHudScoreColors&);
// Retail13A050 engine-condition bank, ordinary multiplayer without charge or
// objective target. Unknown/hidden score status retains the source zero fields.
std::uint16_t source_hud_score_mask(const SourceHudScoreDisplay&);
} // namespace halo2
