#pragma once
#include "halo2/vista_session_storage.hpp"
#include <functional>

namespace halo2 {
struct VistaSessionDisposalServices {
  // Original5BA32F and5B56F9 external calls, only when their actual enabled
  // bytes admit them. Identity/key spans remain valid only during the call.
  std::function<void(std::span<const std::byte>)> release_qos,unregister_key;
};
struct VistaSessionDisposalBanks {
  std::span<std::byte> keys,transport_registrations;
  std::uint32_t transport_owner_word;
};
// Full5CC258 including5CC0A8(true),5CA1CD,5C7FA2/5C7F0C,5C73A3,
//5B56F9 and5BD629, bounded to a genuine sole local host5..8 with no
// remote associations/owner. Every other route rejects before mutation.
// Returns false only for original state0 no-op. Does not reset lifecycle/UI.
bool vista_session_dispose_single_offline(std::span<std::byte>,
    VistaSessionDisposalBanks,bool network_enabled,bool qos_enabled,
    const VistaSessionDisposalServices&);

struct VistaBrowserResetServices {
  std::function<void(std::uint32_t)> close_handle,reset_child;
  std::function<void(std::span<const std::byte>)> release_query;
};
inline constexpr std::size_t vista_browser_source_size=0x4045c;
// Literal5E46E4; browser allocation/platform owners are mandatory if reached.
void vista_browser_reset(std::span<std::byte>,std::uint32_t mode,
    const VistaBrowserResetServices&);

struct VistaLobbyLeaveServices {
  std::function<std::int32_t()> peer_count;
  std::function<void()> browser_reset; //5E4A7E, mode6.
  std::function<void(bool)> disconnect; //5AD6AF, count<=1.
  std::function<void(std::int32_t)> create_task; //60C776(-1,c,642E8C,0,0).
};
// Actual6454F2/616388 sole-peer corridor. Returns false for >1 without effects;
// those host/nonhost confirmation owners remain external. Requeries count
// after browser reset, as original616388 does.
bool vista_lobby_leave_single(std::int32_t controller,const VistaLobbyLeaveServices&);
struct VistaLobbyLeaveTaskServices {
  std::function<std::uint32_t()> lifecycle_context; //6154D8.
  std::function<void(std::uint32_t)> set_status; //639408.
  std::function<void()> complete; //6393FC.
};
// Literal642E8C. Returns whether original completed the task this invocation.
bool vista_lobby_leave_task_update(const VistaLobbyLeaveTaskServices&);
} // namespace halo2
