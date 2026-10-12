#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
struct VistaUiContextConstructionServices {
  std::function<std::uint32_t(std::uint64_t)> create_listener; //6A8775, maskB.
  std::function<bool()> resource_context_available; //60BB89.
  std::function<void(std::span<std::byte>)> initialize_localized_messages; //61AF21.
};
// Complete61B266 field writes. Listener-enabled is the actual8FAD99 predicate;
// native storage owns the original inlineD712C8 bank independently of profiles.
// Unwritten bytes remain caller-owned, as in the source constructor.
void vista_ui_context_initialize(std::span<std::byte> ui,bool listener_enabled,
    const VistaUiContextConstructionServices&);

// Actual61B562 initial prefix. A valid listener and constructor+0C flag cause
// XUserGetSigninState(0)->61AB26 before any XNotifyGetNext is required.
bool vista_ui_initial_signin_poll(std::span<std::byte> ui,
    const std::function<std::uint32_t(std::uint32_t)>& signin_state,
    const std::function<void(std::uint32_t)>& apply_signin);

// Actual supplied-DLL105B6C30: controller argument is unused by that DLL;
// state1/2 are returned and every other user_info+C value returns0.
std::uint32_t vista_ui_xuser_signin_state(std::span<const std::byte> user_info);

// Actual fresh offline branch61AB26->5AFD25->44A7B5->48C8E4. Requires previous
// context0 and quiescent manager+1B42/+1B43. Other source branches reject;
// they are not replaced by a desired sign-in state or a successful no-op.
// special_ui is queried after the source context write, and must be false.
void vista_ui_apply_fresh_offline_signin(std::span<std::byte> ui,
    std::span<std::byte> manager,std::span<std::byte> game,
    std::span<std::byte> profile_service,
    const std::function<bool()>& special_ui,
    const std::function<void(std::span<std::byte>)>& cancel_overlapped);
} // namespace halo2
