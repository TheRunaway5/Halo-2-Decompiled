#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace halo2 {
// Values supplied to original19989D by the session services. A missing
// variant is distinct from an allocated variant with state0.
struct FrontendLobbySessionInput {
  int session_class = -1;
  int live_subtype{};
  int campaign_id = -1;
  std::optional<int> variant_state;
  bool settings_valid{};
};
int frontend_lobby_session_enum(const FrontendLobbySessionInput &);
int frontend_lobby_pane(int session_enum, int previous_pane);
// Original22D93B resolves dynamic E41F from the original session enum.
std::uint32_t frontend_lobby_protocol_string(int session_enum);

// Presentation inputs are original query results, not inferred network state.
// Original24FD74 obtains these before updating the screen. Unknown states
// retain the original empty binding rather than inventing a status message.
struct FrontendLobbyInput {
  int session_enum = -1, previous_pane{};
  bool host{}, team_mode{}, local_team_selection{}, countdown{}, start_permission{}, update_guard{};
  int status = -1, delayed_player = -1;
  bool countdown_cancelled{}, loading_map{}, editing_options{};
  int team_count = 1, player_count{}, max_players{};
  std::uint32_t overflow_status_string{};
  int party_privacy{};
  int prior_stats_count{};
};
struct FrontendLobbyPresentation {
  int pane{};
  // All text indices below address the authored pane array. Original direct
  // type6 ordinals include title/subtitle first, hence authored index=ordinal-2.
  std::optional<std::array<bool, 2>> buttons;
  std::optional<unsigned> leader_text;
  std::optional<bool> leader_visible, start_eligible;
  std::optional<std::uint32_t> status_string;
  bool status_warning{};
  std::uint32_t subtitle_string{};
  std::optional<unsigned> privacy_text;
  std::optional<bool> privacy_visible;
  std::optional<std::uint32_t> privacy_string;
  std::optional<unsigned> prior_stats_text;
  std::optional<bool> prior_stats_visible;
};
FrontendLobbyPresentation frontend_lobby_presentation(const FrontendLobbyInput &);
} // namespace halo2
