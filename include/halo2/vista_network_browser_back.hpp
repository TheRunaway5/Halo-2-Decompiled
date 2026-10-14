#pragma once
#include "halo2/vista_profile_catalog.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <cstddef>

namespace halo2 {
//619DA0's component table admits Back only for event type5/component1 or13.
// Source event type6 takes the browser-selection branch, not this branch.
bool vista_network_browser_back_event(std::uint32_t type,std::uint32_t component) noexcept;
struct VistaBrowserBackServices {
  std::function<std::uint32_t()> channel; // Widget virtual+80.
  std::function<std::uint32_t()> window; // Widget virtual+84.
  std::function<std::uint8_t(std::uint32_t,std::uint32_t,std::uint32_t)> screen_present; //609850.
  std::function<void(std::uint32_t)> mainmenu_reason; //60CE70(2).
  std::function<void(std::uint32_t,std::uint32_t)> close_channel; //6096DA.
  std::function<std::uint8_t()> base_input; //60EEBE, reached only for modal byte!=0.
};
// Literal619E70..619EDE. The caller supplies actual widget+1F00, not a
// synthesized screen history. Channel/window query order differs by branch.
std::uint8_t vista_network_browser_back(std::uint8_t modal,const VistaBrowserBackServices&);

struct VistaBrowserMainmenuReturnState {
  std::uint8_t initialized{}; //D718E4.
  std::uint32_t initialization_clock{}; //D71904.
  std::uint8_t pending_special_menu{}; //D73A8D, cleared before factory call.
};
struct VistaBrowserMainmenuReturnServices {
  std::function<void()> initialize_profiles; //608690, only ifD718E4==0.
  std::function<std::uint32_t()> ui_clock; // ActualD71900 word.
  std::function<std::uint32_t()> controller_count; //6073AE, low16 consumed.
  std::function<std::uint32_t()> controller_mask; //60758D, low16 consumed.
  std::function<void(const std::array<std::uint32_t,8>&)> create_screen; //40B7B9.
};
//60CE70's reason2 branch, bounded to exactly one actual selected profile.
// Other counts require distinct original factories and reject explicitly.
//40B7B9->40B6CA passes literal PC screen6 to611159. The allocation,
// registration, presence0, optional report dialog,840-byte clear and input
// flag reset remain the actual create_screen owner's responsibility.
void vista_network_browser_mainmenu_return_single(VistaBrowserMainmenuReturnState&,
    const VistaBrowserMainmenuReturnServices&);
// Recovered reason2 dispatch for one or multiple selected local profiles.
// Count1 selects PC6; counts2..4 retain the source's FFFF controller mask and
//63F6B7 title9 factory. Zero users has a separate platform-sign-in branch.
void vista_network_browser_mainmenu_return_local(VistaBrowserMainmenuReturnState&,
    const VistaBrowserMainmenuReturnServices&);
using VistaBrowserProfileRefreshServices = VistaProfileRefreshServices;
// Complete608690, retaining its real four-controller storage and882427 byte.
// Catalog refresh/remapping remain the actual profile provider's services.
void vista_network_browser_refresh_profiles(std::span<std::byte> controllers,
    std::uint8_t& refresh_pending,const VistaBrowserProfileRefreshServices&);
} //namespace halo2
