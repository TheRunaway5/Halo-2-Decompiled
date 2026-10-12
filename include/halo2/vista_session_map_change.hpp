#pragma once
#include "halo2/vista_session_transition.hpp"
#include <functional>

namespace halo2 {
inline constexpr std::size_t vista_world_options_size=0x1188;
using VistaWorldOptions=std::array<std::byte,vista_world_options_size>;
enum class VistaSessionMapChangeIntent { select_map,select_scenario,select_variant,ui_reason,phase_write,remote_close };
struct VistaSessionMapChangeCall {
  VistaSessionMapChangeIntent intent{};
  std::uint32_t argument{};
};
struct VistaSessionPresenceContext {
  std::uint32_t user{},context{},value{};
};
// Literal5B06FC/5B0718/5B0655/5B07A0 argument adapters to imported
// XUserSetContext. The supplied DLL rich-presence implementation remains
// an explicit platform service, separate from selected map/variant storage.
std::optional<VistaSessionPresenceContext> vista_session_map_change_presence(
    const VistaSessionMapChangeCall&,std::span<const std::byte> current_options);
struct VistaSessionMapChangeRequest {
  std::uint8_t accepted{};
  std::optional<VistaWorldOptions> options;
  std::optional<VistaWorldOptions> loaded;
  std::array<VistaSessionMapChangeCall,3> calls{};
  unsigned call_count{};
};
struct VistaSessionMapChangeServices {
  // Actual448D6F constructor owners:664ABB raw tick-rate query,882D40 byte,
  // and5B5A84 random8-byte output. The latter is overwritten by the actual
  // retained identity but its source call remains observable. Supplied664ABB
  // returns literal60; a native30-tick binding is an explicit engine boundary.
  std::function<std::int32_t()> configured_tick_rate;
  std::function<std::uint8_t()> constructor_byte;
  std::function<std::array<std::byte,8>()> random_identity;
  //5B3C3A: write network-clock freeze flag91ABC8 and sample raw437E51 into
  //91ABCC. Getter5B3CA0 uses cached time while true. No simulation pause.
  std::function<void(bool)> pause;
  // Optional actual448DD2 owner for modes outside the recovered multiplayer
  // branch. Without it, the literal multiplayer validator below is used.
  std::function<std::uint8_t(std::span<const std::byte>)> validate_options;
  //40911B synchronously consumes the request;4089BA instead queues it.
  // The native Cache/load implementation is an explicit external boundary.
  // Return nonzero only after fulfillment has published actual current options.
  std::function<std::uint8_t(std::span<const std::byte>)> request_load;
  // Actual449801 AFTER accepted fulfillment. No identity is copied from an
  // unaccepted request, and no loaded-world/readiness result is synthesized.
  std::function<std::span<const std::byte>()> loaded_options;
};
// Literal448DD2 mode2 branch, including machine/player identity consistency.
// Other modes are outside this validator's source-qualified native scope.
bool vista_session_multiplayer_options_valid(std::span<const std::byte>);
// Literal5D90FA, source getters,448D6F and bounded path-copy semantics.
// Caller owns the real session bank and synchronous source service ordering.
VistaSessionMapChangeRequest vista_session_build_map_change(
    std::span<const std::byte>,const VistaSessionMapChangeServices&);
// Literal5D94A1 continuation. On acceptance reads actual loaded options,
// copies+10/+14 to the real handler and returns ordered5B06FC/5B0718,
//5B0655 and5B07A0 event intents. Rejection returns5C596E(1) in states5..8,
// otherwise5CC0A8(0). This image's404541 returns literal false.
VistaSessionMapChangeRequest vista_session_apply_map_change(
    VistaSessionTransitionState&,std::span<const std::byte>,const VistaSessionMapChangeServices&);
} // namespace halo2
