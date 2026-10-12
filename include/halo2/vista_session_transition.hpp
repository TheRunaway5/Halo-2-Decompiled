#pragma once
#include "halo2/vista_lifecycle.hpp"
#include "halo2/vista_session_phase.hpp"
#include <optional>

namespace halo2 {
struct VistaSessionTransitionState {
  // Actual in-game handler+10/+14. Only the map-change owner supplies this.
  std::array<std::uint32_t,2> world_identity;
};
struct VistaSessionWorldOptions {
  std::array<std::uint32_t,2> identity,map;
};
struct VistaSessionTransitionServices {
  VistaSessionPhaseServices phase;
  // Actual5C503B/615A7C/5CC0A8 services, reached only by source preflight.
  std::function<std::uint32_t()> termination_reason;
  std::function<void(unsigned)> notify_termination,remote_close;
  // Actual9178D0 and9178D2 bytes. Keep their owner separated from session phase.
  std::function<std::array<std::uint8_t,2>()> closing_flags;
  std::function<std::uint8_t()> game_over_complete,world_ready;
  std::function<void()> force_end,network_close;
  std::function<VistaSessionWorldOptions()> world_options;
  // Actual5D94A1. The owner retains the requested world identity at+10/+14.
  std::function<void(VistaSessionTransitionState&)> map_change;
  std::function<void(unsigned)> postgame_external;
};
// Literal5D7644,5D95A7,5D903F,5D9704,5D90B5 and5D9070. These execute
// synchronous services in original order and reread actual banks afterwards.
// Unlike the pure query projections, the full5C596E writes and5D75DA requests
// are applied to the caller's real retained session/manager in the same pump.
// External world/transport owners remain mandatory at reached boundaries.
std::uint8_t vista_session_transition_preflight(VistaLifecycleManager&,
    std::span<std::byte>,const VistaSessionTransitionServices&);
std::uint8_t vista_session_in_game_update(VistaLifecycleManager&,
    std::span<std::byte>,VistaSessionTransitionState&,const VistaSessionTransitionServices&);
std::uint8_t vista_session_post_game_update(VistaLifecycleManager&,
    std::span<std::byte>,const VistaSessionTransitionServices&);
void vista_session_in_game_enter(VistaSessionTransitionState&,const VistaSessionTransitionServices&);
void vista_session_in_game_exit(VistaSessionTransitionState&,const VistaSessionTransitionServices&);
void vista_session_post_game_enter(const VistaSessionTransitionServices&);
// Replaces only the source3/4 entries in an actual caller-owned handler table.
// Other five source handlers and the network update are supplied by their
// real owners. Captured state, bank, manager and services must outlive the pump.
void vista_session_bind_transition_handlers(std::span<VistaLifecycleHandler>,
    std::span<std::byte>,VistaSessionTransitionState&,const VistaSessionTransitionServices&);
} // namespace halo2
