#pragma once
#include <array>
#include <cstdint>

namespace halo2 {
// Original 131ED0's RGB conversion, followed by 1A170's preservation of the
// previous state word's alpha. No clamp is applied. The supplied original
// x87 control word governs integer rounding, independently of host fenv.
// Masked exceptions and 53/64-bit precision are the admitted CPU contract;
// x87 status flags, traps and graphics command publication are separate.
std::uint32_t source_shader_rgb(const std::array<float,3> &,
    std::uint16_t x87_control_word);
std::uint32_t source_shader_rgb_state_word(std::uint32_t previous,
    const std::array<float,3> &,std::uint16_t x87_control_word);
// The scalar branches of 1A170 use the same admitted integer conversion:
// either FISTP directly, or multiply by 255 and replace only alpha.
std::uint32_t source_shader_integer(float,std::uint16_t x87_control_word);
std::uint32_t source_shader_alpha_state_word(std::uint32_t previous,
    float,std::uint16_t x87_control_word);
}
