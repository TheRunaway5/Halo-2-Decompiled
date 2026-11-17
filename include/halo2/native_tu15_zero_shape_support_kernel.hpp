#pragma once
#include <array>
#include <cstdint>
namespace halo2 {
// Actual2E8D20 produces a local-origin support point. Neither the object nor
// the query direction participates in this owner's ordinary point DATA.
inline constexpr std::array<std::uint32_t,4> native_tu15_zero_shape_support_point()noexcept{return {};}
} // namespace halo2
