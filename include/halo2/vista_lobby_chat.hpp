#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
struct VistaLobbyChatServices {
  // Actual 6A8751/XUserCheckPrivilege. HRESULT and BOOL retain their DWORD
  // bit patterns. The source initializes the BOOL to 0 before this call.
  std::function<std::uint32_t(std::uint32_t user,std::uint32_t privilege,
      std::uint32_t& allowed)> check_privilege;
};
// Complete protected 61AAB4 from supplied halo2.exe SHA33119398...109.
// Reads the actual 609674()->D712C8 presentation context at +8. Context 2
// consumes XUserCheckPrivilege(0,252): only HRESULT==0 and BOOL!=0 allow
// chat. Every other context returns true without consulting that service.
// This query does not manufacture the context or register/hide widgets.
bool vista_lobby_chat_available(std::span<const std::byte> ui_context,
    const VistaLobbyChatServices&);
} // namespace halo2
