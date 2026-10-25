#pragma once
// Native ports of the TU1.5 Xbox matchmaking progress screen (mainmenu
// screen210, c_matchmaking_screen) and the start-match countdown that feeds
// it. Evidence: multiplayer/docs/halo2-matchmaking-progress-evidence.md and
// tools/reference/halo2-decompiled/src/{unknown_250155,unknown_19987f,
// unknown_058dd0,unknown_058ee0,user_interface_text_parser}.cpp.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

namespace halo2 {
// 0x199D7C: the session state as the user interface shows it. The values are
// the original UI states; c_matchmaking_screen::get_title (0x251BFF) and the
// pane selector (0x25142F) are keyed on them.
enum class MatchmakingProgressUi : int {
  none = 0,        // no matchmaking phase
  failed = 1,      // match mode1
  cancelling = 2,  // match mode4
  searching = 3,   // phase1, party A modes6/7
  waiting = 4,     // phase2, gathering (party A mode10..13 without live B)
  joining = 5,     // phase3, party A modes8/9: attempting to join a game
  starting = 6,    // phase4, live game session B
  start_match = 7, // manager state7, c_session_state_start_match
  have_fun = 8,    // match mode2, the game is starting
};

// 0x59590 returns NONE unless manager state7 has established B's countdown.
inline constexpr std::int32_t matchmaking_countdown_none = -1;
// c_session_state_start_match::update (0x72700) waits this long once every
// game-session peer is ready, then sets B's mode4.
inline constexpr std::int32_t matchmaking_start_countdown_ms = 10000;
// 0x25217B refuses Cancel this long after the screen reaches pane2 or later.
inline constexpr std::uint32_t matchmaking_cancel_lock_ms = 60000;

struct MatchmakingStartCountdown {
  std::int32_t seconds = matchmaking_countdown_none;
  bool ready{};
};
// 0x72700: remaining=max(0,10000-elapsed); network_session_start_countdown
// publishes (remaining+999)/1000 whole seconds; ready exactly when remaining
// reaches zero.
inline MatchmakingStartCountdown matchmaking_start_countdown(std::uint32_t elapsed_ms) {
  std::int64_t remaining = std::int64_t(matchmaking_start_countdown_ms) - std::int64_t(elapsed_ms);
  if (remaining < 0) remaining = 0;
  return {std::int32_t((remaining + 999) / 1000), remaining == 0};
}

// 0x25142F: the authored pane for a UI state. The countdown argument is
// 0x59590's value, NONE until B's countdown exists.
inline unsigned matchmaking_progress_pane(MatchmakingProgressUi ui, std::int32_t countdown) {
  switch (ui) {
  case MatchmakingProgressUi::waiting: return 1;
  case MatchmakingProgressUi::starting: return 2;
  case MatchmakingProgressUi::start_match:
    return countdown != matchmaking_countdown_none ? 3u : 2u;
  case MatchmakingProgressUi::have_fun: return 3;
  default: return 0;
  }
}

// The PC (Vista/MCC) screen210 authors only the first three panes. Its raw
// join state shows pane2 (Vista 0x646A82), so the Xbox joining state uses that
// pane there. TU1.5 pane3 is drawn on the PC pane2 frame by the caller.
inline unsigned matchmaking_progress_pc_pane(MatchmakingProgressUi ui, std::int32_t countdown) {
  if (ui == MatchmakingProgressUi::joining) return 2;
  return std::min(matchmaking_progress_pane(ui, countdown), 2u);
}

struct MatchmakingProgressTitle {
  std::uint32_t xbox{}, pc{};
};
// 0x251BFF get_title, with the PC locale's equivalent string IDs (the PC
// screen210 list shifts the same records down by two).
inline std::optional<MatchmakingProgressTitle> matchmaking_progress_title(MatchmakingProgressUi ui) {
  switch (ui) {
  case MatchmakingProgressUi::none: return MatchmakingProgressTitle{0x1400043a, 0x14000438};
  case MatchmakingProgressUi::failed: return MatchmakingProgressTitle{0x1200043b, 0x12000439};
  case MatchmakingProgressUi::cancelling: return MatchmakingProgressTitle{0x1300043c, 0x1300043a};
  case MatchmakingProgressUi::searching: return MatchmakingProgressTitle{0x1300043d, 0x1300043b};
  case MatchmakingProgressUi::waiting: return MatchmakingProgressTitle{0x0e00043e, 0x0e00043c};
  case MatchmakingProgressUi::joining: return MatchmakingProgressTitle{0x0c00043f, 0x0c00043d};
  case MatchmakingProgressUi::starting:
  case MatchmakingProgressUi::start_match: return MatchmakingProgressTitle{0x0d000440, 0x0d00043e};
  case MatchmakingProgressUi::have_fun: return MatchmakingProgressTitle{0x16000441, 0x1600043f};
  }
  return std::nullopt;
}

// Pane3 (0x251D84) authored strings: text0 "Game starts in <countdown>",
// text2 "<variant>", text3 "on <map>", text10 "<count> player(s) total".
inline constexpr std::uint32_t matchmaking_game_starts_xbox = 0x0f00188e, matchmaking_game_starts_pc = 0x0f001afc;
inline constexpr std::uint32_t matchmaking_variant_xbox = 0x0f0001df, matchmaking_variant_pc = 0x0f0001e0;
inline constexpr std::uint32_t matchmaking_on_map_xbox = 0x0e0001e1, matchmaking_on_map_pc = 0x0e0001e2;
inline constexpr std::uint32_t matchmaking_players_total_xbox = 0x0d001829, matchmaking_players_total_pc = 0x0d001a99;
// Xbox token E45B / PC token E478 in "Game starts in ".
inline constexpr char32_t matchmaking_countdown_token_xbox = 0xe45b, matchmaking_countdown_token_pc = 0xe478;

// 0x22D775 parse_matchmaking_countdown.
inline std::string matchmaking_countdown_text(std::int32_t seconds) {
  if (seconds == matchmaking_countdown_none) return ":--";
  char buffer[16];
  std::snprintf(buffer, sizeof buffer, ":%02d", int(seconds));
  return buffer;
}

// 0x25217B: Cancel opens the leave-matchmaking choice only before pane2, or
// once the pane has been current for a minute; otherwise UI sound2 (error).
inline bool matchmaking_progress_cancel_allowed(unsigned pane, std::uint32_t pane_elapsed_ms) {
  return pane < 2 || pane_elapsed_ms >= matchmaking_cancel_lock_ms;
}
// c_matchmaking_screen::v10 (0x251AFA) consumes button3 at pane2 or later.
inline bool matchmaking_progress_consumes_x(unsigned pane) { return pane >= 2; }

// 0x2514FD pane3 roster order over game-session B. `ranked` is 0x19A0C5
// (hopper types 2/4/5 display rank), `teams` is 0x19A127 (types 3/4/5).
// Comparators 0x251384 (team, then larger value first), 0x2513FC (larger
// value first) and 0x2513C9 (team). qsort is not stable; a stable sort keeps
// the service's admitted order among equal keys deterministically.
struct MatchmakingProgressPlayer {
  int team = -1, value = -1;
};
inline std::vector<unsigned> matchmaking_progress_order(const std::vector<MatchmakingProgressPlayer> &players,
                                                        bool ranked, bool teams) {
  std::vector<unsigned> order(players.size());
  for (unsigned i = 0; i < order.size(); ++i) order[i] = i;
  auto by_team = [&](unsigned a, unsigned b) { return players[a].team < players[b].team; };
  auto by_value = [&](unsigned a, unsigned b) { return players[a].value > players[b].value; };
  if (ranked && teams)
    std::stable_sort(order.begin(), order.end(), [&](unsigned a, unsigned b) {
      if (players[a].team != players[b].team) return by_team(a, b);
      return by_value(a, b);
    });
  else if (ranked)
    std::stable_sort(order.begin(), order.end(), by_value);
  else if (teams)
    std::stable_sort(order.begin(), order.end(), by_team);
  return order;
}
// 0x2517E2 colors a row by team only while the countdown exists in a team
// hopper; team NONE uses the default (no-team) color.
inline bool matchmaking_progress_team_colors(std::int32_t countdown, bool teams) {
  return countdown != matchmaking_countdown_none && teams;
}
} // namespace halo2
