#pragma once
#include "halo2/vista_session_transition.hpp"
#include "halo2/vista_session_countdown.hpp"
#include "halo2/vista_session_commit.hpp"
#include "halo2/vista_session_loading_lifecycle.hpp"

namespace halo2 {
struct VistaSessionStartTransitionState {
  VistaSessionCountdownTaskState countdown;
  std::uint32_t pregame_word18{};
  VistaSessionLoadingHandlerState loading;
  // Actual464551 clears this complete global owner at8B00E8.
  std::array<std::byte,0x840> pregame_bank{};
};
struct VistaSessionCountdownMessage {
  std::uint32_t transport_owner,session_slot,channel;
  // Actual5BED40(slot,channel,0,23h,1Ch,payload).
  std::array<std::byte,0x1c> payload;
};
struct VistaSessionStartTransitionServices {
  VistaSessionTransitionServices transition;
  std::function<void(unsigned,unsigned)> null_network_close; //5B54CF
  std::function<void(unsigned)> pregame_external; //5B07A0(4)
  std::function<std::uint32_t()> seed; //45906F, actual clocks XOR CRT rand
  std::function<std::array<std::uint32_t,2>()> random_identity; //5B5A84(8)
  std::function<std::uint8_t()> network_busy; // Actual926CFC byte5E0D4F
  std::function<void(const VistaSessionCountdownMessage&)> countdown_send;
};
// Full raw-bank5C96FB,5C969F. These retain actual remote message construction
// and local sender selection; no peer identity or host authority is assigned.
bool vista_session_countdown_write(std::span<std::byte>,const VistaSessionCountdownWrite&,
    const VistaSessionStartTransitionServices&);
bool vista_session_countdown_clear(std::span<std::byte>);
// Literal0/1/2 updates and1/2 enters with their actual session getters, timer,
// zero/readiness/commit, parameter setters, player preparation and refresh.
// Only OS/random, network IO, termination and network-busy owners are external.
std::uint8_t vista_session_null_update(VistaLifecycleManager&,std::span<std::byte>,
    const VistaSessionStartTransitionServices&);
void vista_session_pregame_enter(VistaSessionStartTransitionState&,
    const VistaSessionStartTransitionServices&);
std::uint8_t vista_session_pregame_update(VistaLifecycleManager&,std::span<std::byte>,
    VistaSessionStartTransitionState&,const VistaSessionStartTransitionServices&);
void vista_session_start_game_enter(std::span<std::byte>,VistaSessionStartTransitionState&,
    const VistaSessionStartTransitionServices&);
std::uint8_t vista_session_start_game_update(VistaLifecycleManager&,std::span<std::byte>,
    VistaSessionStartTransitionState&,const VistaSessionStartTransitionServices&);
void vista_session_bind_start_handlers(std::span<VistaLifecycleHandler>,std::span<std::byte>,
    VistaSessionStartTransitionState&,const VistaSessionStartTransitionServices&);
} // namespace halo2
