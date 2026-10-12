#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Supplied PC image33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109.
// Original6176B0 prefix and616EF0. Protected services remain explicit; this
// observer owns neither a session constructor nor a screen/world loader.
enum class VistaMenuReturnQuery {
  screen, world_ready, world_is_mainmenu, protocol, network_enabled,
  lifecycle_enabled, lifecycle_kind, ui_active, ui_disabled, task_gate_a,
  task_gate_b, task_state, task_blocks, local_controller_count, closing,
  world_gate, world_gate_b, controller, loading, loading_secondary
};
enum class VistaMenuReturnAction {
  set_ui_active, cache_task_state, ui_channel, ui_channel_close, session_close,
  screen_factory, pcr16_request, mainmenu_request, other_mainmenu_request,
  observer_suffix
};
struct VistaMenuReturnIntent {
  VistaMenuReturnAction action{};
  std::array<std::uint32_t,8> arguments{};
  unsigned argument_count{};
  bool operator==(const VistaMenuReturnIntent&) const = default;
};
struct VistaMenuReturnState {
  // Actual863B3C retained task-state word, not a lifecycle/session phase.
  std::uint32_t retained_task_state{};
};
struct VistaMenuReturnServices {
  // Return actual raw field/query values in invocation order. Byte predicates
  // use the low8 bits; controller count uses signed16 AX. Repeated queries can
  // change; they are not deduplicated into a preflight snapshot.
  std::function<std::uint32_t(VistaMenuReturnQuery)> query;
  // Optional synchronous external boundary. If absent, only ordered intents
  // are returned. A live owner must execute actions in order and supply actual
  // subsequent query results; it must not treat this mode as applied UI state.
  std::function<void(const VistaMenuReturnIntent&)> action;
};
struct VistaMenuReturnObservation {
  std::uint32_t screen{}, previous_ui_state{}, current_ui_state{};
  std::array<VistaMenuReturnIntent,12> calls{};
  unsigned call_count{};
  std::span<const VistaMenuReturnIntent> call_intents() const noexcept {
    return {calls.data(),call_count};
  }
};
// UI factory identities are literal original callback addresses; arguments
// are the exact eight-word60C851 descriptor, not an allocated native object.
//617400 is an explicit suffix intent {screen,previous_ui,current_ui}; its body
// is outside this helper. No return AL is assigned meaning by6176B0's caller.
VistaMenuReturnObservation vista_menu_return_observe(
    VistaMenuReturnState&,const VistaMenuReturnServices&);
} // namespace halo2
