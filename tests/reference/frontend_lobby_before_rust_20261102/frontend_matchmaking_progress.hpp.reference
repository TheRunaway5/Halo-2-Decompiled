#pragma once
// Native ports of the TU1.5 Xbox matchmaking progress screen (mainmenu
// screen210, c_matchmaking_screen) and the start-match countdown that feeds
// it. Evidence: multiplayer/docs/halo2-matchmaking-progress-evidence.md and
// tools/reference/halo2-decompiled/src/{unknown_250155,unknown_19987f,
// unknown_058dd0,unknown_058ee0,user_interface_text_parser}.cpp.
// Unqualified addresses below are retail decompiler locators. Independently
// executed TU1.5 controls cover roster 0x25780B and the normal start timer
// 0x76360/0x76481; TU1.5's additional readiness-deferral producers are not
// represented by the replacement service's ordinary connected-peer gate.
#include <algorithm>
#include <bit>
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
// c_session_state_start_match::update (retail0x72700; TU1.5 0x76360,
// normal timer block0x76481) waits this long once every game-session peer
// is ready, then sets B's mode4. This is its ordinary countdown path.
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
// The service owns B's start clock. A native observer converts its published
// remaining milliseconds with 0x72700's same ceiling, and never treats an
// unavailable countdown as elapsed or launch-ready.
inline MatchmakingStartCountdown matchmaking_published_start_countdown(std::optional<unsigned> remaining_ms) {
  if(!remaining_ms || *remaining_ms>unsigned(matchmaking_start_countdown_ms))return {};
  return {std::int32_t((*remaining_ms+999)/1000),*remaining_ms==0};
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

// TU1.5 257E77/257EB7 (retail251B39): a changed pane requests UI
// sound12 exactly when the new pane is3. This is the matched-entry cue;
// the independent lobby stage3..0 sound10 policy is below.
inline std::optional<unsigned> matchmaking_progress_pane_sound(unsigned previous,unsigned pane) {
  return previous!=pane && pane==3 ? std::optional<unsigned>{12} : std::nullopt;
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
// c_matchmaking_screen::v10 (0x251AFA) consumes button3 (Y, the online Y
// menu) at pane2 or later.
inline bool matchmaking_progress_consumes_y(unsigned pane) { return pane >= 2; }
// Leave-matchmaking dialogs: 0x8C names remote party members, 0x8D does not.
inline constexpr unsigned matchmaking_leave_dialog_party = 0x8c, matchmaking_leave_dialog_local = 0x8d;

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

// 0x2514FD selects party A before pane1, and in ranked panes1/2. Only
// pane3 reveals game B's ranked opponents. Datum16 becomes "Matched Player"
// (0x251703); NONE becomes a blank slot (0x251778). `maximum` is available
// only with the original accepted-membership/count-provider gate.
enum class MatchmakingProgressRow { player, matched, empty };
// Screen210 entry values reserved for the two rows that have no player.
inline constexpr int matchmaking_matched_player_row = -2, matchmaking_empty_player_row = -3;
inline constexpr std::uint32_t matchmaking_matched_player_xbox = 0x0e00075e,
                               matchmaking_matched_player_pc = 0x0e00075c;
inline bool matchmaking_progress_uses_party(unsigned pane, bool ranked) {
  return pane != 3 && (pane < 1 || ranked);
}
inline std::vector<MatchmakingProgressRow> matchmaking_progress_rows(
    unsigned pane, bool ranked, unsigned party_players, unsigned game_players,
    std::optional<unsigned> maximum) {
  const bool party = matchmaking_progress_uses_party(pane, ranked);
  const unsigned known = party ? party_players : game_players;
  const unsigned count = party || pane == 1 ? std::max(known, maximum.value_or(0)) : known;
  std::vector<MatchmakingProgressRow> rows;
  rows.reserve(count);
  for (unsigned i = 0; i < count; ++i)
    rows.push_back(i < known ? MatchmakingProgressRow::player :
        i < game_players ? MatchmakingProgressRow::matched : MatchmakingProgressRow::empty);
  return rows;
}

// Matchmaking pregame lobby (screen14 pane2) start countdown.
// 0x25106C -> 0x19A78E(controller,10,3): START GAME on a ready lobby
// (state9) starts a 10 s countdown; another press while it runs (state10)
// shortens it by one second while it is above the 3 s minimum.
inline constexpr std::int32_t matchmaking_lobby_countdown_initial = 10, matchmaking_lobby_countdown_minimum = 3;
inline std::int32_t matchmaking_lobby_countdown_press(std::int32_t current) {
  if (current < 0) return matchmaking_lobby_countdown_initial;
  return current > matchmaking_lobby_countdown_minimum ? current - 1 : current;
}
// c_session_state_pre_game 0x6E360: once a published value has stood for
// 1000 ms it is republished one lower; the clock restarts on every change.
inline bool matchmaking_lobby_countdown_due(std::int32_t current, std::uint32_t elapsed_ms) {
  return current > 0 && std::bit_cast<std::int32_t>(elapsed_ms) >= 1000;
}
// c_screen_24fd74::update_countdown 0x250A8B: UI sound10 on each new stage
// 0..3.
inline bool matchmaking_lobby_countdown_beeps(std::int32_t previous, std::int32_t stage) {
  return stage >= 0 && stage <= 3 && stage != previous;
}
// 0x22D730 parse_countdown (token E413).
inline std::string matchmaking_lobby_countdown_text(std::int32_t seconds) {
  if (seconds < 0) return "-:--";
  char buffer[24];
  std::snprintf(buffer, sizeof buffer, "%d:%02d", int(seconds / 60), int(seconds % 60));
  return buffer;
}
} // namespace halo2
