#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {

// Complete 44B283 ->44A6C6 ->4C1206. All 41 actual child owners must be
// supplied, including their virtual +8(0) operation. This is not a session
// disconnect or a replacement for the children's own reset implementations.
struct VistaGameStateResetChild {
  std::span<std::byte> storage;
  std::function<void(std::uint32_t)> reset;
};
void vista_mainmenu_reset_game_state(std::span<std::byte> root,
    std::span<const VistaGameStateResetChild> children);

// Native ownership for 44A82E's actual 41 category children, its 48C6C0
// profile-service child, and 4C16F5's eight-byte counter allocation. The native
// zeroed allocation boundary supplies otherwise indeterminate padding. Source
// vtable words remain inert metadata; root pointer slots are represented by
// the independently owned C++ arrays, never fabricated 32-bit host pointers.
class VistaMainmenuGameState {
public:
  VistaMainmenuGameState();
  void reset(); // All 41 actual virtual +8 bodies, then 4C1206 and root clear.
  std::span<std::byte> root() noexcept {return root_;}
  std::span<const std::byte> root() const noexcept {return root_;}
  std::span<std::byte> child(std::size_t);
  std::span<const std::byte> child(std::size_t) const;
  std::span<std::byte> profile_service() noexcept {return profile_service_;}
  std::span<std::byte> counter() noexcept {return counter_;}
private:
  std::array<std::byte,0xbc> root_{};
  std::array<std::array<std::byte,24>,41> children_{};
  std::array<std::byte,0x1c0> profile_service_{};
  std::array<std::byte,8> counter_{};
};

enum class VistaMainmenuNetworkResult {
  pbuffer_blocked, special_dialog, network_unavailable_dialog,
  profile_admission_requested, browser_created
};
struct VistaMainmenuNetworkServices {
  // Actual 40E898 XLivePBufferGetByte(879E78,0,&byte). The source initializes
  // byte to zero and ignores HRESULT. A provider must preserve that behavior
  // on failure; it must not substitute a desired enabled/disabled state.
  std::function<void(std::uint8_t&)> pbuffer_get_byte;
  std::function<void()> reset_game_state; // Complete 44B283, mandatory.
  std::function<bool()> network_ready; // 5AC10B.
  std::function<bool()> special_ui; // 609ED8, one or two actual queries.
  std::function<std::uint32_t()> controller_mask; // 60758D, failure only.
  std::function<void(const std::array<std::uint32_t,6>&)> dialog; //60E1D8.
  // This is D712C8+8, as produced by 61B562 notifications ->61AB26.
  // A controller profile flag or XUser identity alone is not this owner.
  std::function<std::uint32_t()> ui_signin_context;
  std::function<void(const std::array<std::uint32_t,8>&)> create_screen;
  // Actual 61A8E3(D712C8,0,612E35), distinct deferred admission path.
  std::function<void(std::uint32_t,std::uint32_t)> request_profile_admission;
};
// Complete protected 40A978 ->40A463 action. The controller comes from the
// actual selected-row event. Opening PC13 does not create a game session.
VistaMainmenuNetworkResult vista_mainmenu_open_network(std::uint32_t controller,
    const VistaMainmenuNetworkServices&);

} // namespace halo2
