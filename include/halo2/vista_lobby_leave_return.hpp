#pragma once
#include <array>
#include <cstdint>
#include <functional>

namespace halo2 {
struct VistaLobbyLeaveReturnState {
  std::uint32_t cached_protocol=0xffffffffu; //Actual863B40, process retained.
};
struct VistaLobbyLeaveReturnServices {
  std::function<std::uint32_t()> protocol; //615EA9, when context is nonzero.
  std::function<std::uint8_t()> request_pending; //4080BA.
  std::function<std::uint32_t()> controller_mask; //60758D, not an ordinal.
  std::function<std::uint8_t(std::uint32_t,std::uint32_t,std::uint32_t)> screen_present; //609850.
  std::function<void(const std::array<std::uint32_t,8>&)> create_screen; //60B11E.
};
// Literal617400 retained-protocol prefix and bounded old-class0/1, or
// old-class2/PC14/protocol2..5 corridor. Returns true only for a real source
// 61A238 factory request. Its ctor61A10F creates source screen13 (not6).
// Other zero-context routes require unbound dialog/signout owners and reject
// explicitly. Nonzero contexts only refresh the actual source protocol cache.
bool vista_lobby_leave_observe(VistaLobbyLeaveReturnState&,std::uint32_t screen,
    std::uint32_t old_class,std::uint32_t context,const VistaLobbyLeaveReturnServices&);
} //namespace halo2
