#include "halo2/frontend_lobby.hpp"

namespace halo2 {
int frontend_lobby_session_enum(const FrontendLobbySessionInput &input) {
  // Literal19989D: campaign settings select even session enums; active game
  // variants select odd enums. Live subtype2 selects matchmaking enum6.
  const bool campaign = input.settings_valid && input.campaign_id != -1;
  const bool variant = input.variant_state &&
      ((*input.variant_state > 0 && *input.variant_state <= 4) ||
       (*input.variant_state > 6 && *input.variant_state <= 9));
  if (input.session_class == 0 || input.session_class == 1) {
    if (campaign) return input.session_class * 2;
    if (variant) return input.session_class * 2 + 1;
  } else if (input.session_class == 2) {
    if (input.live_subtype == 2) return 6;
    if (input.live_subtype == 1) {
      if (campaign) return 4;
      if (variant) return 5;
    }
  }
  return -1;
}
int frontend_lobby_pane(int session, int previous) {
  // Original24FF0A is independent of focused START GAME/GAME SETUP button.
  switch (session) {
  case 0: case 2: case 4: return 1;
  case 1: case 3: case 5: return 0;
  case 6: return 2;
  default: return previous;
  }
}
std::uint32_t frontend_lobby_protocol_string(int session) {
  switch (session) {
  case 0: return 0x100005be;
  case 1: return 0x120005c0;
  case 2: return 0x100005c1;
  case 3: return 0x120005c3;
  case 4: return 0x090005c4;
  case 5: return 0x0b0005c6;
  case 6: return 0x0e0005c8;
  default: return 0;
  }
}
namespace {
bool campaign(int session) {
  return session == 0 || session == 2 || session == 4;
}
std::uint32_t status_string(const FrontendLobbyInput &input) {
  // Unchanged24FFC9 branches around literal19A50F's ordinary status table.
  switch (input.status) {
  case 0: return 0x0a0001e3;
  case 1: return 0x0b0001e4;
  case 2: return 0x0f000205;
  case 3: case 24: return 0x0f000206;
  case 4: return 0x0a000208;
  case 5: return 0x11000209;
  case 6: return 0x190007bb;
  case 7:
    if (campaign(input.session_enum)) return 0x0e0001ec;
    return 0x0c0001e5 +
        (input.team_count >= 2 && input.team_count <= 8 ? input.team_count - 2 : 0);
  case 8: return input.editing_options ? 0x170001ee : 0x100001ed;
  case 9: return input.player_count > input.max_players
                     ? input.overflow_status_string : 0x240001ef;
  case 10:
    if (input.delayed_player != -1)
      return input.countdown_cancelled ? 0x1200020d : 0x1b00020c;
    if (input.loading_map)
      return campaign(input.session_enum) ? 0x1500015d : 0x0a000208;
    return input.host ? 0x200001f0 : 0x2b00020b;
  case 11: return 0x0c0001f2;
  case 12: return 0x0e0001f3;
  case 13: case 14: return 0x210001f4;
  case 15: return 0x230001f5;
  case 16: return 0x0f0001f7;
  case 17: return 0x0f0001f8;
  case 18: return 0x140001f9;
  case 19: return 0x190001fa;
  case 20: return 0x160001fb;
  case 21: return 0x0f0001fc;
  case 22: return 0x1c0001f6;
  case 25: return 0x1a000207;
  default: return 0;
  }
}
bool status_warning(int status) {
  // Literal251188 controls text buffer flag0, independently of string ID.
  switch (status) {
  case 2: case 5: case 6: case 7: case 11: case 12: case 13:
  case 15: case 16: case 17: case 18: case 19: case 20: case 21:
  case 22: case 25: return true;
  default: return false;
  }
}
std::uint32_t subtitle(const FrontendLobbyInput &input) {
  // Original24FF61 replaces the authored subtitle at direct type6 ordinal1.
  if (input.countdown) return input.host ? 0x2700021b : 0x2000021a;
  if (input.host) return input.team_mode ? 0x25000217 : 0x26000216;
  return input.team_mode ? 0x1d000219 : 0x1e000218;
}
} // namespace
FrontendLobbyPresentation frontend_lobby_presentation(const FrontendLobbyInput &input) {
  FrontendLobbyPresentation result;
  result.pane = frontend_lobby_pane(input.session_enum, input.previous_pane);
  const bool known_pane = result.pane >= 0 && result.pane <= 2;
  if (!input.update_guard) {
    // Original250DAE writes both button visibility bits and the inverse leader
    // visibility. Guarded updates retain existing values, represented by null.
    result.buttons = std::array{input.host, input.host};
    result.start_eligible = input.host && !input.local_team_selection &&
                            !input.countdown && input.start_permission;
    if (known_pane) {
      result.leader_text = result.pane == 1 ? 38 : 43;
      result.leader_visible = !input.host;
    }
  }
  result.subtitle_string = subtitle(input);
  if (known_pane) {
    result.status_string = status_string(input);
    // Original2501C0/2503C0 uses the signed retained-results count51EC08.
    result.prior_stats_text = result.pane == 1 ? 40 : 45;
    result.prior_stats_visible = input.prior_stats_count > 0;
    result.status_warning = status_warning(input.status);
    result.privacy_text = result.pane == 1 ? 5 : 10;
    // Original250155 suppresses the privacy label in session enum0/1.
    result.privacy_visible = input.session_enum != 0 && input.session_enum != 1;
    if (*result.privacy_visible) {
      switch (input.party_privacy) {
      case 0: result.privacy_string = 0x04000201; break;
      case 1: result.privacy_string = 0x0f000202; break;
      case 2: result.privacy_string = 0x06000203; break;
      }
    }
  }
  return result;
}
} // namespace halo2
