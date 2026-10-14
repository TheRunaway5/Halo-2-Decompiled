#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <span>

namespace halo2 {
struct VistaProfileRefreshServices {
  std::function<void()> refresh_catalog; //446867.
  std::function<std::uint32_t(std::uint32_t)> remap_index; //442A5D.
  std::function<std::uint32_t()> controller_mask; //60758D, only missing profile.
  std::function<void(const std::array<std::uint32_t,6>&)> missing_profile_dialog; //60E1D8.
};
// Original608690 selected-profile catalog refresh, independent of navigation.
void vista_refresh_selected_profiles(std::span<std::byte>,std::uint8_t&,
    const VistaProfileRefreshServices&);
} // namespace halo2
