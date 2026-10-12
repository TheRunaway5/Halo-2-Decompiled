#pragma once
#include "halo2/vista_lobby_countdown.hpp"
#include "halo2/vista_session_commit.hpp"

namespace halo2 {
// Exact61694B, including the raw5B13C7 query (not the clamped UI stage).
// write is the original5B11E2 boundary; it must retain the resolved user.
bool vista_lobby_cancel_request(std::int32_t controller,std::int32_t requested_stage,
    const VistaLobbyCountdownServices&);
//643C25 ->615A4B(literal10) ->61694B; no error sound on rejection.
bool vista_lobby_cancel_action(std::int32_t controller,
    const VistaLobbyCountdownServices&);

enum class VistaLobbyInputAction { start,setup,back,auxiliary };
// This is a projection of the source route, not its external widget's AL.
// leave/team_edit/base_input still require their genuine owner. In particular
// leave does not mean 'open a confirmation':6454F2 has its own session policy.
enum class VistaLobbyInputRoute {
  consumed,base_input,start_and_base_input,setup,leave,team_edit,unsigned_controller
};
struct VistaLobbyInputServices {
  std::function<std::uint8_t()> pregame; //61520F ->5AD686; event handler only.
  std::function<std::uint8_t(unsigned)> channel_in_use; //60A540(1).
  std::function<std::uint8_t(std::int32_t)> signed_in; //606B50 bit3.
  std::function<std::uint8_t(std::int32_t)> team_edit_active; //screen+1C6C.
  std::function<std::uint8_t()> host_permission; //6152B0 ->5AD940.
  std::function<std::uint8_t()> ready; //6153C9 ->5B1C39; Setup widget only.
  std::function<std::uint8_t()> countdown_active; //616BA2.
  std::function<void(std::int32_t)> start_action; //643A40; result ignored.
  std::function<std::uint8_t(std::int32_t)> cancel_action; //643C25/61694B.
  std::function<void(std::int32_t)> finish_team_edit; //64534C(controller,0).
  std::function<void(std::uint8_t)> settings_change; //615219(1); result ignored.
};
// Original645BB1 event5 parameters0/12,1/13,2. Setup is the distinct64620F
// widget index1, which has different admission and cancels before its factory.
// Other buttons, team editing, unsigned-controller join and ordinary Leave
// remain explicit external routes. Start's inactive base route can produce
// the separate native launch request before its source-owned session begins.
VistaLobbyInputRoute vista_lobby_input(VistaLobbyInputAction,std::int32_t controller,
    const VistaLobbyInputServices&);
// Raw-bank leaves with explicit actual global/current-session predicates.
bool vista_lobby_event_admitted(std::span<const std::byte>,bool lifecycle_enabled); //5AD686.
bool vista_lobby_setup_ready(std::span<const std::byte>,bool current_session); //5B1C39.
bool vista_lobby_controller_signed_in(std::span<const std::byte>,std::int32_t); //606B50.
// Selected-profile bit3 across all four actual127C controller records. These
// are independent of session membership and survive a local session disposal.
std::uint32_t vista_lobby_controller_count(std::span<const std::byte>); //6073AE.
std::uint32_t vista_lobby_controller_mask(std::span<const std::byte>); //60758D.
std::int32_t vista_lobby_peer_count(std::span<const std::byte>,bool current_session); //615285.
//615219 ->5B0FB3/5B0F53 ->5C91CB, bounded to actual local states5..8.
// Remote3/4 reject before mutation; no transport is invented.
bool vista_lobby_settings_change_local(std::span<std::byte>,bool current_session,std::uint8_t);
// Full5B11E2 wrapper, with5C96FB retained as the explicit write callback.
// Fifth source argument is the flag word; the third is the reason. The source
// wrapper supplies identity from local_user[user]+1, NOT from its fifth arg.
bool vista_lobby_countdown_write_request(std::span<const std::byte> session,
    std::span<const std::byte> local_users,bool current_session,
    std::uint8_t running,std::int32_t stage,std::uint32_t reason,std::int32_t user,
    std::uint8_t flag,const std::function<std::uint8_t(const VistaSessionCountdownWrite&)>&);
} // namespace halo2
