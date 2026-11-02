#pragma once
// Typed Rust ports of the TU1.5 Xbox matchmaking progress screen (mainmenu
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
MatchmakingStartCountdown matchmaking_start_countdown(std::uint32_t elapsed_ms);
// The service owns B's start clock. A native observer converts its published
// remaining milliseconds with 0x72700's same ceiling, and never treats an
// unavailable countdown as elapsed or launch-ready.
MatchmakingStartCountdown matchmaking_published_start_countdown(std::optional<unsigned> remaining_ms);

// 0x25142F: the authored pane for a UI state. The countdown argument is
// 0x59590's value, NONE until B's countdown exists.
unsigned matchmaking_progress_pane(MatchmakingProgressUi ui, std::int32_t countdown);

// TU1.5 257E77/257EB7 (retail251B39): a changed pane requests UI
// sound12 exactly when the new pane is3. This is the matched-entry cue;
// the independent lobby stage3..0 sound10 policy is below.
std::optional<unsigned> matchmaking_progress_pane_sound(unsigned previous,unsigned pane);

// The PC (Vista/MCC) screen210 authors only the first three panes. Its raw
// join state shows pane2 (Vista 0x646A82), so the Xbox joining state uses that
// pane there. TU1.5 pane3 is drawn on the PC pane2 frame by the caller.
unsigned matchmaking_progress_pc_pane(MatchmakingProgressUi ui, std::int32_t countdown);

struct MatchmakingProgressTitle {
  std::uint32_t xbox{}, pc{};
};
// 0x251BFF get_title, with the PC locale's equivalent string IDs (the PC
// screen210 list shifts the same records down by two).
std::optional<MatchmakingProgressTitle> matchmaking_progress_title(MatchmakingProgressUi ui);

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
// once the signed clock difference is at least a minute; otherwise UI sound2
// (error). TU1.5 2584C2 uses JL, including wrapped negative differences.
bool matchmaking_progress_cancel_allowed(unsigned pane, std::uint32_t pane_elapsed_ms);
// c_matchmaking_screen::v10 (0x251AFA) consumes button3 (Y, the online Y
// menu) at pane2 or later.
bool matchmaking_progress_consumes_y(unsigned pane);
// Leave-matchmaking dialogs: 0x8C names remote party members, 0x8D does not.
inline constexpr unsigned matchmaking_leave_dialog_party = 0x8c, matchmaking_leave_dialog_local = 0x8d;

// 0x2514FD pane3 roster order over game-session B. `ranked` is 0x19A0C5
// (hopper types 2/4/5 display rank), `teams` is 0x19A127 (types 3/4/5).
// Comparators 0x251384 (team, then larger value first), 0x2513FC (larger
// value first) and 0x2513C9 (team). Rust executes the exact TU1.5 CRT
// qsort swaps, including equal keys; the service admitted slot sequence is
// an explicit input boundary, not a recovered original network producer.
struct MatchmakingProgressPlayer {
  int team = -1, value = -1;
};
std::vector<unsigned> matchmaking_progress_order(const std::vector<MatchmakingProgressPlayer> &players,
                                                        bool ranked, bool teams);
// 0x2517E2 colors a row by team only while the countdown exists in a team
// hopper; team NONE uses the default (no-team) color.
bool matchmaking_progress_team_colors(std::int32_t countdown, bool teams);

// 0x2514FD selects party A before pane1, and in ranked panes1/2. Only
// pane3 reveals game B's ranked opponents. Datum16 becomes "Matched Player"
// (0x251703); NONE becomes a blank slot (0x251778). `maximum` is available
// only with the original accepted-membership/count-provider gate.
enum class MatchmakingProgressRow { player, matched, empty };
// Screen210 entry values reserved for the two rows that have no player.
inline constexpr int matchmaking_matched_player_row = -2, matchmaking_empty_player_row = -3;
inline constexpr std::uint32_t matchmaking_matched_player_xbox = 0x0e00075e,
                               matchmaking_matched_player_pc = 0x0e00075c;
bool matchmaking_progress_uses_party(unsigned pane, bool ranked);
std::vector<MatchmakingProgressRow> matchmaking_progress_rows(
    unsigned pane, bool ranked, unsigned party_players, unsigned game_players,
    std::optional<unsigned> maximum);

// Matchmaking pregame lobby (screen14 pane2) start countdown.
// 0x25106C -> 0x19A78E(controller,10,3): START GAME on a ready lobby
// (state9) starts a 10 s countdown; another press while it runs (state10)
// shortens it by one second while it is above the 3 s minimum.
inline constexpr std::int32_t matchmaking_lobby_countdown_initial = 10, matchmaking_lobby_countdown_minimum = 3;
std::int32_t matchmaking_lobby_countdown_press(std::int32_t current);
// c_session_state_pre_game 0x6E360: once a published value has stood for
// 1000 ms it is republished one lower; the clock restarts on every change.
bool matchmaking_lobby_countdown_due(std::int32_t current, std::uint32_t elapsed_ms);
// c_screen_24fd74::update_countdown 0x250A8B: UI sound10 on each new stage
// 0..3.
bool matchmaking_lobby_countdown_beeps(std::int32_t previous, std::int32_t stage);
// 0x22D730 parse_countdown (token E413).
inline std::string matchmaking_lobby_countdown_text(std::int32_t seconds) {
  if (seconds < 0) return "-:--";
  char buffer[24];
  std::snprintf(buffer, sizeof buffer, "%d:%02d", int(seconds / 60), int(seconds % 60));
  return buffer;
}
} // namespace halo2
