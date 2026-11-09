#pragma once

#include <array>
#include <cstdint>

namespace halo2::source_bitmap_format_catalogue_tu15 {

// The immutable TU1.5 arrays read by 1301E0 and 139080. Reserved entries
// stay in the tables so their original indices never move.
inline constexpr std::array<std::uint32_t, 24> nonlinear{
    25, 0, 1, 26, 0xffffffff, 0xffffffff, 5, 0xffffffff,
    2, 4, 7, 6, 0xffffffff, 0xffffffff, 12, 14,
    15, 11, 11, 0xffffffff, 0xffffffff, 0xffffffff, 40, 40,
};
inline constexpr std::array<std::uint32_t, 24> linear{
    31, 19, 27, 32, 0xffffffff, 0xffffffff, 17, 0xffffffff,
    16, 29, 30, 18, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 23, 23,
};
inline constexpr std::array<std::uint32_t, 12> bit_depth{
    0x00080008, 0x00100008, 0x00000000, 0x00000010,
    0x00100010, 0x00200020, 0x00000000, 0x00080004,
    0x00080008, 0x00800008, 0x00300060, 0x00100010,
};

}
