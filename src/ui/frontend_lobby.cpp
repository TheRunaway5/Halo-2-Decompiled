#include "halo2/frontend_lobby.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
namespace halo2 {
namespace bridge=online_protocol_bridge;
namespace {
template<class T,class U> std::optional<T> optional(const U &value) {
  return value.present ? std::optional<T>{value.value} : std::nullopt;
}
}
int frontend_lobby_session_enum(const FrontendLobbySessionInput &input) {
  bridge::LobbyViewSession f{};f.session_class=input.session_class;f.live_subtype=input.live_subtype;
  f.campaign_id=input.campaign_id;f.variant={input.variant_state.has_value(),input.variant_state.value_or(0)};
  f.settings_valid=input.settings_valid;return bridge::lobby_view_session(f);
}
int frontend_lobby_pane(int session,int previous) {return bridge::lobby_view_pane(session,previous);}
std::uint32_t frontend_lobby_protocol_string(int session) {return bridge::lobby_view_protocol(session);}
FrontendLobbyPresentation frontend_lobby_presentation(const FrontendLobbyInput &input) {
  bridge::LobbyViewInput f{};f.session=input.session_enum;f.previous=input.previous_pane;f.host=input.host;
  f.team_mode=input.team_mode;f.local_team_selection=input.local_team_selection;f.countdown=input.countdown;
  f.start_permission=input.start_permission;f.update_guard=input.update_guard;f.status=input.status;
  f.delayed_player=input.delayed_player;f.countdown_cancelled=input.countdown_cancelled;f.loading_map=input.loading_map;
  f.editing_options=input.editing_options;f.team_count=input.team_count;f.player_count=input.player_count;
  f.max_players=input.max_players;f.overflow_status=input.overflow_status_string;f.privacy=input.party_privacy;
  f.prior_stats=input.prior_stats_count;
  const auto p=bridge::lobby_view_presentation(f);FrontendLobbyPresentation out;
  out.pane=p.pane;if(p.buttons_present)out.buttons=std::array{p.button0,p.button1};
  out.leader_text=optional<unsigned>(p.leader_text);out.leader_visible=optional<bool>(p.leader_visible);
  out.start_eligible=optional<bool>(p.start_eligible);out.status_string=optional<std::uint32_t>(p.status);
  out.status_warning=p.status_warning;out.subtitle_string=p.subtitle;
  out.privacy_text=optional<unsigned>(p.privacy_text);out.privacy_visible=optional<bool>(p.privacy_visible);
  out.privacy_string=optional<std::uint32_t>(p.privacy);out.prior_stats_text=optional<unsigned>(p.prior_stats_text);
  out.prior_stats_visible=optional<bool>(p.prior_stats_visible);return out;
}
}
