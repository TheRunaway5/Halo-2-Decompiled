#pragma once
#include "halo2/appearance.hpp"
#include <array>
#include <cstdint>
#include <span>

namespace halo2 {
// Original profile settings+118 / player appearance+0. The final byte is
// retained because 2B102F copies a dword at appearance+5 into the object emblem.
struct FrontendAppearance {
  std::array<std::int8_t, 4> colors{10, 0, 0, 10};
  std::int8_t species{};
  std::uint8_t foreground{}, background{}, flags{}, reserved{};
  auto operator<=>(const FrontendAppearance &) const = default;
};
static_assert(sizeof(FrontendAppearance) == 9);
// 7F790(NONE,false) -> 1A06A0: signed indices, literal matg+160 palette,
// fallback g468710 white; no team-color or default-color override.
ObjectColors frontend_appearance_colors(const FrontendAppearance &,
                                        std::span<const Vec3> palette);
Vec3 frontend_appearance_name_color(const FrontendAppearance &,
                                     std::span<const Vec3> palette);
// 2CB350 / 2CB4E2 / 2CB174 focus producers. Exiting is animation flag bit1.
void preview_frontend_appearance(FrontendAppearance &, unsigned screen,
                                 unsigned color_slot, int focused_datum,
                                 bool exiting = false);
void apply_frontend_appearance(std::span<Material>, const FrontendAppearance &,
                               std::span<const Vec3> palette);
} // namespace halo2
